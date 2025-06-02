/*
 * @Author: wilson.liu
 * @Date: 2024-03-22 07:08:50
 * @LastEditors: wilson.liu
 * @LastEditTime: 2024-10-01 17:02:47
 * @Description:
 * Copyright (c) 2024 Singularity & Infinity Co., Ltd.
 */

#include "algo_server/tools.hpp"

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <sys/sysinfo.h>
#include <sys/types.h>

#include <memory>

#include "absl/log/log.h"
#include "algo_server/server.hpp"

namespace saico {
namespace tools {

RelayExe::RelayExe() { ret.clear(); }

RelayExe Exe(const std::string& cmd) {
  RelayExe relay;

  std::array<char, 128> buffer;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);

  if (!pipe) {
    relay.code = -1;
    relay.msg = "popen() failed!";
    return relay;
  }

  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    relay.ret += buffer.data();
  }

  return relay;
}

Relay Http(const std::string& url, const std::string& data) {
  Relay relay;

  LOG(INFO) << "HTTP:";

  std::string cmd = "/usr/bin/curl --insecure -X POST \"" + url + "\" -H \"accept: application/json\" -H \"Content-Type: application/json\" -d '" + data + "'";
  LOG(INFO) << "    req: " + cmd;

  auto relay_exe = Exe(cmd);
  if (relay_exe.code) {
    LOG(ERROR) << relay_exe.msg;

    relay.code = -1;
    relay.msg = relay_exe.msg;
    return relay;
  }

  LOG(INFO) << "    res: " + relay_exe.ret;

  return relay;
}

std::string AppendTailSlash(const std::string& dir) {
  if (dir.empty()) {
    return "/";
  }

  if (dir.back() == '/') {
    return dir;
  }

  return dir + "/";
}

std::string GetDirOfFile(const std::string& file) {
  if (file.empty()) {
    return "";
  }

  auto ret_find = file.rfind('/');
  if (ret_find == std::string::npos) {
    return file;
  }

  return file.substr(0, ret_find);
}

MemoryUsage GetMemoryUsage() {
  MemoryUsage ret;

  // get memory cmd
  std::string cmd = "free -b | grep Mem | awk '{print $2, $3}'";
  auto relay_exe = Exe(cmd);
  if (relay_exe.code) {
    ret.code = -1;
    ret.msg = "free cmd error";
    return ret;
  }

  // get value
  static const std::string kNumbers = "0123456789";
  size_t ret_num_head = 0;
  size_t ret_num_tail = 0;
  for (size_t i = 0; i < 2; i++) {
    // find head
    ret_num_head = relay_exe.ret.find_first_of(kNumbers, ret_num_head);
    if (ret_num_head == std::string::npos) {
      ret.code = -2;
      ret.msg = "can not find head of number";
      return ret;
    }

    // find tail
    ret_num_tail = relay_exe.ret.find_first_not_of(kNumbers, ret_num_head);
    if (ret_num_head == std::string::npos) {
      ret.code = -3;
      ret.msg = "can not find tail of number";
      return ret;
    }

    // str to int
    uint64_t* number = nullptr;
    if (i == 0) {
      number = &ret.total;
    } else if (i == 1) {
      number = &ret.used;
    }

    if (number != nullptr) {
      *number = std::stoull(relay_exe.ret.substr(ret_num_head, ret_num_tail - ret_num_head));
    }

    // update head
    ret_num_head = ret_num_tail;
  }

  return ret;
}

Relay ConnectRedis(const Config& config, Redis* redis, const int& redis_index) {
  using saico::airouting_2_0::algorithm::error::redis::connect::Error;

  Relay relay;

  auto relay_connect = redis->Connect(config.GetRedisIp(), config.GetRedisPort(), config.GetRedisTimeout());
  if (relay_connect.code) {
    relay.code = Error::kConnect;
    relay.msg = "Redis Connect Error: " + relay_connect.msg;
    return relay;
  }

  auto relay_auth = redis->Auth(config.GetRedisAuth());
  if (relay_auth.code) {
    relay.code = Error::kAuth;
    relay.msg = "Redis Auth Error: " + relay_auth.msg;
    return relay;
  }

  auto relay_select = redis->Select(redis_index);
  if (relay_select.code) {
    relay.code = Error::kSelect;
    relay.msg = "Redis Select Error: " + relay_select.msg;
    return relay;
  }

  return relay;
}

