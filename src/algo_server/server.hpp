/*
 * @Author: wilson.liu
 * @Date: 2024-03-22 07:08:57
 * @LastEditors: wilson.liu
 * @LastEditTime: 2024-09-27 07:57:40
 * @Description:
 * Copyright (c) 2024 Singularity & Infinity Co., Ltd.
 */

#ifndef SRC_ALGO_SERVER_SERVER_HPP_
#define SRC_ALGO_SERVER_SERVER_HPP_

#include <map>
#include <string>

#include "server/server.hpp"

using saico::server::ErrorCodeServer;
using saico::server::ErrorSubTypeData;
using saico::server::ErrorTypeData;
using saico::server::ModuleInfo;
using saico::server::ServerException;

namespace saico {
namespace airouting_2_0 {
namespace algorithm {

namespace error {

/**
 * Error Type
 */
enum Error {
  kGeneral = 101,
  kConfig,
  kTaskManager,
  kRedis,
  kAws,
  kData,
  kAlgorithms,
};

/**
 * Error Sub Type
 */
namespace general {
enum Error {
  kUnexpected = 1,
  kSegmentationFault,
};

/**
 * Error Exclusive Type
 */
namespace unexpected {
enum Error {
  kUnexpected = 1,
};
}  // namespace unexpected

/**
 * Error Exclusive Type
 */
namespace segmentation_fault {
enum Error {
  kSegmentationFault = 1,
};
}  // namespace segmentation_fault

}  // namespace general

/**
 * Error Sub Type
 */
namespace config {
enum Error {
  kParse = 1,
};

/**
 * Error Exclusive Type
 */
namespace parse {
enum Error {
  kWebServerPort = 1,
  kMaxMemoryUsagePct,
  kLogDir,
  kHomeDir,
  kRedisIp,
  kRedisPort,
  kRedisTimeout,
  kRedisAuth,
  kRedisIndexSet,
  kRedisIndexPub,
  kRedisDataTtl,
  kAwsRegion,
  kAwsAccessKeyId,
  kAwsSecretAccessKey,
  kAwsS3BucketName,
  kUnexpected,
};
}  // namespace parse

}  // namespace config

/**
 * Error Sub Type
 */
namespace task_manager {
enum Error {
  kAdd = 1,
};

/**
 * Error Exclusive Type
 */
namespace add {
enum Error {
  kRepeatedTask = 1,
  kGetMemoryUsage,
  kNotEnoughMemory,
};
}  // namespace add

}  // namespace task_manager

/**
 * Error Sub Type
 */
namespace redis {
enum Error {
  kConnect = 1,
  kSet,
  kGet,
  kSub,
  kPub,
};

/**
 * Error Exclusive Type
 */
namespace connect {
enum Error {
  kConnect = 1,
  kAuth,
  kSelect,
};
}  // namespace connect

/**
 * Error Exclusive Type
 */
namespace set {
enum Error {
  kSet = 1,
};
}  // namespace set

/**
 * Error Exclusive Type
 */
namespace get {
enum Error {
  kGet = 1,
};
}  // namespace get

/**
 * Error Exclusive Type
 */
namespace sub {
enum Error {
  kSub = 1,
};
}  // namespace sub

/**
 * Error Exclusive Type
 */
namespace pub {
enum Error {
  kPub = 1,
};
}  // namespace pub

}  // namespace redis

/**
 * Error Sub Type
 */
namespace aws {
enum Error {
  kInit = 1,
  kGetS3Object,
  kPutS3Object,
};

/**
 * Error Exclusive Type
 */
namespace init {
enum Error {
  kCreateTemporaryDirectory = 1,
  kTemporaryDirectoryType,
  kCreateAwsConfig,
  kCreateAwsCredentials,
};
}  // namespace init

/**
 * Error Exclusive Type
 */
namespace get_s3_object {
enum Error {
  kGetObject = 1,
  kCreateOutputFile,
};
}  // namespace get_s3_object

/**
 * Error Exclusive Type
 */
namespace put_s3_object {
enum Error {
  kReadInputFile = 1,
  kPutObject,
};
}  // namespace put_s3_object

}  // namespace aws

/**
 * Error Sub Type
 */
namespace data {
enum Error {
  kGetInputData = 1,
  kParseOutput,
  kSerializeOutput,
  kPutOutputData,
};

/**
 * Error Exclusive Type
 */
namespace get_input_data {
enum Error {
  kCreateTemporaryDirectory = 1,
  kTemporaryDirectoryType,
  kFileBinExists,
  kGunzip,
  kReadBinFile,
  kClearTemporaryDirectory,
};
}  // namespace get_input_data

/**
 * Error Exclusive Type
 */
namespace parse_output {
enum Error {
  kParseOutput = 1,
};
}  // namespace parse_output

/**
 * Error Exclusive Type
 */
namespace serialize_output {
enum Error {
  kSerializeOutput = 1,
};
}  // namespace serialize_output

/**
 * Error Exclusive Type
 */
namespace put_output_data {
enum Error {
  kCreateTemporaryDirectory = 1,
  kTemporaryDirectoryType,
  kWriteBinFile,
  kFileGzExists,
  kGzip,
  kClearTemporaryDirectory,
};
}  // namespace put_output_data

}  // namespace data

/**
 * Error Sub Type
 */
namespace algorithms {
enum Error {
  kExecute = 1,
};

/**
 * Error Exclusive Type
 */
namespace execute {
enum Error {
  kAirAlgorithm = 1,
};
}  // namespace execute

}  // namespace algorithms

/**
 * Error Sub Type
 */
namespace input_empty {
enum Error {
  kInputEmpty = 1,
};

/**
 * Error Exclusive Type Of Server
 */
namespace input_empty {
enum Error {
  kTaskId = 1,
  kS3Dir,
};
}  // namespace input_empty

}  // namespace input_empty

}  // namespace error

/**
 * Algorithm Server
 */
extern const char* kContent;
extern const char* kTaskId;
extern const char* kS3Dir;
extern const char* kIterations;
extern const char* kData;
extern const char* kError;

extern const char* kContentType;

extern const char* kAwsS3TmpDir;
extern const char* kAwsS3InputFile;
extern const char* kAwsS3OutputFile;

// redis
//// channel set
extern const char* kStatus;
// extern const char* kError;

//// channel pub
extern const char* kAlgoStatus;
extern const char* kAlgoError;

std::string GetRedisKey(const std::string& task_id, const std::string& key);

/**
 * Error Code Algo Server
 */
class ErrorCodeAlgoServer : public ErrorCodeServer {
 public:
  ErrorCodeAlgoServer();
};

ErrorTypeData MakeErrorTypeData(const int& error_type);
ErrorSubTypeData MakeErrorSubTypeData(const int& error_type, const int& error_sub_type);

std::string MakeServerExceptionMsg(const ServerException& server_exception);

struct ServerError {
  std::string code;
  std::string message;
  std::string name;
  std::string type;

  ServerError();
};
ServerError MakeServerError(const ServerException& server_exception);
#define COPY_ERROR(from, to)     \
  to->set_code(from.code);       \
  to->set_message(from.message); \
  to->set_name(from.name);       \
  to->set_type(from.type);

class ServerModule {
 public:
  void Init(const ModuleInfo& module_info, const std::string& operator_code, const std::string& operator_name);
  std::string GetCode();
  std::string GetName();
  std::string GetVersion();

 protected:
  std::string code_;
  std::string name_;
  std::string version_;
};

}  // namespace algorithm
}  // namespace airouting_2_0
}  // namespace saico

#endif  // SRC_ALGO_SERVER_SERVER_HPP_
