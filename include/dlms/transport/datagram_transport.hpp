#pragma once

#include "dlms/transport/transport_status.hpp"

#include <cstddef>
#include <cstdint>

namespace dlms {
namespace transport {

class IDatagramTransport
{
public:
  virtual ~IDatagramTransport() {}

  virtual TransportStatus Open() = 0;
  virtual TransportStatus Close() = 0;
  virtual bool IsOpen() const = 0;

  virtual TransportStatus Send(
    const std::uint8_t* input,
    std::size_t inputSize) = 0;

  virtual TransportStatus Receive(
    std::uint8_t* output,
    std::size_t outputSize,
    std::size_t& bytesRead) = 0;
};

} // namespace transport
} // namespace dlms