Relay SetRedis(const Config& config, const int& redis_index, const std::string& key, const std::string& data) {
  using saico::airouting_2_0::algorithm::error::redis::set::Error;

  Relay relay;

  Redis redis;
  auto relay_connect = ConnectRedis(config, &redis, redis_index);
  if (relay_connect.code) {
    return relay_connect;
  }

  auto relay_set = redis.Set(key, data, config.GetRedisDataTtl());
  if (relay_set.code) {
    relay.code = Error::kSet;
    relay.msg = "Redis Set Error: " + relay_set.msg;
    return relay;
  }

  return relay;
}

Relay GetRedis(const Config& config, const int& redis_index, const std::string& key) {
  using saico::airouting_2_0::algorithm::error::redis::get::Error;

  Relay relay;

  Redis redis;
  auto relay_connect = ConnectRedis(config, &redis, redis_index);
  if (relay_connect.code) {
    return relay_connect;
  }

  relay = redis.Get(key);
  if (relay.msg.empty()) {
    relay.code = Error::kGet;
    relay.msg = "Redis Get Error: " + key;
    return relay;
  }

  return relay;
}

Relay PubRedis(const Config& config, const int& redis_index, const std::string& channel, const std::string& messages) {
  using saico::airouting_2_0::algorithm::error::redis::pub::Error;

  Relay relay;

  Redis redis;
  auto relay_connect = ConnectRedis(config, &redis, redis_index);
  if (relay_connect.code) {
    return relay_connect;
  }

  relay = redis.Publish(channel, messages);
  if (!relay.msg.empty()) {
    relay.code = Error::kPub;
    relay.msg = "Redis Publish Error: " + channel;
    return relay;
  }

  return relay;
}

Relay InitAws(const Config& config) {
  using saico::airouting_2_0::algorithm::error::aws::init::Error;

  Relay relay;

  std::string aws_dir = AppendTailSlash(config.GetHomeDir()) + ".aws";

  // make dir
  {
    if (!std::filesystem::exists(aws_dir) && !std::filesystem::create_directories(aws_dir)) {
      relay.code = Error::kCreateTemporaryDirectory;
      relay.msg = "Failed to create temporary directory";
      return relay;
    }

    if (!std::filesystem::is_directory(aws_dir)) {
      relay.code = Error::kTemporaryDirectoryType;
      relay.msg = "Temporary directory type error";
      return relay;
    }
  }

  // init config
  {
    std::string file = "config";
    std::ofstream output_stream(AppendTailSlash(aws_dir) + file, std::ios_base::out | std::ios::binary);
    if (!output_stream.is_open()) {
      relay.code = Error::kCreateAwsConfig;
      relay.msg = "Failed to create file " + file;
      return relay;
    }

    output_stream << "[default]" << std::endl;
    output_stream << "region = ap-southeast-1" << std::endl;
    output_stream.close();
  }

  // init credentials
  {
    std::string file = "credentials";
    std::ofstream output_stream(AppendTailSlash(aws_dir) + file, std::ios_base::out | std::ios::binary);
    if (!output_stream.is_open()) {
      relay.code = Error::kCreateAwsCredentials;
      relay.msg = "Failed to create file " + file;
      return relay;
    }

    output_stream << "[default]" << std::endl;
    output_stream << "aws_access_key_id = " << config.GetAwsAccessKeyId() << std::endl;
    output_stream << "aws_secret_access_key = " << config.GetAwsSecretAccessKey() << std::endl;
    output_stream.close();
  }

  return relay;
}

Relay GetAwsS3Object(const Aws::String& object_key, const Aws::String& bucket_name, const Aws::S3::S3ClientConfiguration& client_config, const std::string& output_file) {
  using saico::airouting_2_0::algorithm::error::aws::get_s3_object::Error;

  Relay relay;

  Aws::S3::S3Client client(client_config);

  Aws::S3::Model::GetObjectRequest request;
  request.SetBucket(bucket_name);
  request.SetKey(object_key);

  Aws::S3::Model::GetObjectOutcome outcome = client.GetObject(request);

  if (!outcome.IsSuccess()) {
    const Aws::S3::S3Error& err = outcome.GetError();
    relay.code = Error::kGetObject;
    relay.msg = err.GetExceptionName() + ", " + err.GetMessage();
    return relay;
  }

  auto& retrieved_file = outcome.GetResultWithOwnership().GetBody();

  std::ofstream output_stream(output_file, std::ios_base::out | std::ios::binary);
  if (!output_stream.is_open()) {
    relay.code = Error::kCreateOutputFile;
    relay.msg = "Failed to open file " + output_file + ".";
    return relay;
  }

  output_stream << retrieved_file.rdbuf();
  output_stream.close();

  return relay;
}

