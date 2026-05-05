#include "dlms/transport/fake_transport.hpp"
#include "dlms/transport/non_blocking_byte_stream.hpp"
#include "dlms/transport/non_blocking_datagram_transport.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <vector>

namespace {

using dlms::transport::FakeByteStream;
using dlms::transport::FakeDatagramTransport;
using dlms::transport::INonBlockingByteStream;
using dlms::transport::INonBlockingDatagramTransport;
using dlms::transport::TransportStatus;

class FakeNonBlockingByteStream : public INonBlockingByteStream
{
public:
  FakeNonBlockingByteStream()
    : open_(false)
    , writeCapacity_(0)
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

  TransportStatus TryReadSome(
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
    if (readChunks_.empty()) {
      return TransportStatus::WouldBlock;
    }

    std::vector<std::uint8_t>& chunk = readChunks_.front();
    const std::size_t count = std::min(outputSize, chunk.size());
    std::copy(chunk.begin(), chunk.begin() + static_cast<std::ptrdiff_t>(count), output);
    bytesRead = count;
    chunk.erase(chunk.begin(), chunk.begin() + static_cast<std::ptrdiff_t>(count));
    if (chunk.empty()) {
      readChunks_.pop_front();
    }
    return TransportStatus::Ok;
  }

  TransportStatus TryWriteSome(
    const std::uint8_t* input,
    std::size_t inputSize,
    std::size_t& bytesWritten)
  {
    bytesWritten = 0;
    if (!open_) {
      return TransportStatus::NotOpen;
    }
    if (input == 0 && inputSize != 0) {
      return TransportStatus::InvalidArgument;
    }
    if (writeCapacity_ == 0 && inputSize != 0) {
      return TransportStatus::WouldBlock;
    }

    const std::size_t count = std::min(writeCapacity_, inputSize);
    writes_.push_back(std::vector<std::uint8_t>(input, input + count));
    bytesWritten = count;
    writeCapacity_ -= count;
    return TransportStatus::Ok;
  }

  void ScriptRead(const std::vector<std::uint8_t>& bytes)
  {
    readChunks_.push_back(bytes);
  }

  void SetWriteCapacity(std::size_t capacity)
  {
    writeCapacity_ = capacity;
  }

  const std::vector<std::vector<std::uint8_t> >& Writes() const
  {
    return writes_;
  }

private:
  bool open_;
  std::deque<std::vector<std::uint8_t> > readChunks_;
  std::size_t writeCapacity_;
  std::vector<std::vector<std::uint8_t> > writes_;
};

class FakeNonBlockingDatagramTransport : public INonBlockingDatagramTransport
{
public:
  FakeNonBlockingDatagramTransport()
    : open_(false)
    , sendWouldBlock_(false)
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

  TransportStatus TrySend(const std::uint8_t* input, std::size_t inputSize)
  {
    if (!open_) {
      return TransportStatus::NotOpen;
    }
    if (input == 0 && inputSize != 0) {
      return TransportStatus::InvalidArgument;
    }
    if (sendWouldBlock_) {
      return TransportStatus::WouldBlock;
    }

    sentDatagrams_.push_back(std::vector<std::uint8_t>(input, input + inputSize));
    return TransportStatus::Ok;
  }

  TransportStatus TryReceive(
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

  void SetSendWouldBlock(bool value)
  {
    sendWouldBlock_ = value;
  }

  std::size_t PendingDatagramCount() const
  {
    return receiveDatagrams_.size();
  }

private:
  bool open_;
  bool sendWouldBlock_;
  std::deque<std::vector<std::uint8_t> > receiveDatagrams_;
  std::vector<std::vector<std::uint8_t> > sentDatagrams_;
};

TEST(NonBlockingTransport, ByteStreamReturnsWouldBlockWithoutClosing)
{
  FakeNonBlockingByteStream stream;
  ASSERT_EQ(TransportStatus::Ok, stream.Open());

  std::uint8_t output[4] = {};
  std::size_t bytesRead = 99;
  EXPECT_EQ(TransportStatus::WouldBlock, stream.TryReadSome(output, sizeof(output), bytesRead));
  EXPECT_EQ(0u, bytesRead);
  EXPECT_TRUE(stream.IsOpen());

  std::size_t bytesWritten = 99;
  const std::uint8_t input[] = { 1, 2, 3 };
  EXPECT_EQ(TransportStatus::WouldBlock, stream.TryWriteSome(input, sizeof(input), bytesWritten));
  EXPECT_EQ(0u, bytesWritten);
  EXPECT_TRUE(stream.IsOpen());
}

TEST(NonBlockingTransport, DatagramReturnsWouldBlockWithoutDroppingDatagram)
{
  FakeNonBlockingDatagramTransport datagram;
  ASSERT_EQ(TransportStatus::Ok, datagram.Open());

  std::uint8_t output[4] = {};
  std::size_t bytesRead = 99;
  EXPECT_EQ(TransportStatus::WouldBlock, datagram.TryReceive(output, sizeof(output), bytesRead));
  EXPECT_EQ(0u, bytesRead);
  EXPECT_TRUE(datagram.IsOpen());

  datagram.ScriptReceive(std::vector<std::uint8_t>{ 0xA1, 0xB2, 0xC3 });
  std::uint8_t smallOutput[2] = {};
  EXPECT_EQ(
    TransportStatus::OutputBufferTooSmall,
    datagram.TryReceive(smallOutput, sizeof(smallOutput), bytesRead));
  EXPECT_EQ(1u, datagram.PendingDatagramCount());
}

TEST(NonBlockingTransport, InterfacesDoNotChangeBlockingContracts)
{
  FakeByteStream byteStream;
  ASSERT_EQ(TransportStatus::Ok, byteStream.Open());
  std::uint8_t output[4] = {};
  std::size_t bytesRead = 99;

  EXPECT_EQ(TransportStatus::WouldBlock, byteStream.ReadSome(output, sizeof(output), bytesRead));
  EXPECT_EQ(0u, bytesRead);

  FakeDatagramTransport datagram;
  ASSERT_EQ(TransportStatus::Ok, datagram.Open());
  EXPECT_EQ(TransportStatus::WouldBlock, datagram.Receive(output, sizeof(output), bytesRead));
  EXPECT_EQ(0u, bytesRead);
}

} // namespace
