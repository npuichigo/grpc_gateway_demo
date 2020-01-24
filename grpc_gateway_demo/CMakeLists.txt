set(grpc_gateway_demo_SRCS)

# Add respective subdirectories
add_subdirectory(serving)

# Generated protobuf sources
get_filename_component(proto "proto/demo_service.proto" ABSOLUTE)
get_filename_component(proto_path "${proto}" PATH)

set(proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/serving/demo_service.pb.cc")
set(proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/serving/demo_service.pb.h")
set(grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/serving/demo_service.grpc.pb.cc")
set(grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/serving/demo_service.grpc.pb.h")

add_custom_command(
  OUTPUT "${proto_srcs}" "${proto_hdrs}" "${grpc_srcs}" "${grpc_hdrs}"
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}"
  COMMAND ${_PROTOBUF_PROTOC}
  ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}/serving"
    --cpp_out "${CMAKE_CURRENT_BINARY_DIR}/serving"
    -I "${proto_path}"
    -I "/usr/local/include"
    -I "${PROJECT_SOURCE_DIR}/third_party/googleapis"
   --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
    "${proto}"
  DEPENDS "${proto}")

# Compile exposed libraries.
list(APPEND google_proto_srcs "${PROJECT_SOURCE_DIR}/third_party/googleapis/gens/google/api/annotations.pb.cc")
list(APPEND google_proto_srcs "${PROJECT_SOURCE_DIR}/third_party/googleapis/gens/google/api/httpbody.pb.cc")
list(APPEND google_proto_srcs "${PROJECT_SOURCE_DIR}/third_party/googleapis/gens/google/api/http.pb.cc")
add_library(grpc_gateway_demo ${grpc_gateway_demo_SRCS} ${proto_srcs} ${grpc_srcs} ${google_proto_srcs})
target_link_libraries(grpc_gateway_demo ${grpc_gateway_demo_DEPENDENCY_LIBS})
list(APPEND grpc_gateway_demo_MAIN_LIBS -Wl,--whole-archive grpc_gateway_demo -Wl,--no-whole-archive)

add_executable(grpc_server serving/main.cc)
target_link_libraries(grpc_server ${grpc_gateway_demo_MAIN_LIBS})

#add_executable(grpc_test_client serving/grpc_test_client.cc)
#target_link_libraries(grpc_test_client ${grpc_gateway_demo_MAIN_LIBS})
