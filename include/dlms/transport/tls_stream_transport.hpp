#pragma once

#include "dlms/transport/byte_stream.hpp"

#include <string>

namespace dlms {
namespace transport {

struct TlsStreamTransportOptions
{
  // Server name used by the TLS backend for SNI or peer-name checks.
  // Certificate and cipher policy are owned by the backend/user.
  std::string serverName;

  // Enables peer-name validation in the backend when supported.
  bool verifyPeer;

  TlsStreamTransportOptions()
    : verifyPeer(true)
  {
  }
};

class ITlsStreamBackend
{
public:
  virtual ~ITlsStreamBackend() {}

  virtual TransportStatus Handshake(
    IByteStream& lower,
    const TlsStreamTransportOptions& options) = 0;

  virtual TransportStatus Close(IByteStream& lower) = 0;

  virtual TransportStatus ReadSome(
    IByteStream& lower,
    std::uint8_t* output,
    std::size_t outputSize,
    std::size_t& bytesRead) = 0;

  virtual TransportStatus WriteAll(
    IByteStream& lower,
    const std::uint8_t* input,
    std::size_t inputSize) = 0;
};

class UnsupportedTlsStreamBackend : public ITlsStreamBackend
{
public:
  TransportStatus Handshake(
    IByteStream&,
    const TlsStreamTransportOptions&);

  TransportStatus Close(IByteStream& lower);

  TransportStatus ReadSome(
    IByteStream&,
    std::uint8_t* output,
    std::size_t,
    std::size_t& bytesRead);

  TransportStatus WriteAll(
    IByteStream&,
    const std::uint8_t*,
    std::size_t);
};

class TlsStreamTransport : public IByteStream
{
public:
  TlsStreamTransport(
    IByteStream* lower,
    ITlsStreamBackend* backend,
    const TlsStreamTransportOptions& options);
  ~TlsStreamTransport();

  // Opens the lower stream and performs the TLS backend handshake.
  TransportStatus Open();

  // Closes the TLS backend and lower stream. Close is idempotent.
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
  TlsStreamTransport(const TlsStreamTransport&);
  TlsStreamTransport& operator=(const TlsStreamTransport&);

  bool HasValidConfiguration() const;

  IByteStream* lower_;
  ITlsStreamBackend* backend_;
  TlsStreamTransportOptions options_;
  bool open_;
};

} // namespace transport
} // namespace dlms
