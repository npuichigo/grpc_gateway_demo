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

#include <stdint.h>
#include <unistd.h>

#include <condition_variable>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include "Eigen/Dense"
#include "glog/logging.h"
#include "grpc/grpc.h"
#include "grpcpp/security/server_credentials.h"
#include "grpcpp/server_builder.h"
#include "grpcpp/server_context.h"
#include "grpcpp/support/status.h"

namespace grpc_gateway_demo {
namespace serving {

const int kint32max = ((int)0x7FFFFFFF);

// C++11 doesn't have a barrier so we implement our own.
class Barrier {
 public:
  explicit Barrier(size_t cnt) : threshold_(cnt), count_(cnt), generation_(0) {}

  void Wait() {
    std::unique_lock<std::mutex> lock(mu_);
    auto lgen = generation_;
    if (--count_ == 0) {
      generation_++;
      count_ = threshold_;
      cv_.notify_all();
    } else {
      cv_.wait(lock, [this, lgen] { return lgen != generation_; });
    }
  }

 private:
  std::mutex mu_;
  std::condition_variable cv_;
  const size_t threshold_;
  size_t count_;
  size_t generation_;
};

// The step of processing that the state is in. Every state must
// recognize START, COMPLETE and FINISH and the others are optional.
typedef enum {
  START,
  COMPLETE,
  FINISH,
  ISSUED,
  READ,
  WRITEREADY,
  WRITTEN
} Steps;

std::ostream& operator<<(std::ostream& out, const Steps& step) {
  switch (step) {
    case START:
      out << "START";
      break;
    case COMPLETE:
      out << "COMPLETE";
      break;
    case FINISH:
      out << "FINISH";
      break;
    case ISSUED:
      out << "ISSUED";
      break;
    case READ:
      out << "READ";
      break;
    case WRITEREADY:
      out << "WRITEREADY";
      break;
    case WRITTEN:
      out << "WRITTEN";
      break;
  }
  return out;
}

uint64_t NextUniqueRequestId() {
  static std::atomic<uint64_t> id(0);
  return ++id;
}

template <typename ServerResponderType, typename RequestType,
          typename ResponseType>
class HandlerState {
 public:
  using HandlerStateType =
      HandlerState<ServerResponderType, RequestType, ResponseType>;

  // State that is shared across all state objects that make up a GRPC
  // transaction (e.g. a stream).
  struct Context {
    explicit Context(const char* server_id, const uint64_t unique_id = 0)
        : server_id_(server_id),
          unique_id_(unique_id),
          step_(Steps::START),
          finish_ok_(true) {
      ctx_.reset(new grpc::ServerContext());
      responder_.reset(new ServerResponderType(ctx_.get()));
    }

    // Enqueue 'state' so that its response is delivered in the
    // correct order.
    void EnqueueForResponse(HandlerStateType* state) {
      std::lock_guard<std::mutex> lock(mu_);
      states_.push(state);
    }

    // Check the state at the front of the queue and write it if
    // ready. The state at the front of the queue is ready if it is in
    // the WRITEREADY state and it equals 'required_state' (or
    // 'required_state' is nullptr). Return nullptr if front of queue
    // was not ready (and so not written), or return the state if it
    // was ready and written.
    HandlerStateType* WriteResponseIfReady(HandlerStateType* required_state) {
      std::lock_guard<std::mutex> lock(mu_);
      if (states_.empty()) {
        return nullptr;
      }

      HandlerStateType* state = states_.front();
      if (state->step_ != Steps::WRITEREADY) {
        return nullptr;
      }

      if ((required_state != nullptr) && (state != required_state)) {
        return nullptr;
      }

      state->step_ = Steps::WRITTEN;
      responder_->Write(state->response_, state);

      return state;
    }

    // If 'state' is at the front of the queue and written, pop it and
    // return true. Other return false.
    bool PopCompletedResponse(HandlerStateType* state) {
      std::lock_guard<std::mutex> lock(mu_);
      if (states_.empty()) {
        return false;
      }

      HandlerStateType* front = states_.front();
      if ((front == state) && (state->step_ == Steps::WRITTEN)) {
        states_.pop();
        return true;
      }

      return false;
    }

    // Return true if this context has completed all reads and writes.
    bool IsRequestsCompleted() {
      std::lock_guard<std::mutex> lock(mu_);
      return ((step_ == Steps::WRITEREADY) && states_.empty());
    }

