/*
 * @Author: wilson.liu
 * @Date: 2024-03-22 07:08:57
 * @LastEditors: wilson.liu
 * @LastEditTime: 2024-08-22 16:00:37
 * @Description:
 * Copyright (c) 2024 Singularity & Infinity Co., Ltd.
 */

#ifndef SRC_ALGO_SERVER_API_GET_CONFIG_HPP_
#define SRC_ALGO_SERVER_API_GET_CONFIG_HPP_

#include <memory>
#include <string>

#include "algo_server/config.hpp"
#include "algo_server/task_manager.hpp"
#include "httpserver/httpserver.hpp"
#include "server/server.hpp"

using saico::server::Api;
using saico::server::ModuleInfo;
using saico::tools::Config;
using saico::tools::TaskManager;

namespace saico {
namespace airouting_2_0 {
namespace algorithm {

class GetConfig : public httpserver::http_resource, public Api {
 public:
  static const char* kName;

  GetConfig(const ModuleInfo& module_info, const std::string& operator_code, const std::string& operator_name, const Config& config, TaskManager* task_manager);
  ~GetConfig();

  std::shared_ptr<httpserver::http_response> render(const httpserver::http_request& req);

 protected:
  const Config& config_;
  TaskManager* task_manager_;
};

}  // namespace algorithm
}  // namespace airouting_2_0
}  // namespace saico

#endif  // SRC_ALGO_SERVER_API_GET_CONFIG_HPP_
