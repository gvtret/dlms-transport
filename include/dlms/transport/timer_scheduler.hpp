#pragma once

#include "dlms/transport/transport_status.hpp"

#include <cstdint>

namespace dlms {
namespace transport {

struct TransportDuration
{
  std::uint32_t milliseconds;
};

// Small timing abstraction used by higher layers for retry and timeout policy.
// It owns no external resources.
class ITimerScheduler
{
public:
  virtual ~ITimerScheduler() {}

  // Writes a monotonic millisecond counter to value.
  // Expected statuses: Ok or InternalError.
  virtual TransportStatus MonotonicMilliseconds(std::uint64_t& value) const = 0;

  // Blocks for at least duration on the current thread.
  // Expected statuses: Ok or InternalError.
  virtual TransportStatus SleepFor(TransportDuration duration) = 0;
};

// System-clock implementation of ITimerScheduler.
class TimerScheduler : public ITimerScheduler
{
public:
  TransportStatus MonotonicMilliseconds(std::uint64_t& value) const;
  TransportStatus SleepFor(TransportDuration duration);
};

} // namespace transport
} // namespace dlms
