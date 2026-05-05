#pragma once

#include "dlms/transport/timer_scheduler.hpp"
#include "dlms/transport/transport_status.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <vector>

namespace dlms {
namespace transport {

enum TransportEventMask
{
  TransportEventNone = 0,
  TransportEventReadable = 1,
  TransportEventWritable = 2,
  TransportEventTimer = 4
};

struct TransportEvent
{
  int token;
  intptr_t handle;
  int events;

  TransportEvent()
    : token(0)
    , handle(0)
    , events(TransportEventNone)
  {
  }
};

typedef std::function<void(const TransportEvent&)> TransportEventCallback;

class ITransportEventLoop
{
public:
  virtual ~ITransportEventLoop() {}

  virtual TransportStatus Register(
    intptr_t handle,
    int events,
    const TransportEventCallback& callback,
    int& token) = 0;

  virtual TransportStatus RegisterTimer(
    TransportDuration delay,
    const TransportEventCallback& callback,
    int& token) = 0;

  virtual TransportStatus Unregister(int token) = 0;
  virtual TransportStatus DispatchOnce() = 0;
};

class ManualTransportEventLoop : public ITransportEventLoop
{
public:
  ManualTransportEventLoop()
    : nextToken_(1)
  {
  }

  TransportStatus Register(
    intptr_t handle,
    int events,
    const TransportEventCallback& callback,
    int& token)
  {
    token = 0;
    if (events == TransportEventNone || !callback) {
      return TransportStatus::InvalidArgument;
    }

    Registration registration;
    registration.handle = handle;
    registration.events = events;
    registration.callback = callback;
    registration.readyEvents = TransportEventNone;
    registration.timerDelay = TransportDuration{ 0 };
    registration.timer = false;

    token = nextToken_++;
    registrations_[token] = registration;
    return TransportStatus::Ok;
  }

  TransportStatus RegisterTimer(
    TransportDuration delay,
    const TransportEventCallback& callback,
    int& token)
  {
    token = 0;
    if (!callback) {
      return TransportStatus::InvalidArgument;
    }

    Registration registration;
    registration.handle = 0;
    registration.events = TransportEventTimer;
    registration.callback = callback;
    registration.readyEvents = TransportEventTimer;
    registration.timerDelay = delay;
    registration.timer = true;

    token = nextToken_++;
    registrations_[token] = registration;
    return TransportStatus::Ok;
  }

  TransportStatus Unregister(int token)
  {
    registrations_.erase(token);
    return TransportStatus::Ok;
  }

  TransportStatus DispatchOnce()
  {
    std::vector<TransportEvent> events;
    for (std::map<int, Registration>::iterator it = registrations_.begin();
         it != registrations_.end();
         ++it) {
      const int ready = it->second.readyEvents & it->second.events;
      if (ready == TransportEventNone) {
        continue;
      }

      TransportEvent event;
      event.token = it->first;
      event.handle = it->second.handle;
      event.events = ready;
      events.push_back(event);

      if (it->second.timer) {
        it->second.readyEvents = TransportEventNone;
      }
    }

    for (std::vector<TransportEvent>::const_iterator it = events.begin();
         it != events.end();
         ++it) {
      std::map<int, Registration>::iterator registration = registrations_.find(it->token);
      if (registration != registrations_.end()) {
        registration->second.callback(*it);
      }
    }

    return events.empty() ? TransportStatus::WouldBlock : TransportStatus::Ok;
  }

  TransportStatus SetReady(int token, int events)
  {
    std::map<int, Registration>::iterator registration = registrations_.find(token);
    if (registration == registrations_.end()) {
      return TransportStatus::NotOpen;
    }

    registration->second.readyEvents = events;
    return TransportStatus::Ok;
  }

private:
  struct Registration
  {
    intptr_t handle;
    int events;
    TransportEventCallback callback;
    int readyEvents;
    TransportDuration timerDelay;
    bool timer;
  };

  int nextToken_;
  std::map<int, Registration> registrations_;
};

} // namespace transport
} // namespace dlms
