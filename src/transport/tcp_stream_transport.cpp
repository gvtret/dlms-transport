#include "dlms/transport/tcp_stream_transport.hpp"

#include <cerrno>
#include <cstdio>
#include <cstring>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <fcntl.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace dlms {
namespace transport {
namespace {

#if defined(_WIN32)
typedef SOCKET NativeSocket;
const NativeSocket kInvalidSocket = INVALID_SOCKET;

class SocketRuntime
{
public:
  SocketRuntime()
    : ok_(false)
  {
    WSADATA data;
    ok_ = WSAStartup(MAKEWORD(2, 2), &data) == 0;
  }

  ~SocketRuntime()
  {
    if (ok_) {
      WSACleanup();
    }
  }

  bool Ok() const
  {
    return ok_;
  }

private:
  bool ok_;
};

bool SocketRuntimeReady()
{
  static SocketRuntime runtime;
  return runtime.Ok();
}

int LastSocketError()
{
  return WSAGetLastError();
}

bool IsWouldBlock(int error)
{
  return error == WSAEWOULDBLOCK || error == WSAEINPROGRESS;
}

void CloseNativeSocket(NativeSocket socket)
{
  closesocket(socket);
}

bool SetNonBlocking(NativeSocket socket)
{
  u_long enabled = 1;
  return ioctlsocket(socket, FIONBIO, &enabled) == 0;
}

#else
typedef int NativeSocket;
const NativeSocket kInvalidSocket = -1;

bool SocketRuntimeReady()
{
  return true;
}

int LastSocketError()
{
  return errno;
}

bool IsWouldBlock(int error)
{
  return error == EINPROGRESS || error == EWOULDBLOCK || error == EAGAIN;
}

void CloseNativeSocket(NativeSocket socket)
{
  close(socket);
}

bool SetNonBlocking(NativeSocket socket)
{
  const int flags = fcntl(socket, F_GETFL, 0);
  return flags >= 0 && fcntl(socket, F_SETFL, flags | O_NONBLOCK) == 0;
}
#endif

timeval ToTimeval(TransportDuration timeout)
{
  timeval value;
  value.tv_sec = static_cast<long>(timeout.milliseconds / 1000);
  value.tv_usec = static_cast<long>((timeout.milliseconds % 1000) * 1000);
  return value;
}

TransportStatus WaitForSocket(
  NativeSocket socket,
  bool waitForRead,
  TransportDuration timeout)
{
  fd_set readSet;
  fd_set writeSet;
  FD_ZERO(&readSet);
  FD_ZERO(&writeSet);

  if (waitForRead) {
    FD_SET(socket, &readSet);
  } else {
    FD_SET(socket, &writeSet);
  }

  timeval timeoutValue = ToTimeval(timeout);
  const int result = select(
    static_cast<int>(socket + 1),
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

TransportStatus CheckConnected(NativeSocket socket)
{
  int error = 0;
#if defined(_WIN32)
  int errorSize = sizeof(error);
#else
  socklen_t errorSize = sizeof(error);
#endif
  if (getsockopt(socket, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &errorSize) != 0) {
    return TransportStatus::OpenFailed;
  }
  return error == 0 ? TransportStatus::Ok : TransportStatus::OpenFailed;
}

} // namespace

TcpStreamTransport::TcpStreamTransport(const TcpStreamTransportOptions& options)
  : options_(options)
  , socket_(static_cast<intptr_t>(kInvalidSocket))
  , open_(false)
{
}

TcpStreamTransport::~TcpStreamTransport()
{
  Close();
}

TransportStatus TcpStreamTransport::Open()
{
  if (open_) {
    return TransportStatus::AlreadyOpen;
  }
  if (options_.host.empty() || options_.port == 0) {
    return TransportStatus::InvalidArgument;
  }
  if (!SocketRuntimeReady()) {
    return TransportStatus::OpenFailed;
  }

  addrinfo hints;
  std::memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  char port[16];
  std::snprintf(port, sizeof(port), "%u", static_cast<unsigned>(options_.port));

  addrinfo* addresses = 0;
  if (getaddrinfo(options_.host.c_str(), port, &hints, &addresses) != 0) {
    return TransportStatus::OpenFailed;
  }

  TransportStatus finalStatus = TransportStatus::OpenFailed;
  for (addrinfo* address = addresses; address != 0; address = address->ai_next) {
    NativeSocket candidate = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
    if (candidate == kInvalidSocket) {
      continue;
    }
    if (!SetNonBlocking(candidate)) {
      CloseNativeSocket(candidate);
      continue;
    }

    const int connectResult = connect(candidate, address->ai_addr, static_cast<int>(address->ai_addrlen));
    if (connectResult == 0) {
      socket_ = static_cast<intptr_t>(candidate);
      open_ = true;
      finalStatus = TransportStatus::Ok;
      break;
    }

    const int error = LastSocketError();
    if (!IsWouldBlock(error)) {
      CloseNativeSocket(candidate);
      continue;
    }

    finalStatus = WaitForSocket(candidate, false, options_.connectTimeout);
    if (finalStatus == TransportStatus::Ok) {
      finalStatus = CheckConnected(candidate);
    }
    if (finalStatus == TransportStatus::Ok) {
      socket_ = static_cast<intptr_t>(candidate);
      open_ = true;
      break;
    }

    CloseNativeSocket(candidate);
  }

  freeaddrinfo(addresses);
  return finalStatus;
}

TransportStatus TcpStreamTransport::Close()
{
  if (socket_ != static_cast<intptr_t>(kInvalidSocket)) {
    CloseNativeSocket(static_cast<NativeSocket>(socket_));
  }
  socket_ = static_cast<intptr_t>(kInvalidSocket);
  open_ = false;
  return TransportStatus::Ok;
}

bool TcpStreamTransport::IsOpen() const
{
  return open_;
}

TransportStatus TcpStreamTransport::ReadSome(
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

  const TransportStatus waitStatus =
    WaitForSocket(static_cast<NativeSocket>(socket_), true, options_.readTimeout);
  if (waitStatus != TransportStatus::Ok) {
    return waitStatus;
  }

  const int received = recv(
    static_cast<NativeSocket>(socket_),
    reinterpret_cast<char*>(output),
    static_cast<int>(outputSize),
    0);
  if (received > 0) {
    bytesRead = static_cast<std::size_t>(received);
    return TransportStatus::Ok;
  }
  if (received == 0) {
    Close();
    return TransportStatus::ConnectionClosed;
  }

  return IsWouldBlock(LastSocketError()) ? TransportStatus::WouldBlock : TransportStatus::ReadFailed;
}

TransportStatus TcpStreamTransport::WriteAll(
  const std::uint8_t* input,
  std::size_t inputSize)
{
  if (!open_) {
    return TransportStatus::NotOpen;
  }
  if (input == 0 && inputSize != 0) {
    return TransportStatus::InvalidArgument;
  }

  std::size_t written = 0;
  while (written < inputSize) {
    const TransportStatus waitStatus =
      WaitForSocket(static_cast<NativeSocket>(socket_), false, options_.writeTimeout);
    if (waitStatus != TransportStatus::Ok) {
      return waitStatus == TransportStatus::Timeout ? TransportStatus::Timeout : TransportStatus::WriteFailed;
    }

    const int chunk = send(
      static_cast<NativeSocket>(socket_),
      reinterpret_cast<const char*>(input + written),
      static_cast<int>(inputSize - written),
      0);
    if (chunk > 0) {
      written += static_cast<std::size_t>(chunk);
      continue;
    }
    if (chunk == 0) {
      return TransportStatus::WriteFailed;
    }
    if (IsWouldBlock(LastSocketError())) {
      continue;
    }
    return TransportStatus::WriteFailed;
  }

  return TransportStatus::Ok;
}

} // namespace transport
} // namespace dlms
