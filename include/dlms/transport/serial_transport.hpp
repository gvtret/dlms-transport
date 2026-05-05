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
  std::string deviceName;
  std::uint32_t baudRate;
  std::uint8_t dataBits;
  SerialParity parity;
  SerialStopBits stopBits;
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

class SerialTransport : public IByteStream
{
public:
  explicit SerialTransport(const SerialTransportOptions& options);
  ~SerialTransport();

  TransportStatus Open();
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
