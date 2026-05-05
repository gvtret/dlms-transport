#pragma once

#include "dlms/transport/transport_status.hpp"

#include <cstddef>
#include <cstdint>

namespace dlms {
namespace transport {

class IByteStream
{
public:
  virtual ~IByteStream() {}

  virtual TransportStatus Open() = 0;
  virtual TransportStatus Close() = 0;
  virtual bool IsOpen() const = 0;

  virtual TransportStatus ReadSome(
    std::uint8_t* output,
    std::size_t outputSize,
    std::size_t& bytesRead) = 0;

  virtual TransportStatus WriteAll(
    const std::uint8_t* input,
    std::size_t inputSize) = 0;
};

} // namespace transport
} // namespace dlms
