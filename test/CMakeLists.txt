if(DLMS_USE_SYSTEM_GTEST)
  find_package(GTest REQUIRED)
elseif(NOT TARGET GTest::gtest_main)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
  FetchContent_MakeAvailable(googletest)
endif()

set(DLMS_TRANSPORT_TEST_SOURCES
  transport/test_fake_transport.cpp
  transport/test_serial_transport.cpp
  transport/test_tcp_server_transport.cpp
  transport/test_tcp_stream_transport.cpp
  transport/test_timer_scheduler.cpp
  transport/test_transport_status.cpp
  transport/test_udp_transport.cpp)

if(DLMS_BUILD_C_API)
  list(APPEND DLMS_TRANSPORT_TEST_SOURCES
    transport/test_transport_c_api.cpp
    transport/test_transport_c_header.c)
endif()

add_executable(dlms_transport_tests
  ${DLMS_TRANSPORT_TEST_SOURCES})

target_link_libraries(dlms_transport_tests
  PRIVATE
    dlms_transport
    GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(dlms_transport_tests)
