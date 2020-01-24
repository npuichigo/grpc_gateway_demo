# common files.
file(GLOB tmp *.cc)
set(grpc_gateway_demo_SRCS ${grpc_gateway_demo_SRCS} ${tmp})
# exclude test files
file(GLOB tmp *_test.cc)
exclude(grpc_gateway_demo_SRCS "${grpc_gateway_demo_SRCS}" ${tmp})

# test files
file(GLOB tmp *_test.cc)
set(grpc_gateway_demo_TEST_SRCS ${grpc_gateway_demo_TEST_SRCS} ${tmp})

# Send the lists to the parent scope.
set(grpc_gateway_demo_SRCS ${grpc_gateway_demo_SRCS} PARENT_SCOPE)
set(grpc_gateway_demo_TEST_SRCS ${grpc_gateway_demo_TEST_SRCS} PARENT_SCOPE)
