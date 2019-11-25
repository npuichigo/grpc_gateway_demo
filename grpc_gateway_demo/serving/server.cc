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

#include "grpc_gateway_demo/serving/server.h"

#include <memory>
#include <string>

#include "glog/logging.h"
#include "grpc/grpc.h"
#include "grpcpp/security/server_credentials.h"
#include "grpcpp/server_builder.h"
#include "grpcpp/server_context.h"
#include "grpcpp/support/status.h"

namespace grpc_gateway_demo {
namespace serving {
namespace main {

const int kint32max = ((int)0x7FFFFFFF);

Server::~Server() { WaitForTermination(); }

bool Server::BuildAndStart(const Options& server_options) {
  if (server_options.grpc_port == 0) {
    LOG(ERROR) << "server_options.grpc_port is not set.";
    return false;
  }
  // 0.0.0.0" is the way to listen on localhost in gRPC.
  const std::string server_address =
      "0.0.0.0:" + std::to_string(server_options.grpc_port);

  demo_service_.reset(new DemoServiceImpl());
  ::grpc::ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register service through which we'll communicate with clients
  builder.RegisterService(demo_service_.get());
  builder.SetMaxMessageSize(kint32max);
  grpc_server_ = builder.BuildAndStart();
  if (grpc_server_ == nullptr) {
    LOG(ERROR) << "Failed to BuildAndStart gRPC server";
    return false;
  }
  LOG(INFO) << "Running gRPC Server at " << server_address << " ...";
  return true;
}

void Server::WaitForTermination() {
  if (grpc_server_ != nullptr) {
    grpc_server_->Wait();
  }
}

}  // namespace main
}  // namespace serving
}  // namespace grpc_gateway_demo
