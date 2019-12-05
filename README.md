# grpc_gateway_demo

This repository is used to show the gRPC-HTTP transcoder in grap-gateway(https://github.com/grpc-ecosystem/grpc-gateway) can be used together with stream google.api.HttpBody to support streaming media transfer. For example, in the scenario of text-to-speech, it's useful to synthesize speech with streaming mode to reduce latency. This demo also shows how we can automatically transcode streaming grpc to chunked http response.

## Quick Start Guide

Clone the repository and dependencies:
```sh
git clone https://github.com/npuichigo/grpc_gateway_demo.git
cd grpc_gateway_demo
```

From the repo directory, test your service with docker-compose:
```sh
docker-compose pull
docker-compose up
```

Or you can rebuild the images with:

```sh
$ docker-compose build
```

Test restful api with curl:

```sh
$ curl "localhost:8080/get?filename=testdata/music.mp3" -4 -v > test_chunk_music.mp3
  % Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                                 Dload  Upload   Total   Spent    Left  Speed
  0     0    0     0    0     0      0      0 --:--:-- --:--:-- --:--:--     0*   Trying 127.0.0.1...
* Connected to localhost (127.0.0.1) port 8080 (#0)
> GET /get?filename=testdata/music.mp3 HTTP/1.1
> Host: localhost:8080
> User-Agent: curl/7.47.0
> Accept: */*
> 
< HTTP/1.1 200 OK
< Content-Type: audio/mp3
< Grpc-Metadata-Accept-Encoding: identity,gzip
< Grpc-Metadata-Content-Type: application/grpc
< Grpc-Metadata-Grpc-Accept-Encoding: identity,deflate,gzip
< Date: Wed, 27 Nov 2019 03:28:40 GMT
< Transfer-Encoding: chunked
< 
{ [3836 bytes data]
```

The grpc-server will send chunk of streaming data and the grpc-gateway will transcode that to chunked http response.
```sh
$ docker-compose up
WARNING: Some services (grpc-server) use the 'deploy' key, which will be ignored. Compose does not support 'deploy' configuration - use `docker stack deploy` to deploy to a swarm.
Starting grpc_gateway_demo_grpc-server_1 ... done
Starting grpc_gateway_demo_grpc-gateway_1 ... done
Starting grpc_gateway_demo_nginx_1        ... done
Attaching to grpc_gateway_demo_grpc-server_1, grpc_gateway_demo_grpc-gateway_1, grpc_gateway_demo_nginx_1
grpc-server_1   | I1127 03:28:21.755975     1 server.cc:58] Running gRPC Server at 0.0.0.0:9090 ...
grpc-server_1   | I1127 03:28:40.348382     9 demo_service_impl.cc:36] Get audio from grpc server: testdata/music.mp3
grpc-server_1   | I1127 03:28:40.349026     9 demo_service_impl.cc:59] Send 65536 bytes
grpc-server_1   | I1127 03:28:40.449465     9 demo_service_impl.cc:59] Send 65536 bytes
grpc-server_1   | I1127 03:28:40.550000     9 demo_service_impl.cc:59] Send 65536 bytes
```

You can now use your browser to play the streaming media.

<img src="https://github.com/npuichigo/grpc_gateway_demo/blob/master/images/play_mp3.png"/>
<img src="https://github.com/npuichigo/grpc_gateway_demo/blob/master/images/header.png" width="500"/>

## Test with grpc client

Just start with the docker image grpc-server:
```sh
docker run -it --name your_name npuichigo/grpc-server /bin/bash
./build/bin/grpc_server
```

Open another terminal to use the grpc client:
```sh
docker exec -it your_name /bin/bash
./build/bin/grpc_test_client --filename testdata/music.mp3
```


