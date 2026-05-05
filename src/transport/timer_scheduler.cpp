#include "dlms/transport/timer_scheduler.hpp"

#include <chrono>
#include <thread>

namespace dlms {
namespace transport {

TransportStatus TimerScheduler::MonotonicMilliseconds(std::uint64_t& value) const
{
  const std::chrono::steady_clock::duration elapsed =
    std::chrono::steady_clock::now().time_since_epoch();
  value = static_cast<std::uint64_t>(
    std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
  return TransportStatus::Ok;
}

TransportStatus TimerScheduler::SleepFor(TransportDuration duration)
{
  std::this_thread::sleep_for(std::chrono::milliseconds(duration.milliseconds));
  return TransportStatus::Ok;
}

} // namespace transport
} // namespace dlms
