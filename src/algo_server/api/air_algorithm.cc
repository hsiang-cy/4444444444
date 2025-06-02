/*
 * @Author: wilson.liu
 * @Date: 2023-11-03 05:52:30
 * @LastEditors: wilson.liu
 * @LastEditTime: 2024-10-24 14:57:29
 * @Description:
 * Copyright (c) 2023 Singularity & Infinity Co., Ltd.
 */

#include "algo_server/api/air_algorithm.hpp"

#include <string>
#include <utility>
#include <vector>

#include "absl/log/globals.h"
#include "absl/log/log.h"
#include "algo_server/server.hpp"
#include "algo_server/tools.hpp"
#include "algorithms/airio/output.pb.h"
#include "algorithms/algorithm.hpp"

using saico::airouting_2_0::algorithm::kContentType;
using saico::algo_fw::Algorithm;
using saico::server::kCode;
using saico::server::kErrors;
using saico::server::kHttpCodeOk;
using saico::server::kMessage;
using saico::server::kModule;
using saico::server::kType;
using saico::server::kVersion;
using saico::server::ServerException;
using saico::server::ServerResponse;
using saico::tools::AppendTailSlash;
using saico::tools::GetRedis;
using saico::tools::Http;
using saico::tools::RelayGetInputData;
using saico::tools::SetRedis;

