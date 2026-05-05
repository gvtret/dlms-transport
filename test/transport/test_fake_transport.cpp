#include "dlms/transport/fake_transport.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace {

using dlms::transport::FakeByteStream;
using dlms::transport::FakeDatagramTransport;
using dlms::transport::FakeTimerScheduler;
using dlms::transport::TransportDuration;
using dlms::transport::TransportStatus;

TEST(FakeByteStream, CloseIsIdempotent)
{
  FakeByteStream stream;

  EXPECT_EQ(TransportStatus::Ok, stream.Close());
  EXPECT_EQ(TransportStatus::Ok, stream.Open());
  EXPECT_TRUE(stream.IsOpen());
  EXPECT_EQ(TransportStatus::Ok, stream.Close());
  EXPECT_EQ(TransportStatus::Ok, stream.Close());
  EXPECT_FALSE(stream.IsOpen());
}

TEST(FakeByteStream, ReadBeforeOpenReturnsNotOpen)
{
  FakeByteStream stream;
  std::uint8_t output[1] = {};
  std::size_t bytesRead = 7;

  EXPECT_EQ(TransportStatus::NotOpen, stream.ReadSome(output, sizeof(output), bytesRead));
  EXPECT_EQ(0u, bytesRead);
}

TEST(FakeByteStream, WriteBeforeOpenReturnsNotOpen)
{
  FakeByteStream stream;
  const std::uint8_t input[] = { 0x01 };

  EXPECT_EQ(TransportStatus::NotOpen, stream.WriteAll(input, sizeof(input)));
}

TEST(FakeByteStream, ReadNullBufferReturnsInvalidArgument)
{
  FakeByteStream stream;
  std::size_t bytesRead = 7;

  ASSERT_EQ(TransportStatus::Ok, stream.Open());
  EXPECT_EQ(TransportStatus::InvalidArgument, stream.ReadSome(0, 1, bytesRead));
  EXPECT_EQ(0u, bytesRead);
}

TEST(FakeByteStream, WriteNullBufferReturnsInvalidArgument)
{
  FakeByteStream stream;

  ASSERT_EQ(TransportStatus::Ok, stream.Open());
  EXPECT_EQ(TransportStatus::InvalidArgument, stream.WriteAll(0, 1));
}

TEST(FakeByteStream, WriteAllEmptyInputSucceeds)
{
  FakeByteStream stream;

  ASSERT_EQ(TransportStatus::Ok, stream.Open());
  EXPECT_EQ(TransportStatus::Ok, stream.WriteAll(0, 0));
  ASSERT_EQ(1u, stream.Writes().size());
  EXPECT_TRUE(stream.Writes()[0].empty());
}

TEST(FakeByteStream, ReadScriptedChunks)
{
  FakeByteStream stream;
  std::uint8_t output[2] = {};
  std::size_t bytesRead = 0;

  ASSERT_EQ(TransportStatus::Ok, stream.Open());
  stream.ScriptRead(std::vector<std::uint8_t>{ 0x01, 0x02, 0x03 });

  EXPECT_EQ(TransportStatus::Ok, stream.ReadSome(output, sizeof(output), bytesRead));
  EXPECT_EQ(2u, bytesRead);
  EXPECT_EQ(0x01, output[0]);
  EXPECT_EQ(0x02, output[1]);

  output[0] = 0;
  output[1] = 0;
  EXPECT_EQ(TransportStatus::Ok, stream.ReadSome(output, sizeof(output), bytesRead));
  EXPECT_EQ(1u, bytesRead);
  EXPECT_EQ(0x03, output[0]);
}

TEST(FakeByteStream, WriteCapturesBytes)
{
  FakeByteStream stream;
  const std::uint8_t input[] = { 0x10, 0x20, 0x30 };

  ASSERT_EQ(TransportStatus::Ok, stream.Open());
  EXPECT_EQ(TransportStatus::Ok, stream.WriteAll(input, sizeof(input)));

  ASSERT_EQ(1u, stream.Writes().size());
  ASSERT_EQ(3u, stream.Writes()[0].size());
  EXPECT_EQ(0x10, stream.Writes()[0][0]);
  EXPECT_EQ(0x20, stream.Writes()[0][1]);
  EXPECT_EQ(0x30, stream.Writes()[0][2]);
}

