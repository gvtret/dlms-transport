#pragma once

#include "dlms/transport/transport_status.hpp"

#include <cstddef>
#include <cstdint>

namespace dlms {
namespace transport {

class INonBlockingDatagramTransport
{
public:
  virtual ~INonBlockingDatagramTransport() {}

  virtual TransportStatus Open() = 0;
  virtual TransportStatus Close() = 0;
  virtual bool IsOpen() const = 0;

  // Returns WouldBlock when the datagram cannot currently be sent.
  virtual TransportStatus TrySend(
    const std::uint8_t* input,
    std::size_t inputSize) = 0;

  // Returns WouldBlock when no complete datagram is currently available.
  virtual TransportStatus TryReceive(
    std::uint8_t* output,
    std::size_t outputSize,
    std::size_t& bytesRead) = 0;
};

} // namespace transport
} // namespace dlms
