#include "dlms/transport/timer_scheduler.hpp"

#include <gtest/gtest.h>

#include <cstdint>

namespace {

using dlms::transport::TimerScheduler;
using dlms::transport::TransportDuration;
using dlms::transport::TransportStatus;

TEST(TimerScheduler, MonotonicMillisecondsReturnsNonDecreasingValues)
{
  TimerScheduler timer;
  std::uint64_t first = 0;
  std::uint64_t second = 0;

  EXPECT_EQ(TransportStatus::Ok, timer.MonotonicMilliseconds(first));
  EXPECT_EQ(TransportStatus::Ok, timer.MonotonicMilliseconds(second));
  EXPECT_LE(first, second);
}

TEST(TimerScheduler, SleepForWaitsAtLeastRequestedDuration)
{
  TimerScheduler timer;
  std::uint64_t before = 0;
  std::uint64_t after = 0;

  ASSERT_EQ(TransportStatus::Ok, timer.MonotonicMilliseconds(before));
  EXPECT_EQ(TransportStatus::Ok, timer.SleepFor(TransportDuration{ 5 }));
  ASSERT_EQ(TransportStatus::Ok, timer.MonotonicMilliseconds(after));

  EXPECT_GE(after - before, 5u);
}

} // namespace
