#pragma once

#include "dlms/transport/transport_status.hpp"

#include <cstdint>

namespace dlms {
namespace transport {

struct TransportDuration
{
  std::uint32_t milliseconds;
};

class ITimerScheduler
{
public:
  virtual ~ITimerScheduler() {}

  virtual TransportStatus MonotonicMilliseconds(std::uint64_t& value) const = 0;
  virtual TransportStatus SleepFor(TransportDuration duration) = 0;
};

class TimerScheduler : public ITimerScheduler
{
public:
  TransportStatus MonotonicMilliseconds(std::uint64_t& value) const;
  TransportStatus SleepFor(TransportDuration duration);
};

} // namespace transport
} // namespace dlms
