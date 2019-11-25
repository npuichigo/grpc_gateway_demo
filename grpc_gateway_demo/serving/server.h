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

#ifndef GRPC_GATEWAY_DEMO_SERVING_SERVER_H_
#define GRPC_GATEWAY_DEMO_SERVING_SERVER_H_

#include <memory>

#include "grpcpp/server.h"
#include "grpc_gateway_demo/serving/demo_service_impl.h"

namespace grpc_gateway_demo {
namespace serving {
namespace main {

class Server {
 public:
  struct Options {
    // gRPC Server options.
    int grpc_port = 9090;
  };

  // Blocks the current thread waiting for servers (if any)
  // started as part of BuildAndStart() call.
  ~Server();

  // Build and start gRPC server, to be ready to accept and
  // process new requests over gRPC.
  bool BuildAndStart(const Options& server_options);

  // Wait for servers started in BuildAndStart() above to terminate.
  // This will block the current thread until termination is successful.
  void WaitForTermination();

 private:
  std::unique_ptr<DemoServiceImpl> demo_service_;
  std::unique_ptr<::grpc::Server> grpc_server_;
};

}  // namespace main
}  // namespace serving
}  // namespace grpc_gateway_demo

#endif  // GRPC_GATEWAY_DEMO_SERVING_SERVER_H_