    // Enqueue 'extra_data' to spawn new events in correct order.
    void EnqueueForExtraData(void* extra_data) {
      std::lock_guard<std::mutex> lock(extra_data_mu_);
      extra_data_.push(extra_data);
    }

    bool PopForExtraData(void** extra_data) {
      std::lock_guard<std::mutex> lock(extra_data_mu_);
      if (extra_data_.empty()) {
        return false;
      }

      *extra_data = extra_data_.front();
      extra_data_.pop();
      return true;
    }

    // ID for the server this context is on
    const char* const server_id_;

    // Unique ID for the context.
    const uint64_t unique_id_;

    // Context for the rpc, allowing to tweak aspects of it such as
    // the use of compression, authentication, as well as to send
    // metadata back to the client.
    std::unique_ptr<grpc::ServerContext> ctx_;
    std::unique_ptr<ServerResponderType> responder_;

    // The states associated with this context that are currently
    // active. Used by stream handlers to maintain request / response
    // orders. A state enters this queue when it has successfully read
    // a request and exits the queue when it is written.
    std::mutex mu_;
    std::queue<HandlerStateType*> states_;

    // The extra data used by stream handlers with unary request and
    // stream response. We need to split unary request into multiple
    // events and store them here to process.
    std::mutex extra_data_mu_;
    std::queue<void*> extra_data_;

    // The step of the entire context.
    Steps step_;

    // True if this context should finish with OK status, false if
    // should finish with CANCELLED status.
    bool finish_ok_;
  };

  explicit HandlerState(const std::shared_ptr<Context>& context,
                        Steps start_step = Steps::START) {
    Reset(context, start_step);
  }

  void Reset(const std::shared_ptr<Context>& context,
             Steps start_step = Steps::START) {
    context_ = context;
    unique_id_ = NextUniqueRequestId();
    step_ = start_step;
    request_.Clear();
    response_.Clear();
  }

  void Release() { context_ = nullptr; }

  std::shared_ptr<Context> context_;

  uint64_t unique_id_;
  Steps step_;

  RequestType request_;
  ResponseType response_;
};

template <typename ServiceType, typename ServerResponderType,
          typename RequestType, typename ResponseType>
class Handler : public Server::HandlerBase {
 public:
  Handler(const std::string& name,
          const std::shared_ptr<thread::ThreadPool>& pool,
          const char* server_id, ServiceType* service,
          grpc::ServerCompletionQueue* cq, size_t max_state_bucket_count);
  virtual ~Handler();

  // Descriptive name of of the handler.
  const std::string& Name() const { return name_; }

  // Start handling requests using 'thread_cnt' threads.
  void Start(int thread_cnt);

  // Stop handling requests.
  void Stop();

 protected:
  using State = HandlerState<ServerResponderType, RequestType, ResponseType>;
  using StateContext = typename State::Context;

  State* StateNew(const std::shared_ptr<StateContext>& context,
                  Steps start_step = Steps::START) {
    State* state = nullptr;

    if (max_state_bucket_count_ > 0) {
      std::lock_guard<std::mutex> lock(alloc_mu_);

      if (!state_bucket_.empty()) {
        state = state_bucket_.back();
        state->Reset(context, start_step);
        state_bucket_.pop_back();
      }
    }

    if (state == nullptr) {
      state = new State(context, start_step);
    }

    return state;
  }

  void StateRelease(State* state) {
    if (max_state_bucket_count_ > 0) {
      std::lock_guard<std::mutex> lock(alloc_mu_);

      if (state_bucket_.size() < max_state_bucket_count_) {
        state->Release();
        state_bucket_.push_back(state);
        return;
      }
    }

    delete state;
  }

  virtual void StartNewRequest() = 0;
  virtual bool Process(State* state, bool rpc_ok) = 0;

  const std::string name_;
  std::shared_ptr<thread::ThreadPool> pool_;
  const char* const server_id_;

  ServiceType* service_;
  grpc::ServerCompletionQueue* cq_;
  std::vector<std::unique_ptr<std::thread>> threads_;

  // Mutex to serialize State allocation
  std::mutex alloc_mu_;

