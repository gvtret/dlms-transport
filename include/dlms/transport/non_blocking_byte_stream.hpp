#pragma once

#include "dlms/transport/transport_status.hpp"

#include <cstddef>
#include <cstdint>

namespace dlms {
namespace transport {

class INonBlockingByteStream
{
public:
  virtual ~INonBlockingByteStream() {}

  virtual TransportStatus Open() = 0;
  virtual TransportStatus Close() = 0;
  virtual bool IsOpen() const = 0;

  // Returns WouldBlock when no bytes are currently available.
  virtual TransportStatus TryReadSome(
    std::uint8_t* output,
    std::size_t outputSize,
    std::size_t& bytesRead) = 0;

  // Returns WouldBlock when zero bytes can currently be written.
  virtual TransportStatus TryWriteSome(
    const std::uint8_t* input,
    std::size_t inputSize,
    std::size_t& bytesWritten) = 0;
};

} // namespace transport
} // namespace dlms
