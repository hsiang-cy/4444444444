/*
 * @Author: wilson.liu
 * @Date: 2024-03-22 07:08:50
 * @LastEditors: wilson.liu
 * @LastEditTime: 2024-09-27 07:06:06
 * @Description:
 * Copyright (c) 2024 Singularity & Infinity Co., Ltd.
 */

#include "algo_server/server.hpp"

#include "absl/log/log.h"

using saico::server::ServerException;

namespace saico {
namespace airouting_2_0 {
namespace algorithm {

const char* kContent = "content";
const char* kTaskId = "taskId";
const char* kS3Dir = "s3Dir";
const char* kIterations = "iterations";
const char* kData = "data";
const char* kError = "error";

const char* kContentType = "application/json";

const char* kAwsS3TmpDir = "/tmp/aws_s3";
const char* kAwsS3InputFile = "input";
const char* kAwsS3OutputFile = "output";

const char* kStatus = "status";
// const char* kError = "error";

const char* kAlgoStatus = "ALGO_STATUS";
const char* kAlgoError = "ALGO_ERROR";

std::string GetRedisKey(const std::string& task_id, const std::string& key) { return task_id + ":" + key; }

ErrorCodeAlgoServer::ErrorCodeAlgoServer() {
  error_type_data_[error::Error::kGeneral] = {"Algorithms Server", {}};
  error_type_data_[error::Error::kGeneral][error::general::Error::kUnexpected] = {"Unexpected", 400};
  error_type_data_[error::Error::kGeneral][error::general::Error::kSegmentationFault] = {"Segmentation Fault", 400};

  error_type_data_[error::Error::kConfig] = {"Config", {}};
  error_type_data_[error::Error::kConfig][error::config::kParse] = {"Parse", 400};

  error_type_data_[error::Error::kTaskManager] = {"Task Manager", {}};
  error_type_data_[error::Error::kTaskManager][error::task_manager::kAdd] = {"Add", 400};

  error_type_data_[error::Error::kRedis] = {"Redis", {}};
  error_type_data_[error::Error::kRedis][error::redis::kConnect] = {"Connect", 400};
  error_type_data_[error::Error::kRedis][error::redis::kSet] = {"Set", 400};
  error_type_data_[error::Error::kRedis][error::redis::kGet] = {"Get", 400};
  error_type_data_[error::Error::kRedis][error::redis::kSub] = {"Sub", 400};
  error_type_data_[error::Error::kRedis][error::redis::kPub] = {"Pub", 400};

  error_type_data_[error::Error::kAws] = {"Aws", {}};
  error_type_data_[error::Error::kAws][error::aws::kInit] = {"Init", 400};
  error_type_data_[error::Error::kAws][error::aws::kGetS3Object] = {"Get S3 Object", 400};
  error_type_data_[error::Error::kAws][error::aws::kPutS3Object] = {"Put S3 Object", 400};

  error_type_data_[error::Error::kData] = {"Data", {}};
  error_type_data_[error::Error::kData][error::data::kGetInputData] = {"Get Input Data", 400};
  error_type_data_[error::Error::kData][error::data::kParseOutput] = {"Parse Output", 400};
  error_type_data_[error::Error::kData][error::data::kSerializeOutput] = {"Serialize Output", 400};
  error_type_data_[error::Error::kData][error::data::kPutOutputData] = {"Put Output Data", 400};

  error_type_data_[error::Error::kAlgorithms] = {"Algorithms", {}};
  error_type_data_[error::Error::kAlgorithms][error::algorithms::kExecute] = {"Execute", 400};
}

static ErrorCodeAlgoServer algo_server_error_code;

ErrorTypeData MakeErrorTypeData(const int& error_type) { return algo_server_error_code[error_type]; }

ErrorSubTypeData MakeErrorSubTypeData(const int& error_type, const int& error_sub_type) { return algo_server_error_code[error_type][error_sub_type]; }

std::string MakeServerExceptionMsg(const ServerException& server_exception) {
  std::string msg;

  msg += MakeErrorTypeData(server_exception.error_type).name + ".";
  msg += MakeErrorSubTypeData(server_exception.error_type, server_exception.error_sub_type).name + ".";
  msg += server_exception.exclusive_message;

  return msg;
}

ServerError::ServerError() {
  code.clear();
  message.clear();
  name.clear();
  type.clear();
}

ServerError MakeServerError(const ServerException& server_exception) {
  ServerError server_error;

  {
    std::stringstream code;
    code << std::setw(3) << std::setfill('0') << server_exception.error_type;
    code << ".";
    code << std::setw(3) << std::setfill('0') << server_exception.error_sub_type;
    code << ".";
    code << std::setw(3) << std::setfill('0') << server_exception.exclusive_code;
    server_error.code = code.str();
  }

  server_error.message = server_exception.exclusive_message;
  server_error.name = MakeErrorSubTypeData(server_exception.error_type, server_exception.error_sub_type).name;
  server_error.type = MakeErrorTypeData(server_exception.error_type).name;

  return server_error;
}

void ServerModule::Init(const ModuleInfo& module_info, const std::string& operator_code, const std::string& operator_name) {
  code_ = module_info.system_code + "." + module_info.module_code + "." + operator_code;
  name_ = module_info.module_name + "." + operator_name;
  version_ = module_info.version;
}

std::string ServerModule::GetCode() { return code_; }
std::string ServerModule::GetName() { return name_; }
std::string ServerModule::GetVersion() { return version_; }

}  // namespace algorithm
}  // namespace airouting_2_0
}  // namespace saico
