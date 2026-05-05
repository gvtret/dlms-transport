#include "dlms/transport/transport_trace.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <vector>

namespace {

using dlms::transport::DisabledTransportTraceSink;
using dlms::transport::EmitTransportTrace;
using dlms::transport::ITransportTraceSink;
using dlms::transport::TransportStatus;
using dlms::transport::TransportTraceDirection;
using dlms::transport::TransportTraceEvent;
using dlms::transport::TransportTraceEventKind;

class RecordingTraceSink : public ITransportTraceSink
{
public:
  void OnTransportTrace(const TransportTraceEvent& event)
  {
    events.push_back(event);
  }

  std::vector<TransportTraceEvent> events;
};

TEST(TransportTrace, SinkReceivesLifecycleEvents)
{
  RecordingTraceSink sink;
  TransportTraceEvent event;
  event.kind = TransportTraceEventKind::Open;
  event.direction = TransportTraceDirection::None;
  event.status = TransportStatus::Ok;
  event.endpoint = "127.0.0.1:4059";
  event.byteCount = 0;
  event.timestampMilliseconds = 42;

  EmitTransportTrace(&sink, event);

  ASSERT_EQ(1u, sink.events.size());
  EXPECT_EQ(TransportTraceEventKind::Open, sink.events[0].kind);
  EXPECT_EQ(TransportTraceDirection::None, sink.events[0].direction);
  EXPECT_EQ(TransportStatus::Ok, sink.events[0].status);
  EXPECT_EQ("127.0.0.1:4059", sink.events[0].endpoint);
  EXPECT_EQ(0u, sink.events[0].byteCount);
  EXPECT_EQ(42u, sink.events[0].timestampMilliseconds);
}

TEST(TransportTrace, SinkCanBeDisabled)
{
  DisabledTransportTraceSink disabled;
  TransportTraceEvent event;
  event.kind = TransportTraceEventKind::Close;
  event.status = TransportStatus::Ok;

  EmitTransportTrace(&disabled, event);
  EmitTransportTrace(0, event);
}

TEST(TransportTrace, EventDoesNotCopyPayloadByDefault)
{
  TransportTraceEvent event;
  event.kind = TransportTraceEventKind::Write;
  event.direction = TransportTraceDirection::Outbound;
  event.status = TransportStatus::Ok;
  event.byteCount = 128;

  EXPECT_EQ(TransportTraceEventKind::Write, event.kind);
  EXPECT_EQ(TransportTraceDirection::Outbound, event.direction);
  EXPECT_EQ(TransportStatus::Ok, event.status);
  EXPECT_EQ(128u, event.byteCount);
}

} // namespace
