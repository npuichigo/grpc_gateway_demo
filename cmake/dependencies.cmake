# Copyright 2018 ASLP@NPU.  All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Author: npuichigo@gmail.com (zhangyuchao)

# Threads
include(${CMAKE_CURRENT_LIST_DIR}/public/threads.cmake)
if(TARGET Threads::Threads)
  list(APPEND grpc_gateway_demo_DEPENDENCY_LIBS Threads::Threads)
else()
  message(FATAL_ERROR
    "Cannot find threading library. grpc_gateway_demo requires Threads to compile.")
endif()

# gflags
include(${CMAKE_CURRENT_LIST_DIR}/public/gflags.cmake)
if (NOT TARGET gflags)
  include_directories(SYSTEM ${GFLAGS_INCLUDE_DIR})
  list(APPEND grpc_gateway_demo_DEPENDENCY_LIBS ${GFLAGS_LIBRARIES})
  message(FATAL_ERROR
    "Cannot find gflags library. grpc_gateway_demo requires gflags to compile.")
endif()

# glog
include(${CMAKE_CURRENT_LIST_DIR}/public/glog.cmake)
if (TARGET glog::glog)
  include_directories(SYSTEM ${GLOG_INCLUDE_DIR})
  list(APPEND grpc_gateway_demo_DEPENDENCY_LIBS glog::glog)
else()
  message(FATAL_ERROR
    "Cannot find glog library. grpc_gateway_demo requires glog to compile.")
endif()

# Protobuf
include(${CMAKE_CURRENT_LIST_DIR}/public/protobuf.cmake)
if((TARGET protobuf::libprotobuf OR TARGET protobuf::libprotobuf-lite) AND TARGET protobuf::protoc)
  include_directories(SYSTEM ${PROTOBUF_INCLUDE_DIR} ${PROTOBUF_INCLUDE_DIRS})
  list(APPEND grpc_gateway_demo_DEPENDENCY_LIBS protobuf::libprotobuf)
  set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
else()
  message(FATAL_ERROR
    "Cannot find protobuf library. grpc_gateway_demo requires protobuf to compile.")
endif()

# gRPC
include(${CMAKE_CURRENT_LIST_DIR}/public/grpc.cmake)
if((TARGET gRPC::grpc++_unsecure) AND (TARGET gRPC::grpc_cpp_plugin))
  list(APPEND grpc_gateway_demo_DEPENDENCY_LIBS gRPC::grpc++_unsecure)
  set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
else()
  message(FATAL_ERROR
    "Cannot find grpc library. grpc_gateway_demo requires grpc to compile.")
endif()

# googleapis
include_directories(${CMAKE_CURRENT_LIST_DIR}/../third_party/googleapis/gens)
