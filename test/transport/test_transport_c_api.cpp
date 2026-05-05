#include "dlms/transport/transport_c_api.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>

namespace {

dlms_transport_udp_options_t UdpReceiverOptions()
{
  dlms_transport_udp_options_t options;
  options.local_host = "127.0.0.1";
  options.local_port = 0u;
  options.remote_host = 0;
  options.remote_port = 0u;
  options.receive_timeout_ms = 500u;
  options.send_timeout_ms = 500u;
  return options;
}

dlms_transport_udp_options_t UdpSenderOptions(std::uint16_t remotePort)
{
  dlms_transport_udp_options_t options;
  options.local_host = "127.0.0.1";
  options.local_port = 0u;
  options.remote_host = "127.0.0.1";
  options.remote_port = remotePort;
  options.receive_timeout_ms = 500u;
  options.send_timeout_ms = 500u;
  return options;
}

dlms_transport_serial_options_t InvalidSerialOptions()
{
  dlms_transport_serial_options_t options;
#if defined(_WIN32)
  options.device_name = "\\\\.\\COM_DOES_NOT_EXIST_DLMS_TRANSPORT_C_API";
#else
  options.device_name = "/dev/dlms-transport-c-api-does-not-exist";
#endif
  options.baud_rate = 9600u;
  options.data_bits = 8u;
  options.parity = DLMS_TRANSPORT_SERIAL_PARITY_NONE;
  options.stop_bits = DLMS_TRANSPORT_SERIAL_STOP_BITS_ONE;
  options.read_timeout_ms = 500u;
  options.write_timeout_ms = 500u;
  return options;
}

TEST(TransportCApi, StatusValuesMatchStableAbi)
{
  EXPECT_EQ(0, DLMS_TRANSPORT_STATUS_OK);
  EXPECT_EQ(1, DLMS_TRANSPORT_STATUS_INVALID_ARGUMENT);
  EXPECT_EQ(2, DLMS_TRANSPORT_STATUS_NOT_OPEN);
  EXPECT_EQ(3, DLMS_TRANSPORT_STATUS_ALREADY_OPEN);
  EXPECT_EQ(4, DLMS_TRANSPORT_STATUS_OPEN_FAILED);
  EXPECT_EQ(10, DLMS_TRANSPORT_STATUS_OUTPUT_BUFFER_TOO_SMALL);
  EXPECT_EQ(12, DLMS_TRANSPORT_STATUS_INTERNAL_ERROR);
  EXPECT_STREQ("Ok", dlms_transport_status_to_string(DLMS_TRANSPORT_STATUS_OK));
}

TEST(TransportCApi, TimerLifecycle)
{
  dlms_transport_timer_t* timer = 0;
  std::uint64_t now = 99u;

  ASSERT_EQ(DLMS_TRANSPORT_STATUS_OK, dlms_transport_timer_create(&timer));
  ASSERT_NE(static_cast<dlms_transport_timer_t*>(0), timer);
  EXPECT_EQ(DLMS_TRANSPORT_STATUS_OK,
            dlms_transport_timer_monotonic_ms(timer, &now));
  EXPECT_EQ(DLMS_TRANSPORT_STATUS_OK,
            dlms_transport_timer_sleep_for(timer, 0u));
  dlms_transport_timer_destroy(timer);

  EXPECT_EQ(DLMS_TRANSPORT_STATUS_INVALID_ARGUMENT,
            dlms_transport_timer_create(0));
  EXPECT_EQ(DLMS_TRANSPORT_STATUS_INVALID_ARGUMENT,
            dlms_transport_timer_monotonic_ms(0, &now));
  dlms_transport_timer_destroy(0);
}

TEST(TransportCApi, TcpLifecycleAndInvalidArguments)
{
  dlms_transport_tcp_options_t options;
  options.host = "127.0.0.1";
  options.port = 0u;
  options.connect_timeout_ms = 1u;
  options.read_timeout_ms = 1u;
  options.write_timeout_ms = 1u;

  dlms_transport_tcp_t* tcp = 0;
  ASSERT_EQ(DLMS_TRANSPORT_STATUS_OK, dlms_transport_tcp_create(&options, &tcp));
  ASSERT_NE(static_cast<dlms_transport_tcp_t*>(0), tcp);
  EXPECT_EQ(DLMS_TRANSPORT_STATUS_INVALID_ARGUMENT, dlms_transport_tcp_open(tcp));
  EXPECT_EQ(0, dlms_transport_tcp_is_open(tcp));
  EXPECT_EQ(DLMS_TRANSPORT_STATUS_OK, dlms_transport_tcp_close(tcp));

  std::uint8_t output[1] = {};
  std::size_t bytesRead = 7u;
  EXPECT_EQ(DLMS_TRANSPORT_STATUS_NOT_OPEN,
            dlms_transport_tcp_read_some(tcp, output, sizeof(output), &bytesRead));
  EXPECT_EQ(0u, bytesRead);

  const std::uint8_t input[] = { 0x01u };
  EXPECT_EQ(DLMS_TRANSPORT_STATUS_NOT_OPEN,
            dlms_transport_tcp_write_all(tcp, input, sizeof(input)));
  dlms_transport_tcp_destroy(tcp);

  EXPECT_EQ(DLMS_TRANSPORT_STATUS_INVALID_ARGUMENT,
            dlms_transport_tcp_create(0, &tcp));
  dlms_transport_tcp_destroy(0);
}

TEST(TransportCApi, UdpLoopbackDatagram)
{
  dlms_transport_udp_t* receiver = 0;
  dlms_transport_udp_options_t receiverOptions = UdpReceiverOptions();
  ASSERT_EQ(DLMS_TRANSPORT_STATUS_OK,
            dlms_transport_udp_create(&receiverOptions, &receiver));
  ASSERT_EQ(DLMS_TRANSPORT_STATUS_OK, dlms_transport_udp_open(receiver));
  ASSERT_NE(0u, dlms_transport_udp_local_port(receiver));

  dlms_transport_udp_t* sender = 0;
  dlms_transport_udp_options_t senderOptions =
    UdpSenderOptions(dlms_transport_udp_local_port(receiver));
  ASSERT_EQ(DLMS_TRANSPORT_STATUS_OK,
            dlms_transport_udp_create(&senderOptions, &sender));
  ASSERT_EQ(DLMS_TRANSPORT_STATUS_OK, dlms_transport_udp_open(sender));

  const std::uint8_t input[] = { 0x01u, 0x02u, 0x03u };
  std::uint8_t output[4] = {};
  std::size_t bytesRead = 0u;

  EXPECT_EQ(DLMS_TRANSPORT_STATUS_OK,
            dlms_transport_udp_send(sender, input, sizeof(input)));
  EXPECT_EQ(DLMS_TRANSPORT_STATUS_OK,
            dlms_transport_udp_receive(receiver, output, sizeof(output), &bytesRead));
  ASSERT_EQ(3u, bytesRead);
  EXPECT_EQ(0x01u, output[0]);
  EXPECT_EQ(0x02u, output[1]);
  EXPECT_EQ(0x03u, output[2]);

  dlms_transport_udp_destroy(sender);
  dlms_transport_udp_destroy(receiver);
}

TEST(TransportCApi, UdpReportsSmallOutputBuffer)
{
  dlms_transport_udp_t* receiver = 0;
  dlms_transport_udp_options_t receiverOptions = UdpReceiverOptions();
  ASSERT_EQ(DLMS_TRANSPORT_STATUS_OK,
            dlms_transport_udp_create(&receiverOptions, &receiver));
  ASSERT_EQ(DLMS_TRANSPORT_STATUS_OK, dlms_transport_udp_open(receiver));

  dlms_transport_udp_t* sender = 0;
  dlms_transport_udp_options_t senderOptions =
    UdpSenderOptions(dlms_transport_udp_local_port(receiver));
  ASSERT_EQ(DLMS_TRANSPORT_STATUS_OK,
            dlms_transport_udp_create(&senderOptions, &sender));
  ASSERT_EQ(DLMS_TRANSPORT_STATUS_OK, dlms_transport_udp_open(sender));

  const std::uint8_t input[] = { 0x11u, 0x22u, 0x33u };
  std::uint8_t smallOutput[2] = {};
  std::size_t bytesRead = 99u;

  ASSERT_EQ(DLMS_TRANSPORT_STATUS_OK,
            dlms_transport_udp_send(sender, input, sizeof(input)));
  EXPECT_EQ(DLMS_TRANSPORT_STATUS_OUTPUT_BUFFER_TOO_SMALL,
            dlms_transport_udp_receive(receiver,
                                       smallOutput,
                                       sizeof(smallOutput),
                                       &bytesRead));
  EXPECT_EQ(0u, bytesRead);

  dlms_transport_udp_destroy(sender);
  dlms_transport_udp_destroy(receiver);
}

TEST(TransportCApi, SerialLifecycleAndInvalidDevice)
{
  dlms_transport_serial_t* serial = 0;
  dlms_transport_serial_options_t options = InvalidSerialOptions();

  ASSERT_EQ(DLMS_TRANSPORT_STATUS_OK,
            dlms_transport_serial_create(&options, &serial));
  ASSERT_NE(static_cast<dlms_transport_serial_t*>(0), serial);
  EXPECT_EQ(DLMS_TRANSPORT_STATUS_OPEN_FAILED,
            dlms_transport_serial_open(serial));
  EXPECT_EQ(DLMS_TRANSPORT_STATUS_OK, dlms_transport_serial_close(serial));
  EXPECT_EQ(0, dlms_transport_serial_is_open(serial));

  std::uint8_t output[1] = {};
  std::size_t bytesRead = 7u;
  EXPECT_EQ(DLMS_TRANSPORT_STATUS_NOT_OPEN,
            dlms_transport_serial_read_some(serial,
                                            output,
                                            sizeof(output),
                                            &bytesRead));
  EXPECT_EQ(0u, bytesRead);

  const std::uint8_t input[] = { 0x01u };
  EXPECT_EQ(DLMS_TRANSPORT_STATUS_NOT_OPEN,
            dlms_transport_serial_write_all(serial, input, sizeof(input)));
  dlms_transport_serial_destroy(serial);

  options.parity = static_cast<dlms_transport_serial_parity_t>(99);
  EXPECT_EQ(DLMS_TRANSPORT_STATUS_INVALID_ARGUMENT,
            dlms_transport_serial_create(&options, &serial));
  dlms_transport_serial_destroy(0);
}

} // namespace
