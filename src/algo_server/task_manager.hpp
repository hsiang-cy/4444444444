/*
 * @Author: wilson.liu
 * @Date: 2024-03-26 06:10:21
 * @LastEditors: wilson.liu
 * @LastEditTime: 2024-09-23 02:33:35
 * @Description:
 * Copyright (c) 2024 Singularity & Infinity Co., Ltd.
 */

#ifndef SRC_ALGO_SERVER_TASK_MANAGER_HPP_
#define SRC_ALGO_SERVER_TASK_MANAGER_HPP_

#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "algo_fw/framework.hpp"
#include "algo_server/config.hpp"
#include "server/server.hpp"

using saico::algo_fw::Relay;
using saico::server::ModuleInfo;

namespace saico {
namespace tools {

enum TaskStatus { kStandby = 0, kInProgress, kCompleted };

struct TaskData {
  TaskData(const ModuleInfo& module_info, const std::string& operator_code, const std::string& operator_name, const Config& config);
  virtual ~TaskData();

  std::function<void(std::shared_ptr<TaskData>)> api;
  TaskStatus status;

  const ModuleInfo& module_info;
  const std::string& operator_code;
  const std::string& operator_name;
  const Config& config;

  std::string task_id;
  std::string s3_dir;
  int64_t memory_usage;
};

class TaskManager {
 public:
  static const char* kName;

  explicit TaskManager(const Config& config);

  Relay AddTask(const std::string& task_id, std::shared_ptr<TaskData> task_data);
  void ExecuteMultipleTask(const std::string& task_id, std::shared_ptr<TaskData> task_data);
  void Execute();
  static void CatchingSegmentationFault(int signal);

 protected:
  std::mutex mutex_;
  std::map<std::string, std::shared_ptr<TaskData>> tasks_;
  const Config& config_;
  int64_t used_memory_usage_;
};

}  // namespace tools
}  // namespace saico

#endif  // SRC_ALGO_SERVER_TASK_MANAGER_HPP_
