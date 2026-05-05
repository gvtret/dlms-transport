#pragma once

#include "dlms/transport/transport_status.hpp"

#include <cstddef>
#include <cstdint>

namespace dlms {
namespace transport {

// Protocol-neutral datagram transport used by UDP-style profiles.
// One Send maps to one datagram; one Receive returns one datagram.
class IDatagramTransport
{
public:
  virtual ~IDatagramTransport() {}

  // Opens the underlying datagram endpoint.
  //
  // Returns Ok on success, AlreadyOpen when called twice, InvalidArgument for
  // invalid construction options, OpenFailed for endpoint/OS failures.
  virtual TransportStatus Open() = 0;

  // Closes the endpoint. Close is idempotent and returns Ok even when the
  // transport is already closed.
  virtual TransportStatus Close() = 0;

  // Returns true only after a successful Open and before Close.
  virtual bool IsOpen() const = 0;

  // Sends one caller-owned datagram.
  //
  // input may be null only when inputSize is zero. Expected statuses include
  // Ok, NotOpen, InvalidArgument, Timeout, WouldBlock, and WriteFailed.
  virtual TransportStatus Send(
    const std::uint8_t* input,
    std::size_t inputSize) = 0;

  // Receives one datagram into caller-owned output.
  //
  // bytesRead is set to zero before validation. output may be null only when
  // outputSize is zero. If the datagram does not fit, implementations should
  // return OutputBufferTooSmall without consuming it when the platform allows.
  // Expected statuses include Ok, NotOpen, InvalidArgument, Timeout,
  // WouldBlock, OutputBufferTooSmall, and ReadFailed.
  virtual TransportStatus Receive(
    std::uint8_t* output,
    std::size_t outputSize,
    std::size_t& bytesRead) = 0;
};

} // namespace transport
} // namespace dlms
