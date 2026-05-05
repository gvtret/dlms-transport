#include "dlms/transport/serial_transport.hpp"

#include <cerrno>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/select.h>
#include <unistd.h>
#endif

namespace dlms {
namespace transport {
namespace {

#if defined(_WIN32)
typedef HANDLE NativeSerialHandle;
const NativeSerialHandle kInvalidSerialHandle = INVALID_HANDLE_VALUE;

NativeSerialHandle OpenNativeSerial(const SerialTransportOptions& options)
{
  return CreateFileA(
    options.deviceName.c_str(),
    GENERIC_READ | GENERIC_WRITE,
    0,
    0,
    OPEN_EXISTING,
    0,
    0);
}

void CloseNativeSerial(NativeSerialHandle handle)
{
  CloseHandle(handle);
}

TransportStatus NativeRead(
  NativeSerialHandle handle,
  std::uint8_t* output,
  std::size_t outputSize,
  std::size_t& bytesRead)
{
  DWORD received = 0;
  if (!ReadFile(handle, output, static_cast<DWORD>(outputSize), &received, 0)) {
    return TransportStatus::ReadFailed;
  }
  bytesRead = static_cast<std::size_t>(received);
  return TransportStatus::Ok;
}

TransportStatus NativeWriteAll(
  NativeSerialHandle handle,
  const std::uint8_t* input,
  std::size_t inputSize)
{
  std::size_t written = 0;
  while (written < inputSize) {
    DWORD chunk = 0;
    if (!WriteFile(
          handle,
          input + written,
          static_cast<DWORD>(inputSize - written),
          &chunk,
          0)) {
      return TransportStatus::WriteFailed;
    }
    if (chunk == 0) {
      return TransportStatus::WriteFailed;
    }
    written += static_cast<std::size_t>(chunk);
  }
  return TransportStatus::Ok;
}

#else
typedef int NativeSerialHandle;
const NativeSerialHandle kInvalidSerialHandle = -1;

timeval ToTimeval(TransportDuration timeout)
{
  timeval value;
  value.tv_sec = static_cast<long>(timeout.milliseconds / 1000);
  value.tv_usec = static_cast<long>((timeout.milliseconds % 1000) * 1000);
  return value;
}

TransportStatus WaitForHandle(
  NativeSerialHandle handle,
  bool waitForRead,
  TransportDuration timeout)
{
  fd_set readSet;
  fd_set writeSet;
  FD_ZERO(&readSet);
  FD_ZERO(&writeSet);

  if (waitForRead) {
    FD_SET(handle, &readSet);
  } else {
    FD_SET(handle, &writeSet);
  }

  timeval timeoutValue = ToTimeval(timeout);
  const int result = select(
    handle + 1,
    waitForRead ? &readSet : 0,
    waitForRead ? 0 : &writeSet,
    0,
    &timeoutValue);
  if (result > 0) {
    return TransportStatus::Ok;
  }
  if (result == 0) {
    return TransportStatus::Timeout;
  }
  return waitForRead ? TransportStatus::ReadFailed : TransportStatus::WriteFailed;
}

NativeSerialHandle OpenNativeSerial(const SerialTransportOptions& options)
{
  return open(options.deviceName.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
}

void CloseNativeSerial(NativeSerialHandle handle)
{
  close(handle);
}

TransportStatus NativeRead(
  NativeSerialHandle handle,
  std::uint8_t* output,
  std::size_t outputSize,
  std::size_t& bytesRead,
  TransportDuration timeout)
{
  const TransportStatus waitStatus = WaitForHandle(handle, true, timeout);
  if (waitStatus != TransportStatus::Ok) {
    return waitStatus;
  }

  const ssize_t received = read(handle, output, outputSize);
  if (received >= 0) {
    bytesRead = static_cast<std::size_t>(received);
    return TransportStatus::Ok;
  }
  return (errno == EAGAIN || errno == EWOULDBLOCK) ? TransportStatus::WouldBlock : TransportStatus::ReadFailed;
}

TransportStatus NativeWriteAll(
  NativeSerialHandle handle,
  const std::uint8_t* input,
  std::size_t inputSize,
  TransportDuration timeout)
{
  std::size_t written = 0;
  while (written < inputSize) {
    const TransportStatus waitStatus = WaitForHandle(handle, false, timeout);
    if (waitStatus != TransportStatus::Ok) {
      return waitStatus == TransportStatus::Timeout ? TransportStatus::Timeout : TransportStatus::WriteFailed;
    }

    const ssize_t chunk = write(handle, input + written, inputSize - written);
    if (chunk > 0) {
      written += static_cast<std::size_t>(chunk);
      continue;
    }
    if (chunk == 0) {
      return TransportStatus::WriteFailed;
    }
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      continue;
    }
    return TransportStatus::WriteFailed;
  }
  return TransportStatus::Ok;
}
#endif

bool OptionsAreValid(const SerialTransportOptions& options)
{
  return !options.deviceName.empty() &&
    options.baudRate != 0 &&
    options.dataBits >= 5 &&
    options.dataBits <= 8;
}

intptr_t StoreNativeHandle(NativeSerialHandle handle)
{
#if defined(_WIN32)
  return reinterpret_cast<intptr_t>(handle);
#else
  return static_cast<intptr_t>(handle);
#endif
}

NativeSerialHandle LoadNativeHandle(intptr_t handle)
{
#if defined(_WIN32)
  return reinterpret_cast<NativeSerialHandle>(handle);
#else
  return static_cast<NativeSerialHandle>(handle);
#endif
}

} // namespace

SerialTransport::SerialTransport(const SerialTransportOptions& options)
  : options_(options)
  , handle_(StoreNativeHandle(kInvalidSerialHandle))
  , open_(false)
{
}

SerialTransport::~SerialTransport()
{
  Close();
}

TransportStatus SerialTransport::Open()
{
  if (open_) {
    return TransportStatus::AlreadyOpen;
  }
  if (!OptionsAreValid(options_)) {
    return TransportStatus::InvalidArgument;
  }

  NativeSerialHandle handle = OpenNativeSerial(options_);
  if (handle == kInvalidSerialHandle) {
    return TransportStatus::OpenFailed;
  }

  handle_ = StoreNativeHandle(handle);
  open_ = true;
  return TransportStatus::Ok;
}

TransportStatus SerialTransport::Close()
{
  if (handle_ != StoreNativeHandle(kInvalidSerialHandle)) {
    CloseNativeSerial(LoadNativeHandle(handle_));
  }
  handle_ = StoreNativeHandle(kInvalidSerialHandle);
  open_ = false;
  return TransportStatus::Ok;
}

bool SerialTransport::IsOpen() const
{
  return open_;
}

TransportStatus SerialTransport::ReadSome(
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
  if (outputSize == 0) {
    return TransportStatus::Ok;
  }

#if defined(_WIN32)
  return NativeRead(LoadNativeHandle(handle_), output, outputSize, bytesRead);
#else
  return NativeRead(
    LoadNativeHandle(handle_),
    output,
    outputSize,
    bytesRead,
    options_.readTimeout);
#endif
}

TransportStatus SerialTransport::WriteAll(
  const std::uint8_t* input,
  std::size_t inputSize)
{
  if (!open_) {
    return TransportStatus::NotOpen;
  }
  if (input == 0 && inputSize != 0) {
    return TransportStatus::InvalidArgument;
  }
  if (inputSize == 0) {
    return TransportStatus::Ok;
  }

#if defined(_WIN32)
  return NativeWriteAll(LoadNativeHandle(handle_), input, inputSize);
#else
  return NativeWriteAll(
    LoadNativeHandle(handle_),
    input,
    inputSize,
    options_.writeTimeout);
#endif
}

} // namespace transport
} // namespace dlms
