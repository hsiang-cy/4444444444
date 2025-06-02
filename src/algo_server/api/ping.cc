/*
 * @Author: wilson.liu
 * @Date: 2024-03-22 07:08:50
 * @LastEditors: wilson.liu
 * @LastEditTime: 2024-08-22 16:01:11
 * @Description:
 * Copyright (c) 2024 Singularity & Infinity Co., Ltd.
 */

#include "algo_server/api/ping.hpp"

#include <string>

#include "absl/log/log.h"
#include "algo_server/server.hpp"
#include "algo_server/tools.hpp"

using saico::airouting_2_0::algorithm::kContentType;
using saico::server::kHttpCodeOk;

namespace saico {
namespace airouting_2_0 {
namespace algorithm {

const char* Ping::kName = "Ping";

Ping::Ping(const ModuleInfo& module_info, const std::string& operator_code, const std::string& operator_name, const Config& config, TaskManager* task_manager)
    : Api(module_info, operator_code, operator_name), config_(config) {
  task_manager_ = task_manager;
}

Ping::~Ping() {}

std::shared_ptr<httpserver::http_response> Ping::render(const httpserver::http_request& req) {
  // LOG(INFO) << kName + std::string("::") + __func__;
  // LOG(INFO) << "content: " + (std::string)req.get_content();

  std::string msg = "Pong";
  // LOG(INFO) << msg;

  return std::shared_ptr<httpserver::http_response>(new httpserver::string_response(msg, kHttpCodeOk, kContentType));
}

}  // namespace algorithm
}  // namespace airouting_2_0
}  // namespace saico
