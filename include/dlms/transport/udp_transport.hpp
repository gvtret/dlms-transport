#pragma once

#include "dlms/transport/datagram_transport.hpp"
#include "dlms/transport/timer_scheduler.hpp"

#include <cstdint>
#include <string>

namespace dlms {
namespace transport {

struct UdpTransportOptions
{
  // Local bind address. Use "127.0.0.1" for loopback-only tests.
  std::string localHost;

  // Local UDP port. Zero asks the OS to choose an ephemeral port.
  std::uint16_t localPort;

  // Optional remote endpoint. Send requires both remoteHost and remotePort.
  std::string remoteHost;
  std::uint16_t remotePort;

  // Timeouts used for receive and send readiness.
  TransportDuration receiveTimeout;
  TransportDuration sendTimeout;

  UdpTransportOptions()
    : localHost("0.0.0.0")
    , localPort(0)
    , remotePort(0)
    , receiveTimeout{ 5000 }
    , sendTimeout{ 5000 }
  {
  }
};

// UDP datagram transport. Wrapper wPort and length fields are handled above.
class UdpTransport : public IDatagramTransport
{
public:
  explicit UdpTransport(const UdpTransportOptions& options);
  ~UdpTransport();

  // Returns Ok, AlreadyOpen, InvalidArgument, or OpenFailed.
  TransportStatus Open();

  // Idempotent; returns Ok.
  TransportStatus Close();
  bool IsOpen() const;

  TransportStatus Send(
    const std::uint8_t* input,
    std::size_t inputSize);

  TransportStatus Receive(
    std::uint8_t* output,
    std::size_t outputSize,
    std::size_t& bytesRead);

  // Returns the bound local port after Open, or zero when closed/unavailable.
  std::uint16_t LocalPort() const;

private:
  UdpTransport(const UdpTransport&);
  UdpTransport& operator=(const UdpTransport&);

  UdpTransportOptions options_;
  intptr_t socket_;
  bool open_;
  bool remoteConfigured_;
};

} // namespace transport
} // namespace dlms