Relay PutAwsS3Object(const Aws::String& object_key, const Aws::String& bucket_name, const Aws::S3::S3ClientConfiguration& client_config, const std::string& input_file) {
  using saico::airouting_2_0::algorithm::error::aws::put_s3_object::Error;

  Relay relay;

  Aws::S3::S3Client s3Client(client_config);

  Aws::S3::Model::PutObjectRequest request;
  request.SetBucket(bucket_name);
  request.SetKey(object_key);
  request.SetContentEncoding("gzip");
  request.SetContentType("application/octet-stream");

  std::shared_ptr<Aws::IOStream> input_stream = Aws::MakeShared<Aws::FStream>(object_key.c_str(), input_file.c_str(), std::ios_base::in | std::ios_base::binary);

  if (!*input_stream) {
    relay.code = Error::kReadInputFile;
    relay.msg = "Failed to read file " + input_file + ".";
    return relay;
  }

  request.SetBody(input_stream);

  Aws::S3::Model::PutObjectOutcome outcome = s3Client.PutObject(request);

  if (!outcome.IsSuccess()) {
    const Aws::S3::S3Error& err = outcome.GetError();
    relay.code = Error::kPutObject;
    relay.msg = err.GetExceptionName() + ", " + err.GetMessage();
    return relay;
  }

  return relay;
}

RelayGetInputData GetInputData(const std::string& tmp_dir, const std::string& object_name, const std::string& bucket_name) {
  using saico::airouting_2_0::algorithm::error::data::get_input_data::Error;

  RelayGetInputData relay;

  // get path
  std::string gz_file = AppendTailSlash(tmp_dir) + object_name;
  std::string gz_dir = GetDirOfFile(gz_file);

  // make tmp dir
  {
    if (!std::filesystem::exists(gz_dir) && !std::filesystem::create_directories(gz_dir)) {
      relay.code = Error::kCreateTemporaryDirectory;
      relay.msg = "Failed to create temporary directory";
      return relay;
    }

    if (!std::filesystem::is_directory(gz_dir)) {
      relay.code = Error::kTemporaryDirectoryType;
      relay.msg = "Temporary directory type error";
      return relay;
    }
  }

  // get aws s3 object
  {
    Aws::SDKOptions options;
    Aws::InitAPI(options);

    Aws::S3::S3ClientConfiguration clientConfig;

    auto relay_get_aws_s3_object = GetAwsS3Object(object_name, bucket_name, clientConfig, gz_file);
    if (relay_get_aws_s3_object.code) {
      relay.code = relay_get_aws_s3_object.code;
      relay.msg = relay_get_aws_s3_object.msg;
      return relay;
    }

    Aws::ShutdownAPI(options);
  }

  // gunzip file gz
  std::string bin_file = gz_file + ".bin";
  {
    if (std::filesystem::exists(bin_file) && !std::filesystem::remove_all(bin_file)) {
      relay.code = Error::kFileBinExists;
      relay.msg = "File bin already exists";
      return relay;
    }

    std::string cmd = "gunzip -c " + gz_file + " > " + bin_file;
    auto ret = system(cmd.c_str());
    if (ret) {
      relay.code = Error::kGunzip;
      relay.msg = "gunzip failed";
      return relay;
    }
  }

  // bin to proto
  {
    std::ifstream input_stream(bin_file, std::ios::in | std::ios::binary);

    if (!input_stream.is_open()) {
      relay.code = Error::kReadBinFile;
      relay.msg = "Failed to read bin file";
      return relay;
    }

    std::stringstream ss;
    ss << input_stream.rdbuf();

    relay.proto_str = ss.str();
  }

  // clear data
  {
    if (std::filesystem::exists(tmp_dir) && !std::filesystem::remove_all(tmp_dir)) {
      relay.code = Error::kClearTemporaryDirectory;
      relay.msg = "Failed to create temporary directory";
      return relay;
    }
  }

  return relay;
}