namespace saico {
namespace airouting_2_0 {
namespace algorithm {

AirAlgorithmData::AirAlgorithmData(const ModuleInfo& module_info, const std::string& operator_code, const std::string& operator_name, const Config& config) : TaskData(module_info, operator_code, operator_name, config) {}

AirAlgorithmData::~AirAlgorithmData() {}

const char* AirAlgorithm::kName = "AirAlgorithm";
const char* AirAlgorithm::kAirAlgorithm = "air_algorithm";

AirAlgorithm::AirAlgorithm(const ModuleInfo& module_info, const std::string& operator_code, const std::string& operator_name, const Config& config, TaskManager* task_manager)
    : Api(module_info, operator_code, operator_name), config_(config) {
  task_manager_ = task_manager;
}

AirAlgorithm::~AirAlgorithm() {}

std::shared_ptr<httpserver::http_response> AirAlgorithm::render(const httpserver::http_request& req) {
  LOG(INFO) << kName + std::string("::") + __func__;
  LOG(INFO) << "content: " + (std::string)req.get_content();

  ServerResponse server_response(module_info_, operator_code_, operator_name_);
  int http_code = kHttpCodeOk;
  std::string task_id;

  try {
    auto content = nlohmann::json::parse(req.get_content());

    // check content
    if (!content.contains(kTaskId)) {
      ServerException server_exception{
          saico::server::error::Error::kInputEmpty, saico::server::error::input_empty::kInputEmpty, error::input_empty::input_empty::Error::kTaskId, "Column does not exist, " + std::string(kTaskId) + "."};
      LOG(ERROR) << MakeServerExceptionMsg(server_exception);
      throw server_exception;
    }
    task_id = content[kTaskId];

    if (!content.contains(kS3Dir)) {
      ServerException server_exception{
          saico::server::error::Error::kInputEmpty, saico::server::error::input_empty::kInputEmpty, error::input_empty::input_empty::Error::kS3Dir, "Column does not exist, " + std::string(kS3Dir) + "."};
      LOG(ERROR) << MakeServerExceptionMsg(server_exception);
      throw server_exception;
    }

    // add task
    auto data = std::make_shared<AirAlgorithmData>(module_info_, operator_code_, operator_name_, config_);
    data->api = Execute;
    data->task_id = task_id;
    data->s3_dir = content[kS3Dir];

    auto relay = task_manager_->AddTask(task_id, data);
    if (relay.code) {
      ServerException server_exception{error::Error::kTaskManager, error::task_manager::Error::kAdd, relay.code, relay.msg};
      LOG(ERROR) << MakeServerExceptionMsg(server_exception);
      throw server_exception;
    }

    // response
    auto res_data = nlohmann::json::object();
    res_data[kTaskId] = task_id;

    server_response.SetData(res_data);
  } catch (const std::exception& e) {
    ServerException server_exception{error::Error::kGeneral, error::general::Error::kUnexpected, error::general::unexpected::Error::kUnexpected, e.what()};
    LOG(ERROR) << "exception: " << MakeServerExceptionMsg(server_exception);

    server_response.AddErrors(server_exception, MakeErrorTypeData, MakeErrorSubTypeData);
    http_code = MakeErrorSubTypeData(server_exception.error_type, server_exception.error_sub_type).http_code;
  } catch (const ServerException& server_exception) {
    LOG(ERROR) << "server exception: " << MakeServerExceptionMsg(server_exception);

    server_response.AddErrors(server_exception, MakeErrorTypeData, MakeErrorSubTypeData);
    http_code = MakeErrorSubTypeData(server_exception.error_type, server_exception.error_sub_type).http_code;
  }

  LOG(INFO) << server_response.Dump(2);

  return std::shared_ptr<httpserver::http_response>(new httpserver::string_response(server_response.Dump(), http_code, kContentType));
}

std::shared_ptr<AirAlgorithmData> global_data = nullptr;
Relay PostProgress(const std::uint32_t& progress) {
  LOG(INFO) << AirAlgorithm::kName + std::string("::") + __func__ + "(Callback)";

  Relay relay;

  if (global_data == nullptr) {
    relay.code = 1;
    relay.msg = "Failed to get global_data";
    return relay;
  }

  if (progress < 0 || 99 < progress) {
    relay.code = 2;
    relay.msg = "Data format error, progress: " + std::to_string(progress);
    return relay;
  }

  const std::string& task_id = global_data->task_id;
  const Config& config = global_data->config;

  // init ok data
  std::string redis_key_type_set = GetRedisKey(task_id, kStatus);
  std::string redis_key_type_pub = kAlgoStatus;
  std::string redis_data_set = std::to_string(progress);
  std::string redis_data_pub = task_id;

  // set
  {
    LOG(INFO) << "SetRedis: ";
    LOG(INFO) << "    key:  " << redis_key_type_set;
    LOG(INFO) << "    data: " << redis_data_set;
    auto relay_set_redis = SetRedis(config, config.GetRedisIndexSet(), redis_key_type_set, redis_data_set);
    if (relay_set_redis.code) {
      ServerException server_exception{error::kRedis, error::redis::kSet, relay_set_redis.code, relay_set_redis.msg};
      LOG(ERROR) << MakeServerExceptionMsg(server_exception);

      relay.code = 3;
      relay.msg = MakeServerExceptionMsg(server_exception);
      return relay;
    }
  }

  // pub
  {
    LOG(INFO) << "PubRedis: ";
    LOG(INFO) << "    key:  " << redis_key_type_pub;
    LOG(INFO) << "    data: " << redis_data_pub;
    auto relay_pub_redis = PubRedis(config, config.GetRedisIndexPub(), redis_key_type_pub, redis_data_pub);
    if (relay_pub_redis.code) {
      ServerException server_exception{error::kRedis, error::redis::kPub, relay_pub_redis.code, relay_pub_redis.msg};
      LOG(ERROR) << MakeServerExceptionMsg(server_exception);

      relay.code = 4;
      relay.msg = MakeServerExceptionMsg(server_exception);
      return relay;
    }
  }

  return relay;
}

Relay SaveFile(const std::string& file, const std::string& content) {
  LOG(INFO) << AirAlgorithm::kName + std::string("::") + __func__ + "(Callback)";

  Relay relay;

  if (global_data == nullptr) {
    relay.code = 1;
    relay.msg = "Failed to get global_data";
    return relay;
  }

  const std::string& task_id = global_data->task_id;
  const std::string& s3_dir = global_data->s3_dir;
  const Config& config = global_data->config;

  std::string tmp_dir = AppendTailSlash(kAwsS3TmpDir) + task_id;

  LOG(INFO) << "PutOutputData: ";
  LOG(INFO) << "    bucket name: " << config.GetAwsS3BucketName();

  std::string object_name = AppendTailSlash(s3_dir) + file;
  LOG(INFO) << "    object_name: " << object_name;

  auto relay_put_output_data = saico::tools::PutOutputData(content, tmp_dir, object_name, config.GetAwsS3BucketName());
  if (relay_put_output_data.code) {
    ServerException server_exception{error::kData, error::data::kPutOutputData, relay_put_output_data.code, relay_put_output_data.msg};
    LOG(ERROR) << MakeServerExceptionMsg(server_exception);

    relay.code = 2;
    relay.msg = MakeServerExceptionMsg(server_exception);
    return relay;
  }

  return relay;
}

void AirAlgorithm::Execute(std::shared_ptr<TaskData> task_data) {
  LOG(INFO) << kName + std::string("::") + __func__;

  // input
  std::shared_ptr<AirAlgorithmData> data = nullptr;

  // output
  ServerModule server_module;
  std::string tmp_dir;
  air20::output::AirOutput s3_data;

  try {
    data = std::dynamic_pointer_cast<AirAlgorithmData>(task_data);
    const std::string& task_id = data->task_id;
    const std::string& s3_dir = data->s3_dir;
    const ModuleInfo& module_info = data->module_info;
    const std::string& operator_code = data->operator_code;
    const std::string& operator_name = data->operator_name;
    const Config& config = data->config;
    global_data = data;

    LOG(INFO) << "    " + std::string(kTaskId) + ": " + task_id;
    LOG(INFO) << "    " + std::string(kS3Dir) + ": " + s3_dir;

    // init
    server_module.Init(module_info, operator_code, operator_name);
    tmp_dir = AppendTailSlash(kAwsS3TmpDir) + task_id;

    // get input data
    RelayGetInputData relay_get_input_data;
    {
      LOG(INFO) << MakeErrorSubTypeData(error::kData, error::data::kGetInputData).name;

      std::string object_name = AppendTailSlash(s3_dir) + kAwsS3InputFile;
      relay_get_input_data = saico::tools::GetInputData(tmp_dir, object_name, config.GetAwsS3BucketName());
      if (relay_get_input_data.code) {
        ServerException server_exception{error::kData, error::data::kGetInputData, relay_get_input_data.code, relay_get_input_data.msg};
        LOG(ERROR) << MakeServerExceptionMsg(server_exception);
        throw server_exception;
      }
    }

    // air algorithm
    {
      LOG(INFO) << std::string(kAirAlgorithm) + "_" + air::AirAlgorithm().GetVersion();

      air::AirAlgorithm air_algorithm;
      air_algorithm.SetData(relay_get_input_data.proto_str);
      air_algorithm.SetPostProgressCallback(PostProgress);
      air_algorithm.SetSaveFileCallback(SaveFile);

      auto ret_air_algorithm = air_algorithm.Execute();
      absl::SetStderrThreshold(absl::LogSeverity::kInfo);

      if (!s3_data.ParseFromString(ret_air_algorithm)) {
        ServerException server_exception{error::kData, error::data::kParseOutput, error::data::parse_output::kParseOutput, "Failed to ParseFromString"};
        LOG(ERROR) << MakeServerExceptionMsg(server_exception);
        throw server_exception;
      }

      if (s3_data.errors_size() > 0) {
        ServerException server_exception{error::kAlgorithms, error::algorithms::kExecute, error::algorithms::execute::kAirAlgorithm, "Failed to " + std::string(kAirAlgorithm)};
        LOG(ERROR) << MakeServerExceptionMsg(server_exception);
        throw server_exception;
      }
    }
  } catch (const std::exception& e) {
    absl::SetStderrThreshold(absl::LogSeverity::kInfo);

    ServerException server_exception{error::Error::kGeneral, error::general::Error::kUnexpected, error::general::unexpected::Error::kUnexpected, e.what()};
    LOG(ERROR) << "exception: " << MakeServerExceptionMsg(server_exception);

    auto server_error = MakeServerError(server_exception);
    auto* error = s3_data.add_errors();
    COPY_ERROR(server_error, error);
  } catch (const ServerException& server_exception) {
    absl::SetStderrThreshold(absl::LogSeverity::kInfo);

    LOG(ERROR) << "server exception: " << MakeServerExceptionMsg(server_exception);

    auto server_error = MakeServerError(server_exception);
    auto* error = s3_data.add_errors();
    COPY_ERROR(server_error, error);
  }

  const std::string& task_id = data->task_id;
  const std::string& s3_dir = data->s3_dir;
  const Config& config = data->config;

  // s3
  {
    LOG(INFO) << "PutOutputData: ";
    LOG(INFO) << "    bucket name: " << config.GetAwsS3BucketName();

    auto module = s3_data.mutable_module();
    module->set_code(server_module.GetCode());
    module->set_name(server_module.GetName());
    module->set_version(server_module.GetVersion());

    std::string object_name = AppendTailSlash(s3_dir) + kAwsS3OutputFile;
    LOG(INFO) << "    object_name: " << object_name;

    std::string proto_str;
    if (s3_data.SerializeToString(&proto_str)) {
      auto relay_put_output_data = saico::tools::PutOutputData(proto_str, tmp_dir, object_name, config.GetAwsS3BucketName());
      if (relay_put_output_data.code) {
        ServerException server_exception{error::kData, error::data::kPutOutputData, relay_put_output_data.code, relay_put_output_data.msg};
        LOG(ERROR) << MakeServerExceptionMsg(server_exception);
      }
    } else {
      ServerException server_exception{error::kData, error::data::kSerializeOutput, error::data::serialize_output::kSerializeOutput, "Failed to SerializeToString"};
      LOG(ERROR) << MakeServerExceptionMsg(server_exception);
    }
  }

  // redis
  {
    std::string redis_key_type_set;
    std::string redis_key_type_pub;
    std::string redis_data_set;
    std::string redis_data_pub = task_id;

    if (s3_data.errors_size() <= 0) {
      // init ok data
      redis_key_type_set = GetRedisKey(task_id, kStatus);
      redis_key_type_pub = kAlgoStatus;
      redis_data_set = "100";

    } else {
      // init error data
      redis_key_type_set = GetRedisKey(task_id, kError);
      redis_key_type_pub = kAlgoError;

      auto redis_data_json = nlohmann::json::object();

      redis_data_json[kErrors] = nlohmann::json::array();

      auto& errors = redis_data_json[kErrors];
      for (auto& error : s3_data.errors()) {
        errors.push_back({{kCode, error.code()}, {kName, error.name()}, {kType, error.type()}, {kMessage, error.message()}});
      }

      redis_data_json[kModule] = {{kCode, server_module.GetCode()}, {kName, server_module.GetName()}, {kVersion, server_module.GetVersion()}};

      redis_data_set = redis_data_json.dump();
    }

    // set
    {
      LOG(INFO) << "SetRedis: ";
      LOG(INFO) << "    key:  " << redis_key_type_set;
      LOG(INFO) << "    data: " << redis_data_set;
      auto relay_set_redis = SetRedis(config, config.GetRedisIndexSet(), redis_key_type_set, redis_data_set);
      if (relay_set_redis.code) {
        ServerException server_exception{error::kRedis, error::redis::kSet, relay_set_redis.code, relay_set_redis.msg};
        LOG(ERROR) << MakeServerExceptionMsg(server_exception);
      }
    }

    // pub
    {
      LOG(INFO) << "PubRedis: ";
      LOG(INFO) << "    key:  " << redis_key_type_pub;
      LOG(INFO) << "    data: " << redis_data_pub;
      auto relay_pub_redis = PubRedis(config, config.GetRedisIndexPub(), redis_key_type_pub, redis_data_pub);
      if (relay_pub_redis.code) {
        ServerException server_exception{error::kRedis, error::redis::kPub, relay_pub_redis.code, relay_pub_redis.msg};
        LOG(ERROR) << MakeServerExceptionMsg(server_exception);
      }
    }
  }

  LOG(INFO) << "Completed";
}

}  // namespace algorithm
}  // namespace airouting_2_0
}  // namespace saico
