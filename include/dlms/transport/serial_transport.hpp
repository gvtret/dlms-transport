#pragma once

#include "dlms/transport/byte_stream.hpp"
#include "dlms/transport/timer_scheduler.hpp"

#include <cstdint>
#include <string>

namespace dlms {
namespace transport {

enum class SerialParity
{
  None,
  Even,
  Odd
};

enum class SerialStopBits
{
  One,
  Two
};

struct SerialTransportOptions
{
  // Platform device name, for example "\\\\.\\COM3" on Windows or
  // "/dev/ttyUSB0" on POSIX. The string is copied into options.
  std::string deviceName;

  // Basic line settings. Mode switching, discovery, and friendly names are
  // intentionally outside this transport skeleton.
  std::uint32_t baudRate;
  std::uint8_t dataBits;
  SerialParity parity;
  SerialStopBits stopBits;

  // Timeouts used by read and write readiness where supported.
  TransportDuration readTimeout;
  TransportDuration writeTimeout;

  SerialTransportOptions()
    : baudRate(9600)
    , dataBits(8)
    , parity(SerialParity::None)
    , stopBits(SerialStopBits::One)
    , readTimeout{ 5000 }
    , writeTimeout{ 5000 }
  {
  }
};

// Serial byte stream intended for HDLC profile users. It does not implement
// IEC 62056-21 handshakes or HDLC framing.
class SerialTransport : public IByteStream
{
public:
  explicit SerialTransport(const SerialTransportOptions& options);
  ~SerialTransport();

  // Returns Ok, AlreadyOpen, InvalidArgument, or OpenFailed.
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
  SerialTransport(const SerialTransport&);
  SerialTransport& operator=(const SerialTransport&);

  SerialTransportOptions options_;
  intptr_t handle_;
  bool open_;
};

} // namespace transport
} // namespace dlms
