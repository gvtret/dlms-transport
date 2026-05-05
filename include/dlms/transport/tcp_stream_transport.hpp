#pragma once

#include "dlms/transport/byte_stream.hpp"
#include "dlms/transport/timer_scheduler.hpp"

#include <cstdint>
#include <string>

namespace dlms {
namespace transport {

struct TcpStreamTransportOptions
{
  std::string host;
  std::uint16_t port;
  TransportDuration connectTimeout;
  TransportDuration readTimeout;
  TransportDuration writeTimeout;

  TcpStreamTransportOptions()
    : port(0)
    , connectTimeout{ 5000 }
    , readTimeout{ 5000 }
    , writeTimeout{ 5000 }
  {
  }
};

class TcpStreamTransport : public IByteStream
{
public:
  explicit TcpStreamTransport(const TcpStreamTransportOptions& options);
  ~TcpStreamTransport();

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
  TcpStreamTransport(const TcpStreamTransport&);
  TcpStreamTransport& operator=(const TcpStreamTransport&);

  TcpStreamTransportOptions options_;
  intptr_t socket_;
  bool open_;
};

} // namespace transport
} // namespace dlms
