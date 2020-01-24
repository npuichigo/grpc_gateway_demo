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

#include "grpc_gateway_demo/lib/core/thread_pool.h"

#include "glog/logging.h"

namespace grpc_gateway_demo {
namespace thread {

void ThreadPool::ThreadFunc(int thread_id) {
  for (;;) {
    //Wait until work is available or we are shutting down.
    std::unique_lock<std::mutex> lock(mu_);
    if (!shutdown_ && callbacks_.empty()) {
      cv_.wait(lock);
    }
    // Drain callbackes before considering shutdown to ensure all work
    // gets completed.
    if (!callbacks_.empty()) {
      auto cb = callbacks_.front();
      callbacks_.pop();
      lock.unlock();
      LOG(INFO) << "Thread (" << thread_id << ") processing";
      cb();
    } else if (shutdown_) {
      return;
    }
  }
}

ThreadPool::ThreadPool(int num_threads) : shutdown_(false) {
  for (int i = 0; i < num_threads; i++) {
    threads_.push_back(std::thread(&ThreadPool::ThreadFunc, this, i));
  }
}

ThreadPool::~ThreadPool() {
  {
    std::lock_guard<std::mutex> lock(mu_);
    shutdown_ = true;
    cv_.notify_all();
  }
  for (auto t = threads_.begin(); t != threads_.end(); t++) {
    t->join();
  }
}

void ThreadPool::ScheduleCallback(const std::function<void()>& callback) {
  std::lock_guard<std::mutex> lock(mu_);
  callbacks_.push(callback);
  cv_.notify_one();
}

}  // namespace thread
}  // namespace grpc_gateway_demo