TEST(FakeByteStream, ReadTimeout)
{
  FakeByteStream stream;
  std::uint8_t output[1] = {};
  std::size_t bytesRead = 7;

  ASSERT_EQ(TransportStatus::Ok, stream.Open());
  stream.ScriptNextReadStatus(TransportStatus::Timeout);

  EXPECT_EQ(TransportStatus::Timeout, stream.ReadSome(output, sizeof(output), bytesRead));
  EXPECT_EQ(0u, bytesRead);
  EXPECT_EQ(TransportStatus::WouldBlock, stream.ReadSome(output, sizeof(output), bytesRead));
}

TEST(FakeByteStream, PeerClose)
{
  FakeByteStream stream;
  std::uint8_t output[1] = {};
  std::size_t bytesRead = 7;

  ASSERT_EQ(TransportStatus::Ok, stream.Open());
  stream.ScriptNextReadStatus(TransportStatus::ConnectionClosed);

  EXPECT_EQ(TransportStatus::ConnectionClosed, stream.ReadSome(output, sizeof(output), bytesRead));
  EXPECT_EQ(0u, bytesRead);
}

TEST(FakeByteStream, ResetClearsState)
{
  FakeByteStream stream;
  const std::uint8_t input[] = { 0x01 };
  std::uint8_t output[1] = {};
  std::size_t bytesRead = 0;

  ASSERT_EQ(TransportStatus::Ok, stream.Open());
  stream.ScriptRead(std::vector<std::uint8_t>{ 0x02 });
  stream.ScriptNextReadStatus(TransportStatus::Timeout);
  stream.ScriptNextWriteStatus(TransportStatus::WriteFailed);
  EXPECT_EQ(TransportStatus::WriteFailed, stream.WriteAll(input, sizeof(input)));

  stream.Reset();

  EXPECT_FALSE(stream.IsOpen());
  EXPECT_TRUE(stream.Writes().empty());
  EXPECT_EQ(TransportStatus::NotOpen, stream.ReadSome(output, sizeof(output), bytesRead));
}

TEST(FakeDatagramTransport, CloseIsIdempotent)
{
  FakeDatagramTransport datagram;

  EXPECT_EQ(TransportStatus::Ok, datagram.Close());
  EXPECT_EQ(TransportStatus::Ok, datagram.Open());
  EXPECT_TRUE(datagram.IsOpen());
  EXPECT_EQ(TransportStatus::Ok, datagram.Close());
  EXPECT_EQ(TransportStatus::Ok, datagram.Close());
  EXPECT_FALSE(datagram.IsOpen());
}

TEST(FakeDatagramTransport, ReceiveBeforeOpenReturnsNotOpen)
{
  FakeDatagramTransport datagram;
  std::uint8_t output[1] = {};
  std::size_t bytesRead = 7;

  EXPECT_EQ(TransportStatus::NotOpen, datagram.Receive(output, sizeof(output), bytesRead));
  EXPECT_EQ(0u, bytesRead);
}

TEST(FakeDatagramTransport, SendBeforeOpenReturnsNotOpen)
{
  FakeDatagramTransport datagram;
  const std::uint8_t input[] = { 0x01 };

  EXPECT_EQ(TransportStatus::NotOpen, datagram.Send(input, sizeof(input)));
}

TEST(FakeDatagramTransport, ReceiveNullBufferReturnsInvalidArgument)
{
  FakeDatagramTransport datagram;
  std::size_t bytesRead = 7;

  ASSERT_EQ(TransportStatus::Ok, datagram.Open());
  EXPECT_EQ(TransportStatus::InvalidArgument, datagram.Receive(0, 1, bytesRead));
  EXPECT_EQ(0u, bytesRead);
}

TEST(FakeDatagramTransport, SendNullBufferReturnsInvalidArgument)
{
  FakeDatagramTransport datagram;

  ASSERT_EQ(TransportStatus::Ok, datagram.Open());
  EXPECT_EQ(TransportStatus::InvalidArgument, datagram.Send(0, 1));
}

TEST(FakeDatagramTransport, SendEmptyDatagramPolicyIsDocumented)
{
  FakeDatagramTransport datagram;

  ASSERT_EQ(TransportStatus::Ok, datagram.Open());
  EXPECT_EQ(TransportStatus::Ok, datagram.Send(0, 0));
  ASSERT_EQ(1u, datagram.SentDatagrams().size());
  EXPECT_TRUE(datagram.SentDatagrams()[0].empty());
}

