add_subdirectory(core)

# Send the lists to the parent scope.
set(grpc_gateway_demo_SRCS ${grpc_gateway_demo_SRCS} PARENT_SCOPE)
set(grpc_gateway_demo_TEST_SRCS ${grpc_gateway_demo_TEST_SRCS} PARENT_SCOPE)
