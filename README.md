# grpc_gateway_demo

```shell
$ curl localhost:8080/get -4 -X POST -H "Content-Type:application/json" -d '{"filename": "testdata/big.wav"}' -v > test_chunk_big.wav
Note: Unnecessary use of -X or --request, POST is already inferred.
  % Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                                 Dload  Upload   Total   Spent    Left  Speed
  0     0    0     0    0     0      0      0 --:--:-- --:--:-- --:--:--     0*   Trying 127.0.0.1...
* Connected to localhost (127.0.0.1) port 8080 (#0)
> POST /get HTTP/1.1
> Host: localhost:8080
> User-Agent: curl/7.47.0
> Accept: */*
> Content-Type:application/json
> Content-Length: 32
> 
} [32 bytes data]
* upload completely sent off: 32 out of 32 bytes
< HTTP/1.1 200 OK
< Content-Type: audio/wav
< Grpc-Metadata-Accept-Encoding: identity,gzip
< Grpc-Metadata-Content-Type: application/grpc
< Grpc-Metadata-Grpc-Accept-Encoding: identity,deflate,gzip
< Date: Mon, 25 Nov 2019 13:42:58 GMT
< Transfer-Encoding: chunked
< 
{ [3836 bytes data]
100 1137k    0 1137k  100    32  64627      1  0:00:32  0:00:18  0:00:14 58610
* Connection #0 to host localhost left intact
```
