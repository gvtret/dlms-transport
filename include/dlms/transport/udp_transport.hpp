#pragma once

#include "dlms/transport/datagram_transport.hpp"
#include "dlms/transport/timer_scheduler.hpp"

#include <cstdint>
#include <string>

namespace dlms {
namespace transport {

struct UdpTransportOptions
{
  std::string localHost;
  std::uint16_t localPort;
  std::string remoteHost;
  std::uint16_t remotePort;
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

class UdpTransport : public IDatagramTransport
{
public:
  explicit UdpTransport(const UdpTransportOptions& options);
  ~UdpTransport();

  TransportStatus Open();
  TransportStatus Close();
  bool IsOpen() const;

  TransportStatus Send(
    const std::uint8_t* input,
    std::size_t inputSize);

  TransportStatus Receive(
    std::uint8_t* output,
    std::size_t outputSize,
    std::size_t& bytesRead);

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
