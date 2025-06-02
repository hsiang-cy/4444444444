/*
 * @Author: wilson.liu
 * @Date: 2024-03-22 07:08:50
 * @LastEditors: wilson.liu
 * @LastEditTime: 2024-08-22 15:59:36
 * @Description:
 * Copyright (c) 2024 Singularity & Infinity Co., Ltd.
 */

#include "algo_server/api/get_config.hpp"

#include <string>

#include "absl/log/log.h"
#include "algo_server/server.hpp"
#include "algo_server/tools.hpp"

using saico::airouting_2_0::algorithm::kContentType;
using saico::server::kHttpCodeOk;

namespace saico {
namespace airouting_2_0 {
namespace algorithm {

const char* GetConfig::kName = "GetConfig";

GetConfig::GetConfig(const ModuleInfo& module_info, const std::string& operator_code, const std::string& operator_name, const Config& config, TaskManager* task_manager)
    : Api(module_info, operator_code, operator_name), config_(config) {
  task_manager_ = task_manager;
}

GetConfig::~GetConfig() {}

std::shared_ptr<httpserver::http_response> GetConfig::render(const httpserver::http_request& req) {
  LOG(INFO) << kName + std::string("::") + __func__;
  LOG(INFO) << "content: " + (std::string)req.get_content();

  LOG(INFO) << config_.Dump(2);

  return std::shared_ptr<httpserver::http_response>(new httpserver::string_response(config_.Dump(), kHttpCodeOk, kContentType));
}

}  // namespace algorithm
}  // namespace airouting_2_0
}  // namespace saico
