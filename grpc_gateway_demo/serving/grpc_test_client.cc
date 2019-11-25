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

#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "gflags/gflags.h"
#include "glog/logging.h"
#include "grpcpp/grpcpp.h"
#include "grpc_gateway_demo/serving/demo_service_impl.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;
using grpc_gateway_demo::serving::GetRequest;
using grpc_gateway_demo::serving::Demo;

DEFINE_string(address, "0.0.0.0:9090", "Address of gRPC API");
DEFINE_string(filename, "testdata/small.wav", "Name of file to get");

class DemoClient {
 public:
  DemoClient(std::shared_ptr<Channel> channel)
      : stub_(Demo::NewStub(channel)) {}

  void GetSomething(const std::string& filename) {
    // Data we are sending to the server.
    GetRequest request;
    request.set_filename(filename);

    // Container for the data we expect from the server.
    ::google::api::HttpBody reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    std::unique_ptr<ClientReader<::google::api::HttpBody>> reader(
        stub_->GetSomething(&context, request));

    // The actual RPC.
    std::ofstream output("grpc_client_test.wav", std::ofstream::binary);
    while (reader->Read(&reply)) {
      output << reply.data();
    }
    Status status = reader->Finish();
    output.close();

    if (status.ok()) {
      LOG(INFO) << "GetSomething rpc succeeded.";
    } else {
      LOG(ERROR) << "GetSomething rpc failed.";
    }
  }

 private:
  std::unique_ptr<Demo::Stub> stub_;
};

int main(int argc, char* argv[]) {
  FLAGS_alsologtostderr = 1;
  FLAGS_colorlogtostderr = true;

  // Initialize Google's logging library.
  google::InitGoogleLogging(argv[0]);

  // Optional: parse command line flags
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint (in this case,
  // localhost at port 50051). We indicate that the channel isn't authenticated
  // (use of InsecureChannelCredentials()).
  DemoClient client(grpc::CreateChannel(
      FLAGS_address, grpc::InsecureChannelCredentials()));

  client.GetSomething(FLAGS_filename);

  return 0;
}
