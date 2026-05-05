#pragma once

#include "dlms/transport/byte_stream.hpp"
#include "dlms/transport/timer_scheduler.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace dlms {
namespace transport {

struct TcpServerTransportOptions
{
  // Numeric address or DNS name owned by the caller; copied into options.
  // Empty host binds on all local interfaces.
  std::string host;

  // Local TCP port. Port zero requests an ephemeral loopback/listen port.
  std::uint16_t port;

  // listen(2) backlog. Zero is invalid for Open.
  int backlog;

  // Timeouts used for accept, accepted connection read, and write readiness.
  TransportDuration acceptTimeout;
  TransportDuration readTimeout;
  TransportDuration writeTimeout;

  TcpServerTransportOptions()
    : port(0)
    , backlog(1)
    , acceptTimeout{ 5000 }
    , readTimeout{ 5000 }
    , writeTimeout{ 5000 }
  {
  }
};

// TCP server byte-stream listener. This class does not parse wrapper headers or APDUs.
class TcpServerTransport
{
public:
  explicit TcpServerTransport(const TcpServerTransportOptions& options);
  ~TcpServerTransport();

  // Returns Ok, AlreadyOpen, InvalidArgument, or OpenFailed.
  TransportStatus Open();

  // Idempotent; returns Ok.
  TransportStatus Close();
  bool IsOpen() const;

  // Returns the bound local port after Open, or zero when closed/not bound.
  std::uint16_t LocalPort() const;

  // Returns Ok, InvalidArgument, NotOpen, Timeout, or OpenFailed.
  TransportStatus Accept(std::unique_ptr<IByteStream>& connection);

private:
  TcpServerTransport(const TcpServerTransport&);
  TcpServerTransport& operator=(const TcpServerTransport&);

  TcpServerTransportOptions options_;
  intptr_t socket_;
  bool open_;
  std::uint16_t localPort_;
};

} // namespace transport
} // namespace dlms
