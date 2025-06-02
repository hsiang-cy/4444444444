/*
 * @Author: wilson.liu
 * @Date: 2023-11-29 07:57:51
 * @LastEditors: wilson.liu
 * @LastEditTime: 2024-09-27 07:46:36
 * @Description:
 * Copyright (c) 2023 Singularity & Infinity Co., Ltd.
 */

#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/log/log.h"
#include "absl/log/log_sink_registry.h"
#include "algo_server/api/air_algorithm.hpp"
#include "algo_server/api/get_config.hpp"
#include "algo_server/api/ping.hpp"
#include "algo_server/config.hpp"
#include "algo_server/server.hpp"
#include "algo_server/task_manager.hpp"
#include "algo_server/tools.hpp"
#include "algo_server/version.hpp"
#include "httpserver/httpserver.hpp"
#include "server/server.hpp"

using saico::airouting_2_0::algorithm::AirAlgorithm;
using saico::airouting_2_0::algorithm::GetConfig;
using saico::airouting_2_0::algorithm::MakeErrorSubTypeData;
using saico::airouting_2_0::algorithm::MakeErrorTypeData;
using saico::airouting_2_0::algorithm::MakeServerExceptionMsg;
using saico::airouting_2_0::algorithm::Ping;
using saico::server::ModuleInfo;
using saico::server::ServerException;
using saico::tools::AbslLogSink;
using saico::tools::Config;
using saico::tools::InitAws;
using saico::tools::TaskManager;

int main(int argc, char* argv[]) {
  // init log
  std::unique_ptr<AbslLogSink> absl_log_sink = nullptr;
  absl::InitializeLog();
  absl::SetStderrThreshold(absl::LogSeverity::kInfo);

  // init config
  Config config;
  auto relays_parse_env_var = config.ParseEnvVar();

  // init log dir
  int ret_make_log_dir = 0;
  if (!config.GetLogDir().empty()) {
    std::string cmd = "mkdir -p " + config.GetLogDir();
    ret_make_log_dir = system(cmd.c_str());
    if (!ret_make_log_dir) {
      absl_log_sink = std::make_unique<AbslLogSink>(config.GetLogDir(), argv[0]);
      absl::AddLogSink(absl_log_sink.get());
    }
  }

  // start
  LOG(INFO) << "PROJECT_VERSION: " + std::string(PROJECT_VERSION);

  if (relays_parse_env_var.size() > 0) {
    for (auto& relay : relays_parse_env_var) {
      LOG(WARNING) << relay.msg;
    }
  }
  LOG(INFO) << config.Dump(2);

  if (!config.GetLogDir().empty()) {
    if (ret_make_log_dir) {
      LOG(ERROR) << "Make log directory failed, " + config.GetLogDir();
    } else {
      LOG(INFO) << "Make log directory, " + config.GetLogDir();
    }
  }

  auto relay_init_aws = InitAws(config);
  if (relay_init_aws.code) {
    LOG(WARNING) << relay_init_aws.msg;
  }

  // Module Info
  ModuleInfo module_info;
  module_info.system_code = "402";
  module_info.module_code = "110";
  module_info.module_name = "ALGORITHMS_SERVER";
  module_info.version = PROJECT_VERSION;

  // task manager
  TaskManager task_manager(config);

  // httpserver
  httpserver::webserver web_server = httpserver::create_webserver(config.GetWebServerPort()).start_method(httpserver::http::http_utils::INTERNAL_SELECT).max_threads(16);

  Ping ping(module_info, "1001", "PING", config, &task_manager);
  web_server.register_resource("/ping", &ping, true);

  GetConfig get_config(module_info, "1002", "GET_CONFIG", config, &task_manager);
  web_server.register_resource("/get_config", &get_config, true);

  AirAlgorithm air_algorithm(module_info, "1003", "AIR_ALGORITHM", config, &task_manager);
  web_server.register_resource("/air_algorithm", &air_algorithm, true);

  web_server.start(false);

  task_manager.Execute();

  return 0;
}
