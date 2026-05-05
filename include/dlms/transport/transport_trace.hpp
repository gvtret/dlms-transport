#pragma once

#include "dlms/transport/transport_status.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace dlms {
namespace transport {

enum class TransportTraceEventKind
{
  Open,
  Close,
  Accept,
  Read,
  Write,
  Send,
  Receive
};

enum class TransportTraceDirection
{
  None,
  Inbound,
  Outbound
};

struct TransportTraceEvent
{
  TransportTraceEventKind kind;
  TransportTraceDirection direction;
  TransportStatus status;
  std::string endpoint;
  std::size_t byteCount;
  std::uint64_t timestampMilliseconds;

  TransportTraceEvent()
    : kind(TransportTraceEventKind::Open)
    , direction(TransportTraceDirection::None)
    , status(TransportStatus::Ok)
    , byteCount(0)
    , timestampMilliseconds(0)
  {
  }
};

class ITransportTraceSink
{
public:
  virtual ~ITransportTraceSink() {}

  virtual void OnTransportTrace(const TransportTraceEvent& event) = 0;
};

class DisabledTransportTraceSink : public ITransportTraceSink
{
public:
  void OnTransportTrace(const TransportTraceEvent&)
  {
  }
};

inline void EmitTransportTrace(
  ITransportTraceSink* sink,
  const TransportTraceEvent& event)
{
  if (sink != 0) {
    sink->OnTransportTrace(event);
  }
}

} // namespace transport
} // namespace dlms
