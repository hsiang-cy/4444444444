/*
 * @Author: wilson.liu
 * @Date: 2024-03-22 07:08:57
 * @LastEditors: wilson.liu
 * @LastEditTime: 2024-09-26 06:48:01
 * @Description:
 * Copyright (c) 2024 Singularity & Infinity Co., Ltd.
 */

#ifndef SRC_ALGO_SERVER_CONFIG_HPP_
#define SRC_ALGO_SERVER_CONFIG_HPP_

#include <string>
#include <vector>

#include "algo_fw/framework.hpp"

using saico::algo_fw::Relay;

namespace saico {
namespace tools {

class Config {
  // key
  static const char* kWebServerPort;
  static const char* kMaxMemoryUsagePct;
  static const char* kLogDir;
  static const char* kHomeDir;

  static const char* kRedisIp;
  static const char* kRedisPort;
  static const char* kRedisTimeout;
  static const char* kRedisAuth;
  static const char* kRedisIndexSet;
  static const char* kRedisIndexPub;
  static const char* kRedisDataTtl;

  static const char* kAwsRegion;
  static const char* kAwsAccessKeyId;
  static const char* kAwsSecretAccessKey;
  static const char* kAwsS3BucketName;

 public:
  Config();
  ~Config();

  std::vector<Relay> ParseEnvVar();
  std::string Dump(int indent = -1) const;

  unsigned int GetWebServerPort() const;
  double GetMaxMemoryUsagePct() const;
  std::string GetLogDir() const;
  std::string GetHomeDir() const;

  std::string GetRedisIp() const;
  unsigned int GetRedisPort() const;
  double GetRedisTimeout() const;
  std::string GetRedisAuth() const;
  unsigned int GetRedisIndexSet() const;
  unsigned int GetRedisIndexPub() const;
  unsigned int GetRedisDataTtl() const;

  std::string GetAwsRegion() const;
  std::string GetAwsAccessKeyId() const;
  std::string GetAwsSecretAccessKey() const;
  std::string GetAwsS3BucketName() const;

 protected:
  // common
  unsigned int web_server_port_;
  double max_memory_usage_pct_;
  std::string log_dir_;
  std::string home_dir_;

  // redis
  std::string redis_ip_;
  unsigned int redis_port_;
  double redis_timeout_;
  std::string redis_auth_;
  unsigned int redis_index_set_;
  unsigned int redis_index_pub_;
  unsigned int redis_data_ttl_;

  // aws s3
  std::string aws_region_;
  std::string aws_access_key_id_;
  std::string aws_secret_access_key_;
  std::string aws_s3_bucket_name_;
};

}  // namespace tools
}  // namespace saico

#endif  // SRC_ALGO_SERVER_CONFIG_HPP_
