#include "dlms/transport/udp_transport.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>

namespace {

using dlms::transport::TransportDuration;
using dlms::transport::TransportStatus;
using dlms::transport::UdpTransport;
using dlms::transport::UdpTransportOptions;

UdpTransportOptions ReceiverOptions()
{
  UdpTransportOptions options;
  options.localHost = "127.0.0.1";
  options.localPort = 0;
  options.receiveTimeout = TransportDuration{ 500 };
  options.sendTimeout = TransportDuration{ 500 };
  return options;
}

UdpTransportOptions SenderOptions(std::uint16_t remotePort)
{
  UdpTransportOptions options;
  options.localHost = "127.0.0.1";
  options.localPort = 0;
  options.remoteHost = "127.0.0.1";
  options.remotePort = remotePort;
  options.receiveTimeout = TransportDuration{ 500 };
  options.sendTimeout = TransportDuration{ 500 };
  return options;
}

TEST(UdpTransport, OpenLoopback)
{
  UdpTransport receiver(ReceiverOptions());

  EXPECT_EQ(TransportStatus::Ok, receiver.Open());
  EXPECT_TRUE(receiver.IsOpen());
  EXPECT_NE(0u, receiver.LocalPort());
  EXPECT_EQ(TransportStatus::Ok, receiver.Close());
}

TEST(UdpTransport, SendReceiveLoopbackDatagram)
{
  UdpTransport receiver(ReceiverOptions());
  ASSERT_EQ(TransportStatus::Ok, receiver.Open());

  UdpTransport sender(SenderOptions(receiver.LocalPort()));
  ASSERT_EQ(TransportStatus::Ok, sender.Open());

  const std::uint8_t request[] = { 0x01, 0x02, 0x03, 0x04 };
  std::uint8_t output[8] = {};
  std::size_t bytesRead = 0;

  EXPECT_EQ(TransportStatus::Ok, sender.Send(request, sizeof(request)));
  EXPECT_EQ(TransportStatus::Ok, receiver.Receive(output, sizeof(output), bytesRead));
  ASSERT_EQ(4u, bytesRead);
  EXPECT_EQ(0x01, output[0]);
  EXPECT_EQ(0x02, output[1]);
  EXPECT_EQ(0x03, output[2]);
  EXPECT_EQ(0x04, output[3]);
}

TEST(UdpTransport, PreservesDatagramBoundary)
{
  UdpTransport receiver(ReceiverOptions());
  ASSERT_EQ(TransportStatus::Ok, receiver.Open());

  UdpTransport sender(SenderOptions(receiver.LocalPort()));
  ASSERT_EQ(TransportStatus::Ok, sender.Open());

  const std::uint8_t first[] = { 0xA1, 0xB2 };
  const std::uint8_t second[] = { 0xC3, 0xD4, 0xE5 };
  std::uint8_t output[8] = {};
  std::size_t bytesRead = 0;

  ASSERT_EQ(TransportStatus::Ok, sender.Send(first, sizeof(first)));
  ASSERT_EQ(TransportStatus::Ok, sender.Send(second, sizeof(second)));

  EXPECT_EQ(TransportStatus::Ok, receiver.Receive(output, sizeof(output), bytesRead));
  ASSERT_EQ(2u, bytesRead);
  EXPECT_EQ(0xA1, output[0]);
  EXPECT_EQ(0xB2, output[1]);

  EXPECT_EQ(TransportStatus::Ok, receiver.Receive(output, sizeof(output), bytesRead));
  ASSERT_EQ(3u, bytesRead);
  EXPECT_EQ(0xC3, output[0]);
  EXPECT_EQ(0xD4, output[1]);
  EXPECT_EQ(0xE5, output[2]);
}

TEST(UdpTransport, ReceiveBufferTooSmallDoesNotConsumeDatagram)
{
  UdpTransport receiver(ReceiverOptions());
  ASSERT_EQ(TransportStatus::Ok, receiver.Open());

  UdpTransport sender(SenderOptions(receiver.LocalPort()));
  ASSERT_EQ(TransportStatus::Ok, sender.Open());

  const std::uint8_t request[] = { 0x11, 0x22, 0x33 };
  std::uint8_t smallOutput[2] = {};
  std::uint8_t output[3] = {};
  std::size_t bytesRead = 7;

  ASSERT_EQ(TransportStatus::Ok, sender.Send(request, sizeof(request)));
  EXPECT_EQ(
    TransportStatus::OutputBufferTooSmall,
    receiver.Receive(smallOutput, sizeof(smallOutput), bytesRead));
  EXPECT_EQ(0u, bytesRead);

  EXPECT_EQ(TransportStatus::Ok, receiver.Receive(output, sizeof(output), bytesRead));
  ASSERT_EQ(3u, bytesRead);
  EXPECT_EQ(0x11, output[0]);
  EXPECT_EQ(0x22, output[1]);
  EXPECT_EQ(0x33, output[2]);
}

TEST(UdpTransport, ReceiveTimeout)
{
  UdpTransportOptions options = ReceiverOptions();
  options.receiveTimeout = TransportDuration{ 1 };
  UdpTransport receiver(options);
  std::uint8_t output[1] = {};
  std::size_t bytesRead = 0;

  ASSERT_EQ(TransportStatus::Ok, receiver.Open());
  EXPECT_EQ(TransportStatus::Timeout, receiver.Receive(output, sizeof(output), bytesRead));
}

TEST(UdpTransport, CloseIsIdempotent)
{
  UdpTransport receiver(ReceiverOptions());

  EXPECT_EQ(TransportStatus::Ok, receiver.Close());
  EXPECT_EQ(TransportStatus::Ok, receiver.Close());
}

TEST(UdpTransport, SendWithoutRemoteIsInvalid)
{
  UdpTransport receiver(ReceiverOptions());
  const std::uint8_t request[] = { 0x01 };

  ASSERT_EQ(TransportStatus::Ok, receiver.Open());
  EXPECT_EQ(TransportStatus::InvalidArgument, receiver.Send(request, sizeof(request)));
}

} // namespace
