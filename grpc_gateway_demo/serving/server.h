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

#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>

#include "Eigen/Dense"
#include "grpc_gateway_demo/serving/demo_service.grpc.pb.h"
#include "glog/logging.h"
#include "grpc_gateway_demo/lib/core/thread_pool.h"

namespace grpc_gateway_demo {
namespace serving {

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
// Class encompasing the state and logic needed to serve a request.
  class CallData {
   public:
    // Take in the "service" instance (in this case representing an asynchronous
    // server) and the completion queue "cq" used for asynchronous communication
    // with the gRPC runtime.
    CallData(Demo::AsyncService* service, grpc::ServerCompletionQueue* cq,
             thread::ThreadPool* pool)
        : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE),
          pool_(pool) {
      // Invoke the serving logic right away.
      Proceed();
    }

    void Proceed() {
      if (status_ == CREATE) {
        // Make this instance progress to the PROCESS state.
        status_ = PROCESS;

        // As part of the initial CREATE state, we *request* that the system
        // start processing SayHello requests. In this request, "this" acts are
        // the tag uniquely identifying the request (so that different CallData
        // instances can serve different requests concurrently), in this case
        // the memory address of this CallData instance.
        service_->RequestGetSomething(&ctx_, &request_, &responder_, cq_, cq_,
                                      this);
      } else if (status_ == PROCESS) {
        // Spawn a new CallData instance to serve new clients while we process
        // the one for this CallData. The instance will deallocate itself as
        // part of its FINISH state.
        new CallData(service_, cq_, pool_);

        auto filename = request_.filename();

        std::string extension = filename.substr(filename.find_last_of(".") + 1);
        std::string content_type;
        if (extension == "mp3")
          content_type = "audio/mp3";
        else if (extension == "wav")
          content_type = "audio/wav";
        else
          content_type = "application/json";

        pool_->ScheduleCallback([this, content_type] {
          using Eigen::MatrixXf;
          const int size = 1024;
          Eigen::setNbThreads(4);
          MatrixXf m1 = MatrixXf::Random(size, size);
          MatrixXf m2 = MatrixXf::Random(size, size);
          MatrixXf m3(size, size);
          m3.noalias() = m1 * m2;
          reply_.set_content(content_type);
          // And we are done! Let the gRPC runtime know we've finished, using the
          // memory address of this instance as the uniquely identifying tag for
          // the event.
          status_ = FINISH;
          responder_.Finish(reply_, grpc::Status::OK, this);
        });
     } else {
        GPR_ASSERT(status_ == FINISH);
        // Once in the FINISH state, deallocate ourselves (CallData).
        delete this;
      }
    }
   private:
    // The means of communication with the gRPC runtime for an asynchronous
    // server.
    Demo::AsyncService* service_;
    // The producer-consumer queue where for asynchronous server notifications.
    grpc::ServerCompletionQueue* cq_;
    // Context for the rpc, allowing to tweak aspects of it such as the use
    // of compression, authentication, as well as to send metadata back to the
    // client.
    grpc::ServerContext ctx_;

    // What we get from the client.
    GetRequest request_;
    // What we send back to the client.
    GetResponse reply_;

    thread::ThreadPool* pool_;

    // The means to get back to the client.
    grpc::ServerAsyncResponseWriter<GetResponse> responder_;

    // Let's implement a tiny state machine with the following states.
    enum CallStatus { CREATE, PROCESS, FINISH };
   public:
    CallStatus status_;  // The current serving state.
  };

  std::unique_ptr<thread::ThreadPool> pool_;

  std::unique_ptr<grpc::ServerCompletionQueue> cq_;
  Demo::AsyncService service_;
  grpc::ServerBuilder grpc_builder_; 
  std::unique_ptr<grpc::Server> grpc_server_;
};

}  // namespace serving
}  // namespace grpc_gateway_demo

#endif  // GRPC_GATEWAY_DEMO_SERVING_SERVER_H_
