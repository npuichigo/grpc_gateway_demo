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

#ifndef GRPC_GATEWAY_DEMO_LIB_CORE_THREADPOOL_H_
#define GRPC_GATEWAY_DEMO_LIB_CORE_THREADPOOL_H_

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace grpc_gateway_demo {
namespace thread {

class ThreadPool {
 public:
  explicit ThreadPool(int num_threads);
  ~ThreadPool();

  void ScheduleCallback(const std::function<void()>& callback);

 private:
  void ThreadFunc(int thread_id);

  std::mutex mu_;
  std::condition_variable cv_;
  bool shutdown_;
  std::queue<std::function<void()>> callbacks_;
  std::vector<std::thread> threads_;
};

}  // namespace thread
}  // namespace grpc_gateway_demo

#endif  // GRPC_GATEWAY_DEMO_LIB_CORE_THREADPOOL_H_