  // Keep some number of state objects for reuse to avoid the overhead
  // of creating a state for every new request.
  const size_t max_state_bucket_count_;
  std::vector<State*> state_bucket_;
};

template <typename ServiceType, typename ServerResponderType,
          typename RequestType, typename ResponseType>
Handler<ServiceType, ServerResponderType, RequestType, ResponseType>::Handler(
    const std::string& name, const std::shared_ptr<thread::ThreadPool>& pool,
    const char* server_id, ServiceType* service,
    grpc::ServerCompletionQueue* cq, size_t max_state_bucket_count)
    : name_(name),
      pool_(pool),
      server_id_(server_id),
      service_(service),
      cq_(cq),
      max_state_bucket_count_(max_state_bucket_count) {}

template <typename ServiceType, typename ServerResponderType,
          typename RequestType, typename ResponseType>
Handler<ServiceType, ServerResponderType, RequestType,
        ResponseType>::~Handler() {
  for (State* state : state_bucket_) {
    delete state;
  }
  state_bucket_.clear();

  VLOG(1) << "Destructed " << Name();
}

template <typename ServiceType, typename ServerResponderType,
          typename RequestType, typename ResponseType>
void Handler<ServiceType, ServerResponderType, RequestType,
             ResponseType>::Start(int thread_cnt) {
  // Use a barrier to make sure we don't return until all threads have
  // started.
  auto barrier = std::make_shared<Barrier>(thread_cnt + 1);

  for (int t = 0; t < thread_cnt; ++t) {
    threads_.emplace_back(new std::thread([this, barrier] {
      StartNewRequest();
      barrier->Wait();

      void* tag;
      bool ok;

      while (cq_->Next(&tag, &ok)) {
        State* state = static_cast<State*>(tag);
        if (!Process(state, ok)) {
          VLOG(1) << "Done for " << Name() << ", " << state->unique_id_;
          StateRelease(state);
        }
      }
    }));
  }

  barrier->Wait();
  VLOG(1) << "Threads started for " << Name();
}

template <typename ServiceType, typename ServerResponderType,
          typename RequestType, typename ResponseType>
void Handler<ServiceType, ServerResponderType, RequestType,
             ResponseType>::Stop() {
  for (const auto& thread : threads_) {
    thread->join();
  }

  VLOG(1) << "Threads exited for " << Name();
}

/*class InferHandler
    : public Handler<Demo::AsyncService,
                     grpc::ServerAsyncResponseWriter<GetResponse>, GetRequest,
                     GetResponse> {
 public:
  InferHandler(const std::string& name,
               const std::shared_ptr<thread::ThreadPool>& pool,
               const char* server_id, Demo::AsyncService* service,
               grpc::ServerCompletionQueue* cq, size_t max_state_bucket_count)
      : Handler(name, pool, server_id, service, cq, max_state_bucket_count) {}

 protected:
  void StartNewRequest() override;
  bool Process(State* state, bool rpc_ok) override;
};

void InferHandler::StartNewRequest() {
  auto context = std::make_shared<State::Context>(server_id_);
  State* state = StateNew(context);

  service_->RequestGetSomething(state->context_->ctx_.get(), &state->request_,
                                state->context_->responder_.get(), cq_, cq_,
                                state);

  VLOG(1) << "New request handler for " << Name() << ", " << state->unique_id_;
}

bool InferHandler::Process(Handler::State* state, bool rpc_ok) {
  VLOG(1) << "Process for " << Name() << ", rpc_ok=" << rpc_ok << ", "
          << state->unique_id_ << " step " << state->step_;

  // We need an explicit finish indicator. Can't use 'state->step_'
  // because we launch an async thread that could update 'state's
  // step_ to be FINISH before this thread exits this function.
  bool finished = false;

  // If RPC failed on a new request then the server is shutting down
  // and so we should do nothing (including not registering for a new
  // request). If RPC failed on a non-START step then there is nothing
  // we can do since we one execute one step.
  const bool shutdown = (!rpc_ok && (state->step_ == Steps::START));
  if (shutdown) {
    state->step_ = Steps::FINISH;
    finished = true;
  }

  if (state->step_ == Steps::START) {
    // Start a new request to replace this one...
    if (!shutdown) {
      StartNewRequest();
    }
    // Async call the worker.
    state->step_ = Steps::ISSUED;
    pool_->ScheduleCallback([state] {
      const GetRequest& request = state->request_;
      GetResponse& response = state->response_;

      auto filename = request.filename();
      std::string extension = filename.substr(filename.find_last_of(".") + 1);
      std::string content_type;
      if (extension == "mp3")
        content_type = "audio/mp3";
      else if (extension == "wav")
        content_type = "audio/wav";
      else
        content_type = "application/json";
      response.set_content(content_type);

      // Slow calculation.
      using Eigen::MatrixXf;
      const int size = 1024;
      Eigen::setNbThreads(4);
      MatrixXf m1 = MatrixXf::Random(size, size);
      MatrixXf m2 = MatrixXf::Random(size, size);
      MatrixXf m3(size, size);
      m3.noalias() = m1 * m2;

      state->step_ = Steps::COMPLETE;
      state->context_->responder_->Finish(response, grpc::Status::OK, state);
    });
  } else if (state->step_ == Steps::COMPLETE) {
    state->step_ = Steps::FINISH;
    finished = true;
  }

  return !finished;
}*/

// StreamInferHandler
class StreamInferHandler
    : public Handler<Demo::AsyncService,
                     grpc::ServerAsyncWriter<::google::api::HttpBody>,
                     GetRequest, ::google::api::HttpBody> {
 public:
  StreamInferHandler(const std::string& name,
                     const std::shared_ptr<thread::ThreadPool>& pool,
                     const char* server_id, Demo::AsyncService* service,
                     grpc::ServerCompletionQueue* cq,
                     size_t max_state_bucket_count)
      : Handler(name, pool, server_id, service, cq, max_state_bucket_count) {}

 protected:
  void StartNewRequest() override;
  bool Process(State* state, bool rpc_ok) override;
};

void StreamInferHandler::StartNewRequest() {
  const uint64_t unique_id = NextUniqueRequestId();
  auto context = std::make_shared<State::Context>(server_id_, unique_id);
  State* state = StateNew(context);
  service_->RequestGetSomething(state->context_->ctx_.get(), &state->request_,
                                state->context_->responder_.get(), cq_, cq_,
                                state);

  VLOG(1) << "New request handler for " << Name() << ", " << state->unique_id_;
}

bool StreamInferHandler::Process(Handler::State* state, bool rpc_ok) {
  VLOG(1) << "Process for " << Name() << ", rpc_ok=" << rpc_ok << ", context "
          << state->context_->unique_id_ << ", " << state->unique_id_
          << " step " << state->step_;

  // We need an explicit finish indicator. Can't use 'state->step_'
  // because we launch an async thread that could update 'state's
  // step_ to be FINISH before this thread exits this function.
  bool finished = false;

  if (state->step_ == Steps::START) {
    // A new stream connection... If RPC failed on a new request then
    // the server is shutting down and so we should do nothing.
    if (!rpc_ok) {
      state->step_ = Steps::FINISH;
      return false;
    }

    // Start a new request to replace this one...
    StartNewRequest();

    for (int i = 0; i < 10; ++i) {
      void* extra_data = reinterpret_cast<void*>(new int(i));
      state->context_->EnqueueForExtraData(extra_data);
    }

    // Since this is the start of a connection, 'state' hasn't been
    // used yet so use it to read a request off the connection.
    state->step_ = Steps::WRITTEN;
    // state->context_->responder_->Write(state->response_, state);
    state->context_->responder_->SendInitialMetadata(state);
  } else if (state->step_ == Steps::WRITTEN) {
    // If the write failed (for example, client closed the stream)
    // mark that the stream did not complete successfully but don't
    // cancel right away... need to wait for any pending inferences
    // and writes to complete.
    if (!rpc_ok) {
      VLOG(1) << "Write for " << Name() << ", rpc_ok=" << rpc_ok << ", context "
              << state->context_->unique_id_ << ", " << state->unique_id_
              << " step " << state->step_ << ", failed";
      state->context_->finish_ok_ = false;
    }

    void* data = nullptr;
    if (!state->context_->PopForExtraData(&data)) {
      state->context_->step_ = Steps::COMPLETE;
      state->step_ = Steps::COMPLETE;
      state->context_->responder_->Finish(state->context_->finish_ok_
                                              ? grpc::Status::OK
                                              : grpc::Status::CANCELLED,
                                          state);
    } else {
      // Async call the worker.
      state->step_ = Steps::ISSUED;
      pool_->ScheduleCallback([state, data]() mutable {
        int value = *reinterpret_cast<int*>(data);
        delete reinterpret_cast<int*>(data);
        auto& response = state->response_;

        std::string content_type = "application/json";
        response.set_content_type(content_type);
        std::string json_str = "{\"key1\":\"" + std::to_string(value) + "\"}";
        response.set_data(json_str.c_str(), json_str.size());

        state->step_ = WRITTEN;
        state->context_->responder_->Write(response, state);
      });
    }
  } else if (state->step_ == Steps::COMPLETE) {
    state->step_ = Steps::FINISH;
    finished = true;
  }

  return !finished;
}

Server::Server(const char* server_id)
    : server_id_(server_id), infer_allocation_pool_size_(8) {}

Server::~Server() {}

bool Server::BuildAndStart(const Options& server_options) {
  if (server_options.grpc_port == 0) {
    LOG(ERROR) << "server_options.grpc_port is not set.";
    return false;
  }
  // 0.0.0.0" is the way to listen on localhost in gRPC.
  const std::string server_address =
      "0.0.0.0:" + std::to_string(server_options.grpc_port);

  // Listen on the given address without any authentication mechanism.
  grpc_builder_.AddListeningPort(server_address,
                                 grpc::InsecureServerCredentials());
  // Register service through which we'll communicate with clients
  grpc_builder_.SetMaxMessageSize(kint32max);
  grpc_builder_.RegisterService(&service_);
  cq_ = grpc_builder_.AddCompletionQueue();
  grpc_server_ = grpc_builder_.BuildAndStart();
  if (grpc_server_ == nullptr) {
    LOG(ERROR) << "Failed to BuildAndStart gRPC server";
    return false;
  }

  pool_.reset(new thread::ThreadPool(4));

  // Handler for inference requests. 'infer_thread_cnt_' is not used
  // below due to thread-safety requirements and the way
  // InferHandler::Process is written. We should likely implement it
  // by making multiple completion queues each of which is serviced by
  // a single thread.
  // InferHandler* hinfer = new InferHandler(
  //    "InferHandler", pool_, server_id_, &service_, cq_.get(),
  //    infer_allocation_pool_size_ /* max_state_bucket_count */);
  // hinfer->Start(/* infer_thread_cnt_ */ 1);
  // infer_handler_.reset(hinfer);

  // Handler for streaming inference requests.
  // 'stream_infer_thread_cnt_' is not used below due to thread-safety
  // requirements and the way StreamInferHandler::Process is
  // written. We should likely implement it by making multiple
  // completion queues each of which is serviced by a single thread.
  StreamInferHandler* hstreaminfer = new StreamInferHandler(
      "StreamInferHandler", pool_, server_id_, &service_, cq_.get(),
      infer_allocation_pool_size_ /* max_state_bucket_count */);
  hstreaminfer->Start(/* stream_infer_thread_cnt_ */ 1);
  stream_infer_handler_.reset(hstreaminfer);

  running_ = true;
  LOG(INFO) << "Running gRPC Server at " << server_address << " ...";
  return true;
}

namespace {

// Exit mutex and cv used to signal the main thread that it should
// close the server and exit.
volatile bool exiting_ = false;
std::mutex exit_mu_;
std::condition_variable exit_cv_;

void SignalHandler(int signum) {
  // Don't need a mutex here since signals should be disabled while in
  // the handler.
  LOG(INFO) << "Interrupt signal (" << signum << ") received.";

  // Do nothing if already exiting...
  if (exiting_) return;

  {
    std::unique_lock<std::mutex> lock(exit_mu_);
    exiting_ = true;
  }

  exit_cv_.notify_all();
}

}  // namespace

void Server::WaitForTermination() {
  // Trap SIGINT and SIGTERM to allow server to exit gracefully
  signal(SIGINT, SignalHandler);
  signal(SIGTERM, SignalHandler);

  // Wait until a signal terminates the server...
  while (!exiting_) {
    // Wait for a long time. Will be woken if the server is exiting.
    std::unique_lock<std::mutex> lock(exit_mu_);
    std::chrono::seconds wait_timeout(3600);
    exit_cv_.wait_for(lock, wait_timeout);
  }
}

}  // namespace serving
}  // namespace grpc_gateway_demo
