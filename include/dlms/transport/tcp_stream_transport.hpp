#pragma once

#include "dlms/transport/byte_stream.hpp"
#include "dlms/transport/timer_scheduler.hpp"

#include <cstdint>
#include <string>

namespace dlms {
namespace transport {

struct TcpStreamTransportOptions
{
  // Numeric address or DNS name owned by the caller; copied into options.
  std::string host;

  // Remote TCP port. Port zero is invalid for Open.
  std::uint16_t port;

  // Timeouts used for connect, read readiness, and write readiness.
  TransportDuration connectTimeout;
  TransportDuration readTimeout;
  TransportDuration writeTimeout;

  TcpStreamTransportOptions()
    : port(0)
    , connectTimeout{ 5000 }
    , readTimeout{ 5000 }
    , writeTimeout{ 5000 }
  {
  }
};

// TCP client byte stream. This class does not parse wrapper headers or APDUs.
class TcpStreamTransport : public IByteStream
{
public:
  explicit TcpStreamTransport(const TcpStreamTransportOptions& options);
  ~TcpStreamTransport();

  // Returns Ok, AlreadyOpen, InvalidArgument, Timeout, or OpenFailed.
  TransportStatus Open();

  // Idempotent; returns Ok.
  TransportStatus Close();
  bool IsOpen() const;

  TransportStatus ReadSome(
    std::uint8_t* output,
    std::size_t outputSize,
    std::size_t& bytesRead);

  TransportStatus WriteAll(
    const std::uint8_t* input,
    std::size_t inputSize);

private:
  TcpStreamTransport(const TcpStreamTransport&);
  TcpStreamTransport& operator=(const TcpStreamTransport&);

  TcpStreamTransportOptions options_;
  intptr_t socket_;
  bool open_;
};

} // namespace transport
} // namespace dlms
