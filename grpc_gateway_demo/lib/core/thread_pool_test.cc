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

#include "gtest/gtest.h"

namespace tts_system {
namespace thread {

TEST(ThreadPoolTest, ScheduleCallback) {
  // Create thread pool with 4 worker threads.
  ThreadPool pool(4);

  for (int i = 0; i < 16; ++i)
    pool.ScheduleCallback([i] { return i * i; });
}

}  // namespace thread
}  // namespace tts_system
