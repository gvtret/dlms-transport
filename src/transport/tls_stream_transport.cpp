#include "dlms/transport/tls_stream_transport.hpp"

namespace dlms {
namespace transport {

TransportStatus UnsupportedTlsStreamBackend::Handshake(
  IByteStream&,
  const TlsStreamTransportOptions&)
{
  return TransportStatus::UnsupportedFeature;
}

TransportStatus UnsupportedTlsStreamBackend::Close(IByteStream& lower)
{
  return lower.Close();
}

TransportStatus UnsupportedTlsStreamBackend::ReadSome(
  IByteStream&,
  std::uint8_t*,
  std::size_t,
  std::size_t& bytesRead)
{
  bytesRead = 0;
  return TransportStatus::UnsupportedFeature;
}

TransportStatus UnsupportedTlsStreamBackend::WriteAll(
  IByteStream&,
  const std::uint8_t*,
  std::size_t)
{
  return TransportStatus::UnsupportedFeature;
}

TlsStreamTransport::TlsStreamTransport(
  IByteStream* lower,
  ITlsStreamBackend* backend,
  const TlsStreamTransportOptions& options)
  : lower_(lower)
  , backend_(backend)
  , options_(options)
  , open_(false)
{
}

TlsStreamTransport::~TlsStreamTransport()
{
  Close();
}

TransportStatus TlsStreamTransport::Open()
{
  if (open_) {
    return TransportStatus::AlreadyOpen;
  }
  if (!HasValidConfiguration()) {
    return TransportStatus::InvalidArgument;
  }

  const TransportStatus openStatus = lower_->Open();
  if (openStatus != TransportStatus::Ok && openStatus != TransportStatus::AlreadyOpen) {
    return openStatus;
  }

  const TransportStatus handshakeStatus = backend_->Handshake(*lower_, options_);
  if (handshakeStatus != TransportStatus::Ok) {
    lower_->Close();
    return handshakeStatus;
  }

  open_ = true;
  return TransportStatus::Ok;
}

TransportStatus TlsStreamTransport::Close()
{
  if (!open_) {
    if (lower_ != 0) {
      lower_->Close();
    }
    return TransportStatus::Ok;
  }

  TransportStatus status = TransportStatus::Ok;
  if (backend_ != 0 && lower_ != 0) {
    status = backend_->Close(*lower_);
  }
  if (lower_ != 0) {
    const TransportStatus lowerStatus = lower_->Close();
    if (status == TransportStatus::Ok) {
      status = lowerStatus;
    }
  }

  open_ = false;
  return status;
}

bool TlsStreamTransport::IsOpen() const
{
  return open_ && lower_ != 0 && lower_->IsOpen();
}

TransportStatus TlsStreamTransport::ReadSome(
  std::uint8_t* output,
  std::size_t outputSize,
  std::size_t& bytesRead)
{
  bytesRead = 0;
  if (!open_) {
    return TransportStatus::NotOpen;
  }
  if (output == 0 && outputSize != 0) {
    return TransportStatus::InvalidArgument;
  }
  return backend_->ReadSome(*lower_, output, outputSize, bytesRead);
}

TransportStatus TlsStreamTransport::WriteAll(
  const std::uint8_t* input,
  std::size_t inputSize)
{
  if (!open_) {
    return TransportStatus::NotOpen;
  }
  if (input == 0 && inputSize != 0) {
    return TransportStatus::InvalidArgument;
  }
  return backend_->WriteAll(*lower_, input, inputSize);
}

bool TlsStreamTransport::HasValidConfiguration() const
{
  if (lower_ == 0 || backend_ == 0) {
    return false;
  }
  if (options_.verifyPeer && options_.serverName.empty()) {
    return false;
  }
  return true;
}

} // namespace transport
} // namespace dlms
