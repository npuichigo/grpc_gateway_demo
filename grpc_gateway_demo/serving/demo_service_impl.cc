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

#include "grpc_gateway_demo/serving/demo_service_impl.h"

#include <unistd.h>
#include <fstream>
#include <sstream>
#include <string>

#include "glog/logging.h"

namespace grpc_gateway_demo {
namespace serving {

static const int kBufferSize = 32 * 1024;

::grpc::Status DemoServiceImpl::GetSomething(
    ::grpc::ServerContext* context,
    const GetRequest* request,
    ::grpc::ServerWriter<::google::api::HttpBody>* writer) {
  auto wav_filename = request->filename();
  LOG(INFO) << "Get audio from grpc server: " << wav_filename;

  std::string extension = wav_filename.substr(
      wav_filename.find_last_of(".") + 1);
  std::string content_type = "application/json";
  if (extension == "mp3")
    content_type = "audio/mp3";
  else if (extension == "wav")
    content_type = "audio/wav";

  std::ifstream input(wav_filename, std::ifstream::binary);
  int byte_read = 0;
  char buffer[kBufferSize];
  while (!input.eof()) {
    if (context->IsCancelled()) {
      return ::grpc::Status::CANCELLED;
    }
    input.read(buffer, kBufferSize);
    byte_read = input.gcount();
    ::google::api::HttpBody reply;
    reply.set_content_type(content_type);
    reply.set_data(buffer, byte_read);
    writer->Write(reply);
    LOG(INFO) << "Send " << byte_read << " bytes";
    usleep(500 * 1000);
  }
  input.close();
  return ::grpc::Status::OK;
}

}  // namespace serving
}  // namespace grpc_gateway_demo
