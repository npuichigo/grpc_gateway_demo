# common files.
file(GLOB tmp *.cc)
set(grpc_gateway_demo_SRCS ${grpc_gateway_demo_SRCS} ${tmp})
# exclude executable
file (GLOB tmp main.cc)
exclude(grpc_gateway_demo_SRCS "${grpc_gateway_demo_SRCS}" ${tmp})

# Send the lists to the parent scope.
set(grpc_gateway_demo_SRCS ${grpc_gateway_demo_SRCS} PARENT_SCOPE)
