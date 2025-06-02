/*
 * @Author: wilson.liu
 * @Date: 2024-03-22 07:08:57
 * @LastEditors: wilson.liu
 * @LastEditTime: 2024-10-01 17:02:53
 * @Description:
 * Copyright (c) 2024 Singularity & Infinity Co., Ltd.
 */

#ifndef SRC_ALGO_SERVER_TOOLS_HPP_
#define SRC_ALGO_SERVER_TOOLS_HPP_

#include <fstream>
#include <string>

#include "absl/log/log_sink.h"
#include "algo_fw/framework.hpp"
#include "algo_server/config.hpp"
#include "aws/core/utils/memory/stl/AWSString.h"
#include "aws/s3/S3ClientConfiguration.h"
#include "redis/redis.hpp"

using saico::algo_fw::Relay;
using saico::redis::Redis;

namespace saico {
namespace tools {

// common
struct RelayExe : public Relay {
  RelayExe();

  std::string ret;
};
RelayExe Exe(const std::string& cmd);

Relay Http(const std::string& url, const std::string& data);
std::string AppendTailSlash(const std::string& dir);
std::string GetDirOfFile(const std::string& file);

struct MemoryUsage : public Relay {
  uint64_t total;
  uint64_t used;
};
MemoryUsage GetMemoryUsage();

// redis
Relay ConnectRedis(const Config& config, Redis* redis, const int& redis_index);
Relay SetRedis(const Config& config, const int& redis_index, const std::string& key, const std::string& data);
Relay GetRedis(const Config& config, const int& redis_index, const std::string& key);
Relay PubRedis(const Config& config, const int& redis_index, const std::string& channel, const std::string& messages);

// aws
Relay InitAws(const Config& config);
Relay GetAwsS3Object(const Aws::String& object_key, const Aws::String& bucket_name, const Aws::S3::S3ClientConfiguration& client_config, const std::string& output_file);
Relay PutAwsS3Object(const Aws::String& object_key, const Aws::String& bucket_name, const Aws::S3::S3ClientConfiguration& client_config, const std::string& input_file);

// data
struct RelayGetInputData : public Relay {
  std::string proto_str;
};
RelayGetInputData GetInputData(const std::string& tmp_dir, const std::string& object_name, const std::string& bucket_name);

struct RelayPutOutputData : public Relay {};
RelayPutOutputData PutOutputData(const std::string& proto_str, const std::string& tmp_dir, const std::string& object_name, const std::string& bucket_name);

// log
class AbslLogSink : public absl::LogSink {
 public:
  explicit AbslLogSink(const std::string& log_dir, const std::string& log_name);
  ~AbslLogSink();

  void Send(const absl::LogEntry& entry) override;

 protected:
  std::string log_dir_;
  std::string log_name_;
  std::string log_name_delimiter_;
  std::string log_datetime_;
  std::ofstream log_;
};

}  // namespace tools
}  // namespace saico

#endif  // SRC_ALGO_SERVER_TOOLS_HPP_
