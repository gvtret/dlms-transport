cmake_minimum_required(VERSION 3.16)

project(dlms_transport
  VERSION 0.1.0
  LANGUAGES C CXX)

option(DLMS_BUILD_TESTS "Build GoogleTest tests" ON)
option(DLMS_BUILD_C_API "Build stable C ABI wrapper" ON)
option(DLMS_USE_SYSTEM_GTEST "Use system-installed GoogleTest" OFF)

add_library(dlms_transport
  src/transport/iec62056_21_mode_e.cpp
  src/transport/serial_port_discovery.cpp
  src/transport/serial_transport.cpp
  src/transport/tcp_server_transport.cpp
  src/transport/tcp_stream_transport.cpp
  src/transport/tls_stream_transport.cpp
  src/transport/udp_transport.cpp
  src/transport/timer_scheduler.cpp)

if(DLMS_BUILD_C_API)
  target_sources(dlms_transport
    PRIVATE
      src/transport/transport_c_api.cpp)
endif()

target_compile_features(dlms_transport PUBLIC cxx_std_11)

target_include_directories(dlms_transport
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(WIN32)
  target_link_libraries(dlms_transport
    PUBLIC
      ws2_32)
endif()

set(DLMS_TRANSPORT_HAS_API ON CACHE INTERNAL
  "dlms-transport exposes the transport API required by upper-layer integration tests")

if(DLMS_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt")
  enable_testing()
  add_subdirectory(test)
endif()
