// Copyright 2018 ASLP@NPU.  All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: ASLP@NPU

#include <iostream>
#include <memory>
#include <string>

#include "gflags/gflags.h"
#include "glog/logging.h"
#include "grpc_gateway_demo/serving/server.h"

DEFINE_int32(port, 9090, "Grpc port to listen on for gRPC API");

int main(int argc, char** argv) {
  FLAGS_alsologtostderr = 1;
  FLAGS_colorlogtostderr = true;

  // Initialize Google's logging library.
  google::InitGoogleLogging(argv[0]);

  // Set program usage message.
  auto usage = std::string("usage:") + argv[0] + " [options...]";
  gflags::SetUsageMessage(usage);

  // Parse command line flags.
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  if (argc != 1) {
    LOG(WARNING) << "unknown argument: " << argv[1];
    gflags::ShowUsageWithFlagsRestrict(argv[0], "main");
  }

  grpc_gateway_demo::serving::main::Server::Options options;
  options.grpc_port = FLAGS_port;

  grpc_gateway_demo::serving::main::Server server;
  auto state = server.BuildAndStart(options);
  if (!state) {
    LOG(ERROR) << "Failed to start server.";
    return -1;
  }
  server.WaitForTermination();
  return 0;
}
