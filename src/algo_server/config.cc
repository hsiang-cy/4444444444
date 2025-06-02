/*
 * @Author: wilson.liu
 * @Date: 2024-03-22 07:08:50
 * @LastEditors: wilson.liu
 * @LastEditTime: 2024-10-24 16:47:07
 * @Description:
 * Copyright (c) 2024 Singularity & Infinity Co., Ltd.
 */

#include "algo_server/config.hpp"

#include <algorithm>
#include <fstream>

#include "algo_server/server.hpp"
#include "nlohmann/json.hpp"

namespace saico {
namespace tools {

const char* Config::kWebServerPort = "WEB_SERVER_PORT";
const char* Config::kMaxMemoryUsagePct = "MAX_MEMORY_USAGE_PCT";
const char* Config::kLogDir = "LOG_DIR";
const char* Config::kHomeDir = "HOME";

const char* Config::kRedisIp = "REDIS_IP";
const char* Config::kRedisPort = "REDIS_PORT";
const char* Config::kRedisTimeout = "REDIS_TIMEOUT";
const char* Config::kRedisAuth = "REDIS_AUTH";
const char* Config::kRedisIndexSet = "REDIS_INDEX_SET";
const char* Config::kRedisIndexPub = "REDIS_INDEX_PUB";
const char* Config::kRedisDataTtl = "REDIS_DATA_TTL";

const char* Config::kAwsRegion = "AWS_REGION";
const char* Config::kAwsAccessKeyId = "AWS_ACCESS_KEY_ID";
const char* Config::kAwsSecretAccessKey = "AWS_SECRET_ACCESS_KEY";
const char* Config::kAwsS3BucketName = "AWS_S3_BUCKET_NAME";

Config::Config() {
  web_server_port_ = 4210;
  max_memory_usage_pct_ = 0.8;
  log_dir_.clear();
  home_dir_.clear();

  redis_ip_ = "127.0.0.1";
  redis_port_ = 6379;
  redis_timeout_ = 3.0;
  redis_auth_ = "air2";
  redis_index_set_ = 0;
  redis_index_pub_ = 0;
  redis_data_ttl_ = 10800;

  aws_region_ = "";
  aws_access_key_id_ = "";
  aws_secret_access_key_ = "";
  aws_s3_bucket_name_ = "airouting-dev";
}

Config::~Config() {}

std::vector<Relay> Config::ParseEnvVar() {
  using saico::airouting_2_0::algorithm::error::config::parse::Error;

  std::vector<Relay> relays;
  const char* env_key = nullptr;

  // WEB SERVER PORT
  try {
    env_key = kWebServerPort;
    auto env_val = std::getenv(env_key);
    if (!env_val) {
      throw std::invalid_argument("Does not exist.");
    }

    web_server_port_ = std::stoul(env_val);
  } catch (const std::exception& e) {
    relays.emplace_back();

    Relay& relay = relays.back();
    relay.code = Error::kWebServerPort;
    relay.msg = "Environment variable error, " + std::string(env_key) + ", " + e.what();
  }

  // MAX MEMORY USAGE PCT
  try {
    env_key = kMaxMemoryUsagePct;
    auto env_val = std::getenv(env_key);
    if (!env_val) {
      throw std::invalid_argument("Does not exist.");
    }

    auto max_memory_usage_pct = std::stod(env_val);
    if (max_memory_usage_pct >= 0 && (max_memory_usage_pct < 0.2 || 0.8 < max_memory_usage_pct)) {
      throw std::invalid_argument("The value range is -1 or 0.2 to 0.8.");
    }

    max_memory_usage_pct_ = max_memory_usage_pct;
  } catch (const std::exception& e) {
    relays.emplace_back();

    Relay& relay = relays.back();
    relay.code = Error::kMaxMemoryUsagePct;
    relay.msg = "Environment variable error, " + std::string(env_key) + ", " + e.what();
  }

  // LOG DIR
  try {
    env_key = kLogDir;
    auto env_val = std::getenv(env_key);
    if (env_val) {
      log_dir_ = env_val;

      if (!log_dir_.empty() && log_dir_.back() != '/') {
        log_dir_ += "/";
      }
    }
  } catch (const std::exception& e) {
    relays.emplace_back();

    Relay& relay = relays.back();
    relay.code = Error::kLogDir;
    relay.msg = "Environment variable error, " + std::string(env_key) + ", " + e.what();
  }

  // HOME DIR
  try {
    env_key = kHomeDir;
    auto env_val = std::getenv(env_key);
    if (!env_val) {
      throw std::invalid_argument("Does not exist.");
    }

    home_dir_ = env_val;
  } catch (const std::exception& e) {
    relays.emplace_back();

    Relay& relay = relays.back();
    relay.code = Error::kHomeDir;
    relay.msg = "Environment variable error, " + std::string(env_key) + ", " + e.what();
  }

  // REDIS IP
  try {
    env_key = kRedisIp;
    auto env_val = std::getenv(env_key);
    if (!env_val) {
      throw std::invalid_argument("Does not exist.");
    }

    redis_ip_ = env_val;
  } catch (const std::exception& e) {
    relays.emplace_back();

    Relay& relay = relays.back();
    relay.code = Error::kRedisIp;
    relay.msg = "Environment variable error, " + std::string(env_key) + ", " + e.what();
  }

  // REDIS PORT
  try {
    env_key = kRedisPort;
    auto env_val = std::getenv(env_key);
    if (!env_val) {
      throw std::invalid_argument("Does not exist.");
    }

    redis_port_ = std::stoul(env_val);
  } catch (const std::exception& e) {
    relays.emplace_back();

    Relay& relay = relays.back();
    relay.code = Error::kRedisPort;
    relay.msg = "Environment variable error, " + std::string(env_key) + ", " + e.what();
  }

  // REDIS TIMEOUT
  try {
    env_key = kRedisTimeout;
    auto env_val = std::getenv(env_key);
    if (!env_val) {
      throw std::invalid_argument("Does not exist.");
    }

    auto redis_timeout = std::stod(env_val);
    if (redis_timeout < 0.1) {
      throw std::invalid_argument("The value range is greater than or equal to 0.1.");
    }

    redis_timeout_ = redis_timeout;
  } catch (const std::exception& e) {
    relays.emplace_back();

    Relay& relay = relays.back();
    relay.code = Error::kRedisTimeout;
    relay.msg = "Environment variable error, " + std::string(env_key) + ", " + e.what();
  }

  // REDIS AUTH
  try {
    env_key = kRedisAuth;
    auto env_val = std::getenv(env_key);
    if (!env_val) {
      throw std::invalid_argument("Does not exist.");
    }

    redis_auth_ = env_val;
  } catch (const std::exception& e) {
    relays.emplace_back();

    Relay& relay = relays.back();
    relay.code = Error::kRedisAuth;
    relay.msg = "Environment variable error, " + std::string(env_key) + ", " + e.what();
  }

  // REDIS INDEX SET
  try {
    env_key = kRedisIndexSet;
    auto env_val = std::getenv(env_key);
    if (!env_val) {
      throw std::invalid_argument("Does not exist.");
    }

    redis_index_set_ = std::stoul(env_val);
  } catch (const std::exception& e) {
    relays.emplace_back();

    Relay& relay = relays.back();
    relay.code = Error::kRedisIndexSet;
    relay.msg = "Environment variable error, " + std::string(env_key) + ", " + e.what();
  }

  // REDIS INDEX PUB
  try {
    env_key = kRedisIndexPub;
    auto env_val = std::getenv(env_key);
    if (!env_val) {
      throw std::invalid_argument("Does not exist.");
    }

    redis_index_pub_ = std::stoul(env_val);
  } catch (const std::exception& e) {
    relays.emplace_back();

    Relay& relay = relays.back();
    relay.code = Error::kRedisIndexPub;
    relay.msg = "Environment variable error, " + std::string(env_key) + ", " + e.what();
  }

  // REDIS DATA TTL
  try {
    env_key = kRedisDataTtl;
    auto env_val = std::getenv(env_key);
    if (!env_val) {
      throw std::invalid_argument("Does not exist.");
    }

    redis_data_ttl_ = std::stoul(env_val);
  } catch (const std::exception& e) {
    relays.emplace_back();

    Relay& relay = relays.back();
    relay.code = Error::kRedisDataTtl;
    relay.msg = "Environment variable error, " + std::string(env_key) + ", " + e.what();
  }

  // AWS REGION
  try {
    env_key = kAwsRegion;
    auto env_val = std::getenv(env_key);
    if (!env_val) {
      throw std::invalid_argument("Does not exist.");
    }

    aws_region_ = env_val;
  } catch (const std::exception& e) {
    relays.emplace_back();

    Relay& relay = relays.back();
    relay.code = Error::kAwsRegion;
    relay.msg = "Environment variable error, " + std::string(env_key) + ", " + e.what();
  }

  // AWS ACCESS KEY ID
  try {
    env_key = kAwsAccessKeyId;
    auto env_val = std::getenv(env_key);
    if (!env_val) {
      throw std::invalid_argument("Does not exist.");
    }

    aws_access_key_id_ = env_val;
  } catch (const std::exception& e) {
    relays.emplace_back();

    Relay& relay = relays.back();
    relay.code = Error::kAwsAccessKeyId;
    relay.msg = "Environment variable error, " + std::string(env_key) + ", " + e.what();
  }

  // AWS SECRET ACCESS KEY
  try {
    env_key = kAwsSecretAccessKey;
    auto env_val = std::getenv(env_key);
    if (!env_val) {
      throw std::invalid_argument("Does not exist.");
    }

    aws_secret_access_key_ = env_val;
  } catch (const std::exception& e) {
    relays.emplace_back();

    Relay& relay = relays.back();
    relay.code = Error::kAwsSecretAccessKey;
    relay.msg = "Environment variable error, " + std::string(env_key) + ", " + e.what();
  }

  // AWS S3 BUCKET NAME
  try {
    env_key = kAwsS3BucketName;
    auto env_val = std::getenv(env_key);
    if (!env_val) {
      throw std::invalid_argument("Does not exist.");
    }

    aws_s3_bucket_name_ = env_val;
  } catch (const std::exception& e) {
    relays.emplace_back();

    Relay& relay = relays.back();
    relay.code = Error::kAwsS3BucketName;
    relay.msg = "Environment variable error, " + std::string(env_key) + ", " + e.what();
  }

  return relays;
}

std::string Config::Dump(int indent) const {
  nlohmann::json ret = {{kWebServerPort, web_server_port_},
                        {kMaxMemoryUsagePct, max_memory_usage_pct_},
                        {kLogDir, log_dir_},
                        {kHomeDir, home_dir_},
                        // {kRedisIp, redis_ip_},
                        {kRedisPort, redis_port_},
                        {kRedisTimeout, redis_timeout_},
                        // {kRedisAuth, redis_auth_},
                        {kRedisIndexSet, redis_index_set_},
                        {kRedisIndexPub, redis_index_pub_},
                        {kRedisDataTtl, redis_data_ttl_},
                        // {kAwsRegion, aws_region_},
                        // {kAwsAccessKeyId, aws_access_key_id_},
                        // {kAwsSecretAccessKey, aws_secret_access_key_},
                        // {kAwsS3BucketName, aws_s3_bucket_name_}
                        };
  return ret.dump(indent);
}

unsigned int Config::GetWebServerPort() const { return web_server_port_; }
double Config::GetMaxMemoryUsagePct() const { return max_memory_usage_pct_; }
std::string Config::GetLogDir() const { return log_dir_; }
std::string Config::GetHomeDir() const { return home_dir_; }

std::string Config::GetRedisIp() const { return redis_ip_; }
unsigned int Config::GetRedisPort() const { return redis_port_; }
double Config::GetRedisTimeout() const { return redis_timeout_; }
std::string Config::GetRedisAuth() const { return redis_auth_; }
unsigned int Config::GetRedisIndexSet() const { return redis_index_set_; }
unsigned int Config::GetRedisIndexPub() const { return redis_index_pub_; }
unsigned int Config::GetRedisDataTtl() const { return redis_data_ttl_; }

std::string Config::GetAwsRegion() const { return aws_region_; }
std::string Config::GetAwsAccessKeyId() const { return aws_access_key_id_; }
std::string Config::GetAwsSecretAccessKey() const { return aws_secret_access_key_; }
std::string Config::GetAwsS3BucketName() const { return aws_s3_bucket_name_; }

}  // namespace tools
}  // namespace saico
