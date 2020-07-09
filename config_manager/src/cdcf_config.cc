/*
 * Copyright (c) 2019 ThoughtWorks Inc.
 */
#include "../include/cdcf_config.h"

CDCFConfig::CDCFConfig() {
  opt_group{custom_options_, "global"}
      .add(threads_proportion, "threads_proportion", "set threads proportion")
      .add(log_file_, "log-file",
           "set log file name, default: cdcf.log,"
           " format in: /user/cdcf/cdcf.log")
      .add(log_level_, "log-level",
           "set log level, default: info, format in: info")
      .add(log_file_size_in_bytes_, "log-file-size",
           "set maximum rotating log file size in bytes,"
           " default: unlimited, format in: 1024")
      .add(log_file_number_, "log-file-num",
           "set maximum rotating log file number, each file has size"
           " 'log-file-size', default: 0, format in: 1024");
}

CDCFConfig::RetValue CDCFConfig::parse_config(int argc, char** argv,
                                              const char* ini_file_cstr) {
  if (auto err = parse(argc, argv, ini_file_cstr)) {
    std::cerr << "error while parsing CLI and file options: "
              << caf::actor_system_config::render(err) << std::endl;
    return RetValue::kFailure;
  }

  if (cli_helptext_printed) {
    return RetValue::kFailure;
  }

  size_t thread_num =
      floor(std::thread::hardware_concurrency() * threads_proportion);
  set("scheduler.max-threads", std::max(thread_num, 4ul));

  return RetValue::kSuccess;
}

CDCFConfig::RetValue CDCFConfig::parse_config(
    const std::vector<std::string>& args, const char* ini_file_cstr) {
  if (auto err = parse(args, ini_file_cstr)) {
    std::cerr << "error while parsing CLI and file options: "
              << caf::actor_system_config::render(err) << std::endl;
    return RetValue::kFailure;
  }

  if (cli_helptext_printed) {
    return RetValue::kFailure;
  }

  size_t thread_num =
      floor(std::thread::hardware_concurrency() * threads_proportion);
  set("scheduler.max-threads", std::max(thread_num, 4ul));

  return RetValue::kSuccess;
}
