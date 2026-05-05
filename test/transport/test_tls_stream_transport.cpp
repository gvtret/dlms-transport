#include "dlms/transport/fake_transport.hpp"
#include "dlms/transport/tls_stream_transport.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>

namespace {

using dlms::transport::FakeByteStream;
using dlms::transport::IByteStream;
using dlms::transport::ITlsStreamBackend;
using dlms::transport::TlsStreamTransport;
using dlms::transport::TlsStreamTransportOptions;
using dlms::transport::TransportStatus;
using dlms::transport::UnsupportedTlsStreamBackend;

class FakeTlsBackend : public ITlsStreamBackend
{
public:
  FakeTlsBackend()
    : handshakeStatus(TransportStatus::Ok)
    , closeCount(0)
    , handshakeCount(0)
  {
  }

  TransportStatus Handshake(
    IByteStream&,
    const TlsStreamTransportOptions&)
  {
    ++handshakeCount;
    return handshakeStatus;
  }

  TransportStatus Close(IByteStream& lower)
  {
    ++closeCount;
    return lower.Close();
  }

  TransportStatus ReadSome(
    IByteStream& lower,
    std::uint8_t* output,
    std::size_t outputSize,
    std::size_t& bytesRead)
  {
    return lower.ReadSome(output, outputSize, bytesRead);
  }

  TransportStatus WriteAll(
    IByteStream& lower,
    const std::uint8_t* input,
    std::size_t inputSize)
  {
    return lower.WriteAll(input, inputSize);
  }

  TransportStatus handshakeStatus;
  int closeCount;
  int handshakeCount;
};

TlsStreamTransportOptions Options()
{
  TlsStreamTransportOptions options;
  options.serverName = "meter.example";
  options.verifyPeer = true;
  return options;
}

TEST(TlsStreamTransport, InvalidConfigurationReturnsInvalidArgument)
{
  FakeByteStream lower;
  FakeTlsBackend backend;
  TlsStreamTransportOptions options = Options();

  TlsStreamTransport missingLower(0, &backend, options);
  EXPECT_EQ(TransportStatus::InvalidArgument, missingLower.Open());

  TlsStreamTransport missingBackend(&lower, 0, options);
  EXPECT_EQ(TransportStatus::InvalidArgument, missingBackend.Open());

  TlsStreamTransportOptions missingName = options;
  missingName.serverName.clear();
  TlsStreamTransport missingServerName(&lower, &backend, missingName);
  EXPECT_EQ(TransportStatus::InvalidArgument, missingServerName.Open());
}

TEST(TlsStreamTransport, WrapsByteStreamLifecycle)
{
  FakeByteStream lower;
  FakeTlsBackend backend;
  TlsStreamTransport transport(&lower, &backend, Options());

  ASSERT_EQ(TransportStatus::Ok, transport.Open());
  EXPECT_TRUE(transport.IsOpen());
  EXPECT_EQ(1, backend.handshakeCount);

  const std::uint8_t request[] = { 0x01, 0x02 };
  EXPECT_EQ(TransportStatus::Ok, transport.WriteAll(request, sizeof(request)));
  ASSERT_EQ(1u, lower.Writes().size());
  ASSERT_EQ(2u, lower.Writes()[0].size());
  EXPECT_EQ(0x01, lower.Writes()[0][0]);
  EXPECT_EQ(0x02, lower.Writes()[0][1]);

  lower.ScriptRead(std::vector<std::uint8_t>{ 0xA1, 0xB2 });
  std::uint8_t output[4] = {};
  std::size_t bytesRead = 0;
  EXPECT_EQ(TransportStatus::Ok, transport.ReadSome(output, sizeof(output), bytesRead));
  EXPECT_EQ(2u, bytesRead);
  EXPECT_EQ(0xA1, output[0]);
  EXPECT_EQ(0xB2, output[1]);

  EXPECT_EQ(TransportStatus::Ok, transport.Close());
  EXPECT_FALSE(transport.IsOpen());
  EXPECT_EQ(1, backend.closeCount);
}

TEST(TlsStreamTransport, ReportsHandshakeFailure)
{
  FakeByteStream lower;
  FakeTlsBackend backend;
  backend.handshakeStatus = TransportStatus::OpenFailed;
  TlsStreamTransport transport(&lower, &backend, Options());

  EXPECT_EQ(TransportStatus::OpenFailed, transport.Open());
  EXPECT_FALSE(transport.IsOpen());
  EXPECT_FALSE(lower.IsOpen());
  EXPECT_EQ(1, backend.handshakeCount);
}

TEST(TlsStreamTransport, UnsupportedBackendReportsUnsupportedFeature)
{
  FakeByteStream lower;
  UnsupportedTlsStreamBackend backend;
  TlsStreamTransport transport(&lower, &backend, Options());

  EXPECT_EQ(TransportStatus::UnsupportedFeature, transport.Open());
  EXPECT_FALSE(transport.IsOpen());
  EXPECT_FALSE(lower.IsOpen());
}

} // namespace
