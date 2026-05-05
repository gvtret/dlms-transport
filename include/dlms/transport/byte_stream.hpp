#pragma once

#include "dlms/transport/transport_status.hpp"

#include <cstddef>
#include <cstdint>

namespace dlms {
namespace transport {

// Protocol-neutral byte stream used by TCP and serial transports.
// Implementations own the underlying OS handle after Open succeeds.
class IByteStream
{
public:
  virtual ~IByteStream() {}

  // Opens the underlying byte stream endpoint.
  //
  // Returns Ok on success, AlreadyOpen when called twice, InvalidArgument for
  // invalid construction options, OpenFailed for endpoint/OS failures.
  virtual TransportStatus Open() = 0;

  // Closes the endpoint. Close is idempotent and returns Ok even when the
  // stream is already closed.
  virtual TransportStatus Close() = 0;

  // Returns true only after a successful Open and before Close or a peer-close
  // condition observed by the implementation.
  virtual bool IsOpen() const = 0;

  // Reads up to outputSize bytes into caller-owned output.
  //
  // bytesRead is set to zero before validation. output may be null only when
  // outputSize is zero. Expected statuses include Ok, NotOpen,
  // InvalidArgument, Timeout, WouldBlock, ReadFailed, and ConnectionClosed.
  virtual TransportStatus ReadSome(
    std::uint8_t* output,
    std::size_t outputSize,
    std::size_t& bytesRead) = 0;

  // Writes exactly inputSize bytes from caller-owned input before returning Ok.
  //
  // input may be null only when inputSize is zero. Expected statuses include
  // Ok, NotOpen, InvalidArgument, Timeout, WouldBlock, and WriteFailed.
  virtual TransportStatus WriteAll(
    const std::uint8_t* input,
    std::size_t inputSize) = 0;
};

} // namespace transport
} // namespace dlms
