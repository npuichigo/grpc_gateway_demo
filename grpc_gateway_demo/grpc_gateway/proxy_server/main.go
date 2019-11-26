package main

import (
	"flag"
	"net/http"

	"github.com/golang/glog"
	"golang.org/x/net/context"
	"github.com/grpc-ecosystem/grpc-gateway/runtime"
	"google.golang.org/grpc"

	gw "github.com/npuichigo/grpc_gateway_demo/grpc_gateway/service_gw"
)

var (
	proxyEndpoint = flag.String("proxy_endpoint", "localhost:9090", "endpoint of DemoService")
)

func run() error {
	ctx := context.Background()
	ctx, cancel := context.WithCancel(ctx)
	defer cancel()

	mux := runtime.NewServeMux()
	runtime.SetHTTPBodyMarshaler(mux)
	opts := []grpc.DialOption{grpc.WithInsecure()}
	err := gw.RegisterDemoHandlerFromEndpoint(ctx, mux, *proxyEndpoint, opts)
	if err != nil {
	  return err
	}

  s := &http.Server {
    Addr: ":8080",
    Handler: allowCORS(mux),
  }

  glog.Infof("Starting listening at %s", ":8080")
  if err := s.ListenAndServe(); err != http.ErrServerClosed {
    glog.Errorf("Failed to listen and server: %v", err)
    return err
  }
  return nil
}

func main() {
	flag.Parse()
	defer glog.Flush()

  glog.Info("Running grpc gateway proxy server on 0.0.0.0:8080")
  glog.Info("Grpc endpoint is: ", proxyEndpoint)

	if err := run(); err != nil {
	  glog.Fatal(err)
	}
}
