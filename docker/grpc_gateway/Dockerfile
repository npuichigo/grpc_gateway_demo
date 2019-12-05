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

FROM npuichigo/common as build

RUN go get -u github.com/grpc-ecosystem/grpc-gateway/protoc-gen-grpc-gateway && \
    go get -u github.com/grpc-ecosystem/grpc-gateway/protoc-gen-swagger && \
    go get -u github.com/golang/protobuf/protoc-gen-go

# grpc-gateway does not support stream google.api.HttpBody well. Will remove these lines if the
# pr is merged (https://github.com/grpc-ecosystem/grpc-gateway/pull/1088).
RUN git clone https://github.com/npuichigo/grpc-gateway.git /github/npuichigo/grpc-gateway && \
    cp -r /github/npuichigo/grpc-gateway/runtime $GOPATH/src/github.com/grpc-ecosystem/grpc-gateway && \
    rm -rf /github

ENV GRPC_GATEWAY_PATH $GOPATH/src/github.com/npuichigo/grpc_gateway_demo/grpc_gateway

COPY ./grpc_gateway_demo/grpc_gateway $GRPC_GATEWAY_PATH

COPY ./grpc_gateway_demo/proto /grpc_gateway_demo/grpc_gateway_demo/proto
RUN protoc -I/grpc_gateway_demo/grpc_gateway_demo/proto \
        -I$GOPATH/src \
        -I$GOPATH/src/github.com/grpc-ecosystem/grpc-gateway/third_party/googleapis \
        --go_out=plugins=grpc:$GRPC_GATEWAY_PATH/service_gw \
        demo_service.proto

RUN protoc -I/grpc_gateway_demo/grpc_gateway_demo/proto \
        -I$GOPATH/src \
        -I$GOPATH/src/github.com/grpc-ecosystem/grpc-gateway/third_party/googleapis \
        --grpc-gateway_out=logtostderr=true:$GRPC_GATEWAY_PATH/service_gw \
        demo_service.proto

RUN cd $GRPC_GATEWAY_PATH/service_gw && \
    go get . && \
    cd $GRPC_GATEWAY_PATH/proxy_server && \
    go build && \
    go install

FROM ubuntu:16.04
COPY --from=build /usr/local/bin /usr/local/bin
COPY --from=build /usr/local/lib /usr/local/lib
COPY --from=build /go/bin /go/bin

EXPOSE 8080
ENTRYPOINT ["/go/bin/proxy_server"]
