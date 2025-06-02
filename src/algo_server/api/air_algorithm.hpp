/*
 * @Author: wilson.liu
 * @Date: 2023-11-03 05:52:30
 * @LastEditors: wilson.liu
 * @LastEditTime: 2024-09-27 08:40:10
 * @Description:
 * Copyright (c) 2023 Singularity & Infinity Co., Ltd.
 */

#ifndef SRC_ALGO_SERVER_API_AIR_ALGORITHM_HPP_
#define SRC_ALGO_SERVER_API_AIR_ALGORITHM_HPP_

#include <memory>
#include <string>

#include "algo_server/config.hpp"
#include "algo_server/task_manager.hpp"
#include "httpserver/httpserver.hpp"
#include "server/server.hpp"

using saico::server::Api;
using saico::server::ModuleInfo;
using saico::tools::Config;
using saico::tools::TaskData;
using saico::tools::TaskManager;

namespace saico {
namespace airouting_2_0 {
namespace algorithm {

struct AirAlgorithmData : public TaskData {
  AirAlgorithmData(const ModuleInfo& module_info, const std::string& operator_code, const std::string& operator_name, const Config& config);
  virtual ~AirAlgorithmData();
};

class AirAlgorithm : public httpserver::http_resource, public Api {
 public:
  static const char* kName;
  static const char* kAirAlgorithm;

  AirAlgorithm(const ModuleInfo& module_info, const std::string& operator_code, const std::string& operator_name, const Config& config, TaskManager* task_manager);
  ~AirAlgorithm();

  std::shared_ptr<httpserver::http_response> render(const httpserver::http_request& req);
  static void Execute(std::shared_ptr<TaskData> task_data);

 protected:
  const Config& config_;
  TaskManager* task_manager_;
};

}  // namespace algorithm
}  // namespace airouting_2_0
}  // namespace saico

#endif  // SRC_ALGO_SERVER_API_AIR_ALGORITHM_HPP_