RelayPutOutputData PutOutputData(const std::string& proto_str, const std::string& tmp_dir, const std::string& object_name, const std::string& bucket_name) {
  using saico::airouting_2_0::algorithm::error::data::put_output_data::Error;

  RelayPutOutputData relay;

  // get path
  std::string gz_file = AppendTailSlash(tmp_dir) + object_name;
  std::string gz_dir = GetDirOfFile(gz_file);

  // make tmp dir
  {
    if (!std::filesystem::exists(gz_dir) && !std::filesystem::create_directories(gz_dir)) {
      relay.code = Error::kCreateTemporaryDirectory;
      relay.msg = "Failed to create temporary directory";
      return relay;
    }

    if (!std::filesystem::is_directory(gz_dir)) {
      relay.code = Error::kTemporaryDirectoryType;
      relay.msg = "Temporary directory type error";
      return relay;
    }
  }

  // proto to bin
  std::string bin_file = gz_file + ".bin";
  {
    std::ofstream output_stream(bin_file, std::ios::out | std::ios::binary);

    if (!output_stream.is_open()) {
      relay.code = Error::kWriteBinFile;
      relay.msg = "Failed to write bin file";
      return relay;
    }

    output_stream << proto_str;
    output_stream.close();
  }

  // gunzip file gz
  {
    if (std::filesystem::exists(gz_file) && !std::filesystem::remove_all(gz_file)) {
      relay.code = Error::kFileGzExists;
      relay.msg = "File bin already exists";
      return relay;
    }

    std::string cmd = "gzip -c " + bin_file + " > " + gz_file;
    auto ret = system(cmd.c_str());
    if (ret) {
      relay.code = Error::kGzip;
      relay.msg = "gunzip failed";
      return relay;
    }
  }

  // upload file gz to aws s3
  {
    Aws::SDKOptions options;
    Aws::InitAPI(options);

    Aws::S3::S3ClientConfiguration clientConfig;

    auto relay_get_aws_s3_object = PutAwsS3Object(object_name, bucket_name, clientConfig, gz_file);
    if (relay_get_aws_s3_object.code) {
      relay.code = relay_get_aws_s3_object.code;
      relay.msg = relay_get_aws_s3_object.msg;
      return relay;
    }

    Aws::ShutdownAPI(options);
  }

  // clear data
  {
    if (std::filesystem::exists(tmp_dir) && !std::filesystem::remove_all(tmp_dir)) {
      relay.code = Error::kClearTemporaryDirectory;
      relay.msg = "Failed to create temporary directory";
      return relay;
    }
  }

  return relay;
}

AbslLogSink::AbslLogSink(const std::string& log_dir, const std::string& log_name) {
  // log dir
  if (log_dir.empty()) {
    log_dir_ = "logs/";

  } else {
    log_dir_ = log_dir;
    if (log_dir_.back() != '/') {
      log_dir_ += "/";
    }
  }

  // log name
  if (log_name.empty()) {
    log_name_ = "log";
  } else {
    size_t ret = log_name.rfind('/');
    if (ret == std::string::npos) {
      log_name_ = log_name;
    } else {
      log_name_ = log_name.substr(ret + 1);
    }
  }

  // log name delimiter_
  log_name_delimiter_ = "-";
}

AbslLogSink::~AbslLogSink() { log_.close(); }

void AbslLogSink::Send(const absl::LogEntry& entry) {
  // get current datetime
  std::string datetime;
  {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d");
    if (oss.fail()) {
      return;
    }

    datetime = oss.str();
  }

  // get time stamp limit
  std::time_t time_stamp_limit;
  {
    std::tm tm{};
    std::istringstream iss(datetime + "000000");
    iss >> std::get_time(&tm, "%Y%m%d%H%M%S");
    if (iss.fail()) {
      return;
    }

    time_stamp_limit = mktime(&tm);
    time_stamp_limit -= (60 * 60 * 24) * 30;
  }

  // clear expired files
  for (const auto& dir_entry : std::filesystem::directory_iterator(log_dir_)) {
    std::string dir_entry_path = dir_entry.path();

    size_t ret_find = dir_entry_path.find(log_dir_ + log_name_);
    if (ret_find == std::string::npos) {
      continue;
    }

    // get dir entry date
    std::string dir_entry_date = dir_entry_path.substr(log_dir_.size() + log_name_delimiter_.size() + log_name_.size());

    // get dir entry time stamp
    std::time_t time_stamp;
    {
      std::tm tm{};
      std::istringstream iss(dir_entry_date + "000000");
      iss >> std::get_time(&tm, "%Y%m%d%H%M%S");
      if (iss.fail()) {
        continue;
      }

      time_stamp = mktime(&tm);
    }

    // remove file
    if (time_stamp <= time_stamp_limit) {
      if (std::remove(std::string(dir_entry.path()).c_str())) {
        return;
      }
    }
  }

  // check whether the log file needs to be re-created
  if (log_.is_open() && log_datetime_ != datetime) {
    log_.close();
  }

  // create log file
  if (!log_.is_open()) {
    std::string log_full_path = log_dir_ + log_name_ + log_name_delimiter_ + datetime;

    // create log file directory
    {
      std::string cmd = "mkdir -p " + log_dir_;
      if (system(cmd.c_str())) {
        return;
      }
    }

    // record datetime
    log_datetime_ = datetime;

    // create log file
    log_.open(log_full_path, std::ios::out | std::ios::app);
    if (!log_.is_open()) {
      return;
    }
  }

  // write log
  log_ << entry.text_message_with_prefix_and_newline_c_str();
  log_.flush();
}

}  // namespace tools
}  // namespace saico
