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

FROM ubuntu:16.04

ARG MAKEFLAGS=-j8

RUN apt-get update && apt-get install -y --no-install-recommends \
        autoconf \
        automake \
        build-essential \
        cmake \
        curl \
        default-jdk \
        default-jre \
        git \
        libpcre3-dev \
        libssl-dev \
        make \
        pkg-config \
        python3 \
        python3-pip \
        vim \
        unzip \
        wget \
        zip && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Install golang
ENV GOLANG_VERSION 1.13.4

RUN set -eux; \
    dpkgArch="$(dpkg --print-architecture)"; \
    case "${dpkgArch##*-}" in \
		    amd64) goRelArch='linux-amd64'; goRelSha256='692d17071736f74be04a72a06dab9cac1cd759377bd85316e52b2227604c004c' ;; \
		    armhf) goRelArch='linux-armv6l'; goRelSha256='9f76e6353c9ae2dcad1731b7239531eb8be2fe171f29f2a9c5040945a930fd41' ;; \
		    arm64) goRelArch='linux-arm64'; goRelSha256='8b8d99eb07206f082468fb4d0ec962a819ae45d54065fc1ed6e2c502e774aaf0' ;; \
		    i386) goRelArch='linux-386'; goRelSha256='497934398ca57c7c207ce3388f099823923b4c7b74394d6ed64cd2d3751aecb8' ;; \
		    ppc64el) goRelArch='linux-ppc64le'; goRelSha256='815bf3c7100e73cfac50c4a07c8eeb4b0458a49ffa0e13a74a6cf7ad8e2a6499' ;; \
		    s390x) goRelArch='linux-s390x'; goRelSha256='efc6947e8eb0a6409f4c8ba62b00ae4e54404064bc221df1b73364a95945a350' ;; \
		    *) goRelArch='src'; goRelSha256='95dbeab442ee2746b9acf0934c8e2fc26414a0565c008631b04addb8c02e7624'; \
			      echo >&2; echo >&2 "warning: current architecture ($dpkgArch) does not have a corresponding Go binary release; will be building from source"; echo >&2 ;; \
	  esac; \
    \
    url="http://golang.org/dl/go${GOLANG_VERSION}.${goRelArch}.tar.gz"; \
    wget --no-check-certificate -O go.tgz "$url"; \
    echo "${goRelSha256} *go.tgz" | sha256sum -c -; \
    tar -C /usr/local -xzf go.tgz; \
    rm go.tgz; \
    \
    if [ "$goRelArch" = 'src' ]; then \
        echo >&2; \
        echo >&2 'error: UNIMPLEMENTED'; \
        echo >&2 'TODO install golang-any from jessie-backports for GOROOT_BOOTSTRAP (and uninstall after build)'; \
        echo >&2; \
        exit 1; \
    fi; \
    \
    export PATH="/usr/local/go/bin:$PATH"; \
    go version

ENV GOPATH /go
ENV PATH $GOPATH/bin:/usr/local/go/bin:$PATH

RUN mkdir -p "$GOPATH/src" "$GOPATH/bin" && chmod -R 777 "$GOPATH"

# Install grpc with protobuf and gflags
RUN git clone -b v1.17.1 https://github.com/grpc/grpc /github/grpc && \
    cd /github/grpc && \
    git submodule update --init && \
    \
    cd third_party/cares/cares && \
    git fetch origin && \
    git checkout cares-1_13_0 && \
    cmake -H. -Bbuild -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build && \
    cmake --build build --target install && \
    cd ../../../ && \
    rm -rf third_party/cares/cares && \
    \
    cd third_party/zlib && \
    cmake -H. -Bbuild -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build && \
    cmake --build build --target install && \
    cd ../../ && \
    rm -rf third_party/zlib && \
    \
    cd third_party/protobuf && \
    cmake -H./cmake -Bbuild -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_BUILD_SHARED_LIBS=ON && \
    cmake --build build && \
    cmake --build build --target install && \
    ldconfig && \
    cd ../../ && \
    rm -rf third_party/protobuf && \
    \
    cmake -H. -Bbuild -G "Unix Makefiles" -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF \
        -DgRPC_PROTOBUF_PROVIDER=package -DgRPC_ZLIB_PROVIDER=package -DgRPC_CARES_PROVIDER=package \
        -DgRPC_SSL_PROVIDER=package -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build && \
    cmake --build build --target install && \
    \
    rm -rf /github/grpc

RUN pip3 install grpcio==1.17.1 grpcio-tools==1.17.1

# Install glog
RUN git clone -b v0.3.5 https://github.com/google/glog.git /github/glog && \
    cd /github/glog && \
    cmake -H. -Bbuild -G "Unix Makefiles" && \
    cmake --build build && \
    cmake --build build --target install && \
    rm -rf /github/glog
