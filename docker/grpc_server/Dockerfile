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
# Author: ASLP@NPU

FROM npuichigo/common AS build

COPY . /grpc_gateway_demo

RUN cd /grpc_gateway_demo/third_party/googleapis && \
    make && \
    cd /grpc_gateway_demo && \
    make

FROM ubuntu:16.04
COPY --from=build /usr/local/bin /usr/local/bin
COPY --from=build /usr/local/lib /usr/local/lib
COPY --from=build /grpc_gateway_demo/build /grpc_gateway_demo/build

WORKDIR /grpc_gateway_demo

EXPOSE 9090
ENTRYPOINT ["/grpc_gateway_demo/build/bin/grpc_server"]
