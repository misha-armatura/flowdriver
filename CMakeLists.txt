cmake_minimum_required(VERSION 3.16)

project(flowdriver VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Dependencies
find_package(Qt6 REQUIRED COMPONENTS Quick Core Network WebSockets)
find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(absl REQUIRED COMPONENTS
    base
    strings
    str_format
    cord
    time
)
find_package(Boost REQUIRED COMPONENTS 
    system    # Required for Beast
    url       # For URL parsing
)
find_package(OpenSSL REQUIRED)  # Required by Beast SSL
find_package(nlohmann_json REQUIRED)

# Generate protobuf and gRPC code
set(PROTO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/proto/flowdriver.proto")
set(PROTO_SRC_DIR "${CMAKE_CURRENT_BINARY_DIR}")

# Use system gRPC plugin
find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)

add_custom_command(
    OUTPUT 
        "${PROTO_SRC_DIR}/flowdriver.pb.cc"
        "${PROTO_SRC_DIR}/flowdriver.pb.h"
        "${PROTO_SRC_DIR}/flowdriver.grpc.pb.cc"
        "${PROTO_SRC_DIR}/flowdriver.grpc.pb.h"
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
        --grpc_out="${PROTO_SRC_DIR}"
        --cpp_out="${PROTO_SRC_DIR}"
        -I "${CMAKE_CURRENT_SOURCE_DIR}/proto"
        --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
        "${PROTO_FILES}"
    DEPENDS "${PROTO_FILES}"
)

# Create library for generated protobuf code
add_library(flowdriver_proto
    ${PROTO_SRC_DIR}/flowdriver.pb.cc
    ${PROTO_SRC_DIR}/flowdriver.grpc.pb.cc
)

target_link_libraries(flowdriver_proto
    PUBLIC
    protobuf::libprotobuf
    gRPC::grpc++
    grpc++_reflection
    absl::base
    absl::strings
    absl::str_format
    absl::cord
    absl::time
)

target_include_directories(flowdriver_proto
    PUBLIC
    ${PROTO_SRC_DIR}
)

# Add the cmake modules directory to the search path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Now find ZeroMQ
find_package(ZeroMQ QUIET)

if(NOT ZeroMQ_FOUND)
    # Try pkg-config if find_package fails
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(ZMQ REQUIRED libzmq>=4.0.0)
endif()

# Core library
add_library(flowdriver_core
    include/core/protocol_handler.hpp
    include/core/rest_handler.hpp
    include/core/websocket_handler.hpp
    include/core/zeromq_handler.hpp
    #include/core/zeromq_handler_impl.hpp
    include/core/grpc_handler.hpp
    include/core/types.hpp
    include/core/auth_manager.hpp
    src/core/rest_handler.cpp
    src/core/protocol_handler.cpp
    src/core/websocket_handler.cpp
    src/core/zeromq_handler.cpp
    #src/core/zeromq_handler_impl.cpp
    src/core/grpc_handler.cpp
    src/core/auth_manager.cpp
)

target_link_libraries(flowdriver_core
    PUBLIC
    flowdriver_proto
    zmq
    Qt6::Core
    Qt6::Network
    Qt6::WebSockets
    Qt6::Quick
    Qt6::Qml
    Boost::system
    Boost::url
    OpenSSL::SSL    # Beast SSL needs this
    OpenSSL::Crypto # Beast SSL needs this
)

target_include_directories(flowdriver_core
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${PROTO_SRC_DIR}
    ${ZeroMQ_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Add link directories if needed
link_directories(${ZeroMQ_LIBRARY_DIRS})

# UI Models library
add_library(flowdriver_models
    include/models/request_manager.hpp
    include/models/body_model.hpp
    include/models/headers_model.hpp
    include/models/query_model.hpp
    include/models/response_model.hpp
    include/models/auth_model.hpp
    src/models/request_manager.cpp
    src/models/body_model.cpp
    src/models/headers_model.cpp
    src/models/query_model.cpp
    src/models/response_model.cpp
    src/models/auth_model.cpp
)

target_include_directories(flowdriver_models
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(flowdriver_models
    PUBLIC
    flowdriver_core
    flowdriver_proto
)

# Main executable
add_executable(flowdriver
    src/main.cpp
    qml/qml.qrc
)

target_include_directories(flowdriver
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR} 
)

target_link_libraries(flowdriver
    PRIVATE
    flowdriver_models  
    flowdriver_core    
    flowdriver_proto   
    Qt6::Core
    Qt6::Quick
    Qt6::WebSockets
    Qt6::Qml
    protobuf::libprotobuf
    gRPC::grpc++
    zmq
    nlohmann_json::nlohmann_json
)

# QML module registration
qt_add_qml_module(flowdriver
    URI FlowDriver.UI
    VERSION 1.0
    QML_FILES
        qml/main.qml
        qml/RequestBuilder.qml
        qml/ResponseViewer.qml
        qml/HeadersEditor.qml
        qml/HeaderRow.qml
        qml/BodyEditor.qml
        qml/QueryEditor.qml
        qml/QueryParamRow.qml
        qml/AuthEditor.qml
    RESOURCES
        qml/qml.qrc
)
