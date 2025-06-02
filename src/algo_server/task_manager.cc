/*
 * @Author: wilson.liu
 * @Date: 2024-03-26 06:10:15
 * @LastEditors: wilson.liu
 * @LastEditTime: 2024-09-27 08:30:19
 * @Description:
 * Copyright (c) 2024 Singularity & Infinity Co., Ltd.
 */

#include "algo_server/task_manager.hpp"

#include <csignal>
#include <thread>

#include "absl/log/log.h"
#include "algo_server/server.hpp"
#include "algo_server/tools.hpp"

using saico::airouting_2_0::algorithm::GetRedisKey;
using saico::airouting_2_0::algorithm::kAlgoError;
using saico::airouting_2_0::algorithm::kError;
using saico::airouting_2_0::algorithm::kTaskId;
using saico::airouting_2_0::algorithm::MakeServerError;
using saico::airouting_2_0::algorithm::MakeServerExceptionMsg;
using saico::airouting_2_0::algorithm::ServerModule;
using saico::server::kCode;
using saico::server::kErrors;
using saico::server::kMessage;
using saico::server::kModule;
using saico::server::kName;
using saico::server::kType;
using saico::server::kVersion;

namespace saico {
namespace tools {

std::map<std::string, std::shared_ptr<TaskData>>* global_tasks;

TaskData::TaskData(const ModuleInfo& module_info, const std::string& operator_code, const std::string& operator_name, const Config& config)
    : module_info(module_info), operator_code(operator_code), operator_name(operator_name), config(config) {
  api = nullptr;
  status = kStandby;

  task_id.clear();
  s3_dir.clear();
  memory_usage = 0;
}

TaskData::~TaskData() {}

const char* TaskManager::kName = "TaskManager";

TaskManager::TaskManager(const Config& config) : config_(config) {
  used_memory_usage_ = 0;

  global_tasks = &tasks_;
  signal(SIGSEGV, CatchingSegmentationFault);
}

Relay TaskManager::AddTask(const std::string& task_id, std::shared_ptr<TaskData> task_data) {
  std::lock_guard<std::mutex> lock(mutex_);

  using saico::airouting_2_0::algorithm::error::task_manager::add::Error;

  LOG(INFO) << kName + std::string("::") + __func__;
  LOG(INFO) << "    " + std::string(kTaskId) + ": " + task_id;

  Relay ret;

  // Check repeated task.
  if (tasks_.count(task_id) > 0) {
    ret.code = Error::kRepeatedTask;
    ret.msg = "Repeated task";
    return ret;
  }

  if (config_.GetMaxMemoryUsagePct() >= 0) {
    // Get current memory usage
    auto memory_usage = GetMemoryUsage();
    if (memory_usage.code) {
      ret.code = Error::kGetMemoryUsage;
      ret.msg = memory_usage.msg;
      return ret;
    }
    LOG(INFO) << "Current memory usage:";
    LOG(INFO) << "    total: " + std::to_string(memory_usage.total / 1024 / 1024) + " MB (" + std::to_string(memory_usage.total) + " B)";
    LOG(INFO) << "    used: " + std::to_string(memory_usage.used / 1024 / 1024) + " MB (" + std::to_string(memory_usage.used) + " B)";

    // Memory usage without tasks.
    if (tasks_.size() == 0) {
      LOG(INFO) << "No tasks, update used memory usage.";
      used_memory_usage_ = memory_usage.used;
    }

    // Memory usage for all tasks.
    int64_t tasks_memory_usage = 0;
    for (auto it = tasks_.begin(); it != tasks_.end(); it++) {
      tasks_memory_usage += it->second->memory_usage;
    }

    // Max memory usage.
    int64_t max_memory_usage = memory_usage.total * config_.GetMaxMemoryUsagePct();

    int64_t need_memory_usage = used_memory_usage_ + tasks_memory_usage + task_data->memory_usage;

    LOG(INFO) << "Judgment conditions for memory usage:";
    LOG(INFO) << "    used: " + std::to_string(used_memory_usage_ / 1024 / 1024) + " MB (" + std::to_string(used_memory_usage_) + " B)";
    LOG(INFO) << "    tasks: " + std::to_string(tasks_memory_usage / 1024 / 1024) + " MB (" + std::to_string(tasks_memory_usage) + " B)";
    LOG(INFO) << "    task: " + std::to_string(task_data->memory_usage / 1024 / 1024) + " MB (" + std::to_string(task_data->memory_usage) + " B)";
    LOG(INFO) << "    need: " + std::to_string(need_memory_usage / 1024 / 1024) + " MB (" + std::to_string(need_memory_usage) + " B)";
    LOG(INFO) << "    max: " + std::to_string(max_memory_usage / 1024 / 1024) + " MB (" + std::to_string(max_memory_usage) + " B)";

    // Check memory usage
    if (need_memory_usage >= max_memory_usage) {
      ret.code = Error::kNotEnoughMemory;
      ret.msg = "Not enough memory";
      return ret;
    }
  }

  tasks_.emplace(task_id, task_data);

  return ret;
}

void TaskManager::ExecuteMultipleTask(const std::string& task_id, std::shared_ptr<TaskData> task_data) {
  LOG(INFO) << kName + std::string("::") + __func__;

  {
    std::lock_guard<std::mutex> lock(mutex_);
    task_data->status = TaskStatus::kInProgress;
  }

  task_data->api(task_data);

  {
    std::lock_guard<std::mutex> lock(mutex_);
    task_data->status = TaskStatus::kCompleted;
  }
}

void TaskManager::Execute() {
  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::lock_guard<std::mutex> lock(mutex_);

    for (auto it = tasks_.begin(); it != tasks_.end();) {
      auto task_data = it->second;

      switch (task_data->status) {
        case TaskStatus::kStandby:
        case TaskStatus::kInProgress: {
          switch (task_data->status) {
            case TaskStatus::kStandby: {
              // LOG(INFO) << it->first + ", Standby";
              std::thread t(&TaskManager::ExecuteMultipleTask, this, it->first, task_data);
              t.detach();

              break;
            }

            case TaskStatus::kInProgress: {
              // LOG(INFO) << it->first + ", InProgress";
              break;
            }

            default: {
              break;
            }
          }

          it++;

          break;
        }

        case TaskStatus::kCompleted: {
          // LOG(INFO) << it->first + ", Completed";
          it = tasks_.erase(it);

          break;
        }

        default: {
          it++;
          break;
        }
      }
    }
  }
}

void TaskManager::CatchingSegmentationFault(int signal) {
  LOG(INFO) << kName + std::string("::") + __func__;

  for (auto it = global_tasks->begin(); it != global_tasks->end(); it++) {
    ServerModule server_module;
    std::string redis_data;

    try {
      std::shared_ptr<TaskData> data = (it->second);

      // init
      server_module.Init(data->module_info, data->operator_code, data->operator_name);

      // redis
      {
        std::string redis_key_type_set;
        std::string redis_key_type_pub;

        {
          // init error data
          redis_key_type_set = kError;
          redis_key_type_pub = kAlgoError;

          auto redis_data_json = nlohmann::json::object();

          redis_data_json[kErrors] = nlohmann::json::array();

          ServerException server_exception{saico::airouting_2_0::algorithm::error::Error::kGeneral,
                                           saico::airouting_2_0::algorithm::error::general::Error::kSegmentationFault,
                                           saico::airouting_2_0::algorithm::error::general::segmentation_fault::Error::kSegmentationFault,
                                           __func__};

          auto server_error = MakeServerError(server_exception);
          redis_data_json[kErrors].push_back({
              {kCode, server_error.code},
              {kMessage, server_error.message},
              {kName, server_error.name},
              {kType, server_error.type},
          });

          redis_data_json[kModule] = {{kCode, server_module.GetCode()}, {kName, server_module.GetName()}, {kVersion, server_module.GetVersion()}};

          redis_data = redis_data_json.dump();
        }

        // set
        {
          LOG(INFO) << "SetRedis: ";
          LOG(INFO) << "    key:  " << GetRedisKey(data->task_id, redis_key_type_set);
          LOG(INFO) << "    data: " << redis_data;
          auto relay = SetRedis(data->config, data->config.GetRedisIndexSet(), GetRedisKey(data->task_id, redis_key_type_set), redis_data);
          if (relay.code) {
            ServerException server_exception{saico::airouting_2_0::algorithm::error::kRedis, saico::airouting_2_0::algorithm::error::redis::kSet, relay.code, relay.msg};
            LOG(ERROR) << MakeServerExceptionMsg(server_exception);
          }
        }

        // pub
        {
          LOG(INFO) << "PubRedis: ";
          LOG(INFO) << "    key:  " << GetRedisKey(data->task_id, redis_key_type_pub);
          LOG(INFO) << "    data: " << data->task_id;
          auto relay = PubRedis(data->config, data->config.GetRedisIndexPub(), redis_key_type_pub, data->task_id);
          if (relay.code) {
            ServerException server_exception{saico::airouting_2_0::algorithm::error::kRedis, saico::airouting_2_0::algorithm::error::redis::kPub, relay.code, relay.msg};
            LOG(ERROR) << MakeServerExceptionMsg(server_exception);
          }
        }
      }
    } catch (const std::exception& e) {
      LOG(ERROR) << e.what();
    }
  }

  exit(1);
}

}  // namespace tools
}  // namespace saico
