#pragma once

namespace dlms {
namespace transport {

enum class TransportStatus
{
  Ok,
  InvalidArgument,
  NotOpen,
  AlreadyOpen,
  OpenFailed,
  ReadFailed,
  WriteFailed,
  Timeout,
  ConnectionClosed,
  WouldBlock,
  OutputBufferTooSmall,
  UnsupportedFeature,
  InternalError
};

inline const char* ToString(TransportStatus status)
{
  switch (status) {
  case TransportStatus::Ok:
    return "Ok";
  case TransportStatus::InvalidArgument:
    return "InvalidArgument";
  case TransportStatus::NotOpen:
    return "NotOpen";
  case TransportStatus::AlreadyOpen:
    return "AlreadyOpen";
  case TransportStatus::OpenFailed:
    return "OpenFailed";
  case TransportStatus::ReadFailed:
    return "ReadFailed";
  case TransportStatus::WriteFailed:
    return "WriteFailed";
  case TransportStatus::Timeout:
    return "Timeout";
  case TransportStatus::ConnectionClosed:
    return "ConnectionClosed";
  case TransportStatus::WouldBlock:
    return "WouldBlock";
  case TransportStatus::OutputBufferTooSmall:
    return "OutputBufferTooSmall";
  case TransportStatus::UnsupportedFeature:
    return "UnsupportedFeature";
  case TransportStatus::InternalError:
    return "InternalError";
  }

  return "Unknown";
}

} // namespace transport
} // namespace dlms
