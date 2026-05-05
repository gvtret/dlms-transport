#include "dlms/transport/transport_event_loop.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace {

using dlms::transport::ManualTransportEventLoop;
using dlms::transport::TransportDuration;
using dlms::transport::TransportEvent;
using dlms::transport::TransportEventReadable;
using dlms::transport::TransportEventTimer;
using dlms::transport::TransportEventWritable;
using dlms::transport::TransportStatus;

TEST(TransportEventLoop, RegistersReadableWritableTimers)
{
  ManualTransportEventLoop loop;
  std::vector<TransportEvent> events;
  int ioToken = 0;
  int timerToken = 0;

  EXPECT_EQ(
    TransportStatus::Ok,
    loop.Register(
      123,
      TransportEventReadable | TransportEventWritable,
      [&events](const TransportEvent& event) { events.push_back(event); },
      ioToken));
  EXPECT_NE(0, ioToken);

  EXPECT_EQ(
    TransportStatus::Ok,
    loop.RegisterTimer(
      TransportDuration{ 10 },
      [&events](const TransportEvent& event) { events.push_back(event); },
      timerToken));
  EXPECT_NE(0, timerToken);
}

TEST(TransportEventLoop, DispatchesTransportCallbacks)
{
  ManualTransportEventLoop loop;
  std::vector<TransportEvent> events;
  int ioToken = 0;
  int timerToken = 0;

  ASSERT_EQ(
    TransportStatus::Ok,
    loop.Register(
      456,
      TransportEventReadable | TransportEventWritable,
      [&events](const TransportEvent& event) { events.push_back(event); },
      ioToken));
  ASSERT_EQ(
    TransportStatus::Ok,
    loop.RegisterTimer(
      TransportDuration{ 1 },
      [&events](const TransportEvent& event) { events.push_back(event); },
      timerToken));
  ASSERT_EQ(TransportStatus::Ok, loop.SetReady(ioToken, TransportEventReadable));

  EXPECT_EQ(TransportStatus::Ok, loop.DispatchOnce());

  ASSERT_EQ(2u, events.size());
  EXPECT_EQ(ioToken, events[0].token);
  EXPECT_EQ(456, events[0].handle);
  EXPECT_EQ(TransportEventReadable, events[0].events);
  EXPECT_EQ(timerToken, events[1].token);
  EXPECT_EQ(TransportEventTimer, events[1].events);
}

TEST(TransportEventLoop, UnregisterIsIdempotent)
{
  ManualTransportEventLoop loop;
  std::vector<TransportEvent> events;
  int token = 0;

  ASSERT_EQ(
    TransportStatus::Ok,
    loop.Register(
      789,
      TransportEventReadable,
      [&events](const TransportEvent& event) { events.push_back(event); },
      token));
  ASSERT_EQ(TransportStatus::Ok, loop.SetReady(token, TransportEventReadable));

  EXPECT_EQ(TransportStatus::Ok, loop.Unregister(token));
  EXPECT_EQ(TransportStatus::Ok, loop.Unregister(token));
  EXPECT_EQ(TransportStatus::WouldBlock, loop.DispatchOnce());
  EXPECT_TRUE(events.empty());
}

} // namespace