TEST(FakeDatagramTransport, ReceiveScriptedDatagrams)
{
  FakeDatagramTransport datagram;
  std::uint8_t output[3] = {};
  std::size_t bytesRead = 0;

  ASSERT_EQ(TransportStatus::Ok, datagram.Open());
  datagram.ScriptReceive(std::vector<std::uint8_t>{ 0x11, 0x22 });

  EXPECT_EQ(TransportStatus::Ok, datagram.Receive(output, sizeof(output), bytesRead));
  EXPECT_EQ(2u, bytesRead);
  EXPECT_EQ(0x11, output[0]);
  EXPECT_EQ(0x22, output[1]);
  EXPECT_EQ(TransportStatus::WouldBlock, datagram.Receive(output, sizeof(output), bytesRead));
}

TEST(FakeDatagramTransport, SendCapturesDatagrams)
{
  FakeDatagramTransport datagram;
  const std::uint8_t input[] = { 0x10, 0x20, 0x30 };

  ASSERT_EQ(TransportStatus::Ok, datagram.Open());
  EXPECT_EQ(TransportStatus::Ok, datagram.Send(input, sizeof(input)));

  ASSERT_EQ(1u, datagram.SentDatagrams().size());
  ASSERT_EQ(3u, datagram.SentDatagrams()[0].size());
  EXPECT_EQ(0x10, datagram.SentDatagrams()[0][0]);
  EXPECT_EQ(0x20, datagram.SentDatagrams()[0][1]);
  EXPECT_EQ(0x30, datagram.SentDatagrams()[0][2]);
}

TEST(FakeDatagramTransport, ReceiveTimeout)
{
  FakeDatagramTransport datagram;
  std::uint8_t output[1] = {};
  std::size_t bytesRead = 7;

  ASSERT_EQ(TransportStatus::Ok, datagram.Open());
  datagram.ScriptNextReceiveStatus(TransportStatus::Timeout);

  EXPECT_EQ(TransportStatus::Timeout, datagram.Receive(output, sizeof(output), bytesRead));
  EXPECT_EQ(0u, bytesRead);
  EXPECT_EQ(TransportStatus::WouldBlock, datagram.Receive(output, sizeof(output), bytesRead));
}

TEST(FakeDatagramTransport, OutputBufferTooSmall)
{
  FakeDatagramTransport datagram;
  std::uint8_t smallOutput[1] = {};
  std::uint8_t output[2] = {};
  std::size_t bytesRead = 7;

  ASSERT_EQ(TransportStatus::Ok, datagram.Open());
  datagram.ScriptReceive(std::vector<std::uint8_t>{ 0x01, 0x02 });

  EXPECT_EQ(
    TransportStatus::OutputBufferTooSmall,
    datagram.Receive(smallOutput, sizeof(smallOutput), bytesRead));
  EXPECT_EQ(0u, bytesRead);

  EXPECT_EQ(TransportStatus::Ok, datagram.Receive(output, sizeof(output), bytesRead));
  EXPECT_EQ(2u, bytesRead);
  EXPECT_EQ(0x01, output[0]);
  EXPECT_EQ(0x02, output[1]);
}

TEST(FakeDatagramTransport, ResetClearsState)
{
  FakeDatagramTransport datagram;
  const std::uint8_t input[] = { 0x01 };
  std::uint8_t output[1] = {};
  std::size_t bytesRead = 0;

  ASSERT_EQ(TransportStatus::Ok, datagram.Open());
  datagram.ScriptReceive(std::vector<std::uint8_t>{ 0x02 });
  datagram.ScriptNextReceiveStatus(TransportStatus::Timeout);
  datagram.ScriptNextSendStatus(TransportStatus::WriteFailed);
  EXPECT_EQ(TransportStatus::WriteFailed, datagram.Send(input, sizeof(input)));

  datagram.Reset();

  EXPECT_FALSE(datagram.IsOpen());
  EXPECT_TRUE(datagram.SentDatagrams().empty());
  EXPECT_EQ(TransportStatus::NotOpen, datagram.Receive(output, sizeof(output), bytesRead));
}

TEST(FakeTimerScheduler, SleepForAdvancesMonotonicTime)
{
  FakeTimerScheduler timer;
  std::uint64_t now = 99;

  EXPECT_EQ(TransportStatus::Ok, timer.MonotonicMilliseconds(now));
  EXPECT_EQ(0u, now);

  EXPECT_EQ(TransportStatus::Ok, timer.SleepFor(TransportDuration{ 25 }));
  EXPECT_EQ(TransportStatus::Ok, timer.MonotonicMilliseconds(now));
  EXPECT_EQ(25u, now);
}

} // namespace
