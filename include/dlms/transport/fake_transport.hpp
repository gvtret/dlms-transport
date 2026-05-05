#pragma once

#include "dlms/transport/byte_stream.hpp"
#include "dlms/transport/datagram_transport.hpp"
#include "dlms/transport/timer_scheduler.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <vector>

namespace dlms {
namespace transport {

class FakeByteStream : public IByteStream
{
public:
  FakeByteStream()
    : open_(false)
    , nextReadStatus_(TransportStatus::Ok)
    , nextWriteStatus_(TransportStatus::Ok)
  {
  }

  TransportStatus Open()
  {
    if (open_) {
      return TransportStatus::AlreadyOpen;
    }

    open_ = true;
    return TransportStatus::Ok;
  }

  TransportStatus Close()
  {
    open_ = false;
    return TransportStatus::Ok;
  }

  bool IsOpen() const
  {
    return open_;
  }

  TransportStatus ReadSome(
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
    if (nextReadStatus_ != TransportStatus::Ok) {
      const TransportStatus status = nextReadStatus_;
      nextReadStatus_ = TransportStatus::Ok;
      return status;
    }
    if (readChunks_.empty()) {
      return TransportStatus::WouldBlock;
    }

    std::vector<std::uint8_t>& chunk = readChunks_.front();
    const std::size_t count = std::min(outputSize, chunk.size());
    std::copy(chunk.begin(), chunk.begin() + static_cast<std::ptrdiff_t>(count), output);
    bytesRead = count;

    if (count == chunk.size()) {
      readChunks_.pop_front();
    } else {
      chunk.erase(chunk.begin(), chunk.begin() + static_cast<std::ptrdiff_t>(count));
    }

    return TransportStatus::Ok;
  }

  TransportStatus WriteAll(const std::uint8_t* input, std::size_t inputSize)
  {
    if (!open_) {
      return TransportStatus::NotOpen;
    }
    if (input == 0 && inputSize != 0) {
      return TransportStatus::InvalidArgument;
    }
    if (nextWriteStatus_ != TransportStatus::Ok) {
      const TransportStatus status = nextWriteStatus_;
      nextWriteStatus_ = TransportStatus::Ok;
      return status;
    }

    if (inputSize == 0) {
      writes_.push_back(std::vector<std::uint8_t>());
    } else {
      writes_.push_back(std::vector<std::uint8_t>(input, input + inputSize));
    }
    return TransportStatus::Ok;
  }

  void ScriptRead(const std::vector<std::uint8_t>& bytes)
  {
    readChunks_.push_back(bytes);
  }

  void ScriptNextReadStatus(TransportStatus status)
  {
    nextReadStatus_ = status;
  }

  void ScriptNextWriteStatus(TransportStatus status)
  {
    nextWriteStatus_ = status;
  }

  const std::vector<std::vector<std::uint8_t> >& Writes() const
  {
    return writes_;
  }

  void Reset()
  {
    open_ = false;
    readChunks_.clear();
    writes_.clear();
    nextReadStatus_ = TransportStatus::Ok;
    nextWriteStatus_ = TransportStatus::Ok;
  }

private:
  bool open_;
  std::deque<std::vector<std::uint8_t> > readChunks_;
  std::vector<std::vector<std::uint8_t> > writes_;
  TransportStatus nextReadStatus_;
  TransportStatus nextWriteStatus_;
};

class FakeDatagramTransport : public IDatagramTransport
{
public:
  FakeDatagramTransport()
    : open_(false)
    , nextReceiveStatus_(TransportStatus::Ok)
    , nextSendStatus_(TransportStatus::Ok)
  {
  }

  TransportStatus Open()
  {
    if (open_) {
      return TransportStatus::AlreadyOpen;
    }

    open_ = true;
    return TransportStatus::Ok;
  }

  TransportStatus Close()
  {
    open_ = false;
    return TransportStatus::Ok;
  }

  bool IsOpen() const
  {
    return open_;
  }

  TransportStatus Send(const std::uint8_t* input, std::size_t inputSize)
  {
    if (!open_) {
      return TransportStatus::NotOpen;
    }
    if (input == 0 && inputSize != 0) {
      return TransportStatus::InvalidArgument;
    }
    if (nextSendStatus_ != TransportStatus::Ok) {
      const TransportStatus status = nextSendStatus_;
      nextSendStatus_ = TransportStatus::Ok;
      return status;
    }

    if (inputSize == 0) {
      sentDatagrams_.push_back(std::vector<std::uint8_t>());
    } else {
      sentDatagrams_.push_back(std::vector<std::uint8_t>(input, input + inputSize));
    }
    return TransportStatus::Ok;
  }

  TransportStatus Receive(
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
    if (nextReceiveStatus_ != TransportStatus::Ok) {
      const TransportStatus status = nextReceiveStatus_;
      nextReceiveStatus_ = TransportStatus::Ok;
      return status;
    }
    if (receiveDatagrams_.empty()) {
      return TransportStatus::WouldBlock;
    }

    const std::vector<std::uint8_t>& datagram = receiveDatagrams_.front();
    if (outputSize < datagram.size()) {
      return TransportStatus::OutputBufferTooSmall;
    }

    std::copy(datagram.begin(), datagram.end(), output);
    bytesRead = datagram.size();
    receiveDatagrams_.pop_front();
    return TransportStatus::Ok;
  }

  void ScriptReceive(const std::vector<std::uint8_t>& bytes)
  {
    receiveDatagrams_.push_back(bytes);
  }

  void ScriptNextReceiveStatus(TransportStatus status)
  {
    nextReceiveStatus_ = status;
  }

  void ScriptNextSendStatus(TransportStatus status)
  {
    nextSendStatus_ = status;
  }

  const std::vector<std::vector<std::uint8_t> >& SentDatagrams() const
  {
    return sentDatagrams_;
  }

  void Reset()
  {
    open_ = false;
    receiveDatagrams_.clear();
    sentDatagrams_.clear();
    nextReceiveStatus_ = TransportStatus::Ok;
    nextSendStatus_ = TransportStatus::Ok;
  }

private:
  bool open_;
  std::deque<std::vector<std::uint8_t> > receiveDatagrams_;
  std::vector<std::vector<std::uint8_t> > sentDatagrams_;
  TransportStatus nextReceiveStatus_;
  TransportStatus nextSendStatus_;
};

class FakeTimerScheduler : public ITimerScheduler
{
public:
  FakeTimerScheduler()
    : now_(0)
  {
  }

  TransportStatus MonotonicMilliseconds(std::uint64_t& value) const
  {
    value = now_;
    return TransportStatus::Ok;
  }

  TransportStatus SleepFor(TransportDuration duration)
  {
    now_ += duration.milliseconds;
    return TransportStatus::Ok;
  }

  void SetNow(std::uint64_t value)
  {
    now_ = value;
  }

private:
  std::uint64_t now_;
};

} // namespace transport
} // namespace dlms
