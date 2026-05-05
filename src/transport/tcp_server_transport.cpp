#include "dlms/transport/tcp_server_transport.hpp"

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

std::uint16_t SocketLocalPort(NativeSocket socket)
{
  sockaddr_storage address;
  std::memset(&address, 0, sizeof(address));
#if defined(_WIN32)
  int addressSize = sizeof(address);
#else
  socklen_t addressSize = sizeof(address);
#endif
  if (getsockname(socket, reinterpret_cast<sockaddr*>(&address), &addressSize) != 0) {
    return 0;
  }

  if (address.ss_family == AF_INET) {
    const sockaddr_in* ipv4 = reinterpret_cast<const sockaddr_in*>(&address);
    return ntohs(ipv4->sin_port);
  }
  if (address.ss_family == AF_INET6) {
    const sockaddr_in6* ipv6 = reinterpret_cast<const sockaddr_in6*>(&address);
    return ntohs(ipv6->sin6_port);
  }
  return 0;
}

class AcceptedTcpStream : public IByteStream
{
public:
  AcceptedTcpStream(
    NativeSocket socket,
    TransportDuration readTimeout,
    TransportDuration writeTimeout)
    : socket_(socket)
    , readTimeout_(readTimeout)
    , writeTimeout_(writeTimeout)
    , open_(true)
  {
  }

  ~AcceptedTcpStream()
  {
    Close();
  }

  TransportStatus Open()
  {
    return open_ ? TransportStatus::AlreadyOpen : TransportStatus::OpenFailed;
  }

  TransportStatus Close()
  {
    if (socket_ != kInvalidSocket) {
      CloseNativeSocket(socket_);
    }
    socket_ = kInvalidSocket;
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
    if (outputSize == 0) {
      return TransportStatus::Ok;
    }

    const TransportStatus waitStatus = WaitForSocket(socket_, true, readTimeout_);
    if (waitStatus != TransportStatus::Ok) {
      return waitStatus;
    }

    const int received = recv(
      socket_,
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

  TransportStatus WriteAll(
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
      const TransportStatus waitStatus = WaitForSocket(socket_, false, writeTimeout_);
      if (waitStatus != TransportStatus::Ok) {
        return waitStatus == TransportStatus::Timeout ? TransportStatus::Timeout : TransportStatus::WriteFailed;
      }

      const int chunk = send(
        socket_,
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

private:
  AcceptedTcpStream(const AcceptedTcpStream&);
  AcceptedTcpStream& operator=(const AcceptedTcpStream&);

  NativeSocket socket_;
  TransportDuration readTimeout_;
  TransportDuration writeTimeout_;
  bool open_;
};

} // namespace

TcpServerTransport::TcpServerTransport(const TcpServerTransportOptions& options)
  : options_(options)
  , socket_(static_cast<intptr_t>(kInvalidSocket))
  , open_(false)
  , localPort_(0)
{
}

TcpServerTransport::~TcpServerTransport()
{
  Close();
}

TransportStatus TcpServerTransport::Open()
{
  if (open_) {
    return TransportStatus::AlreadyOpen;
  }
  if (options_.backlog <= 0) {
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
  hints.ai_flags = AI_PASSIVE;

  char port[16];
  std::snprintf(port, sizeof(port), "%u", static_cast<unsigned>(options_.port));

  addrinfo* addresses = 0;
  const char* host = options_.host.empty() ? 0 : options_.host.c_str();
  if (getaddrinfo(host, port, &hints, &addresses) != 0) {
    return TransportStatus::OpenFailed;
  }

  TransportStatus finalStatus = TransportStatus::OpenFailed;
  for (addrinfo* address = addresses; address != 0; address = address->ai_next) {
    NativeSocket candidate = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
    if (candidate == kInvalidSocket) {
      continue;
    }

    int reuse = 1;
    setsockopt(candidate, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));

    if (!SetNonBlocking(candidate)) {
      CloseNativeSocket(candidate);
      continue;
    }
    if (bind(candidate, address->ai_addr, static_cast<int>(address->ai_addrlen)) != 0) {
      CloseNativeSocket(candidate);
      continue;
    }
    if (listen(candidate, options_.backlog) != 0) {
      CloseNativeSocket(candidate);
      continue;
    }

    socket_ = static_cast<intptr_t>(candidate);
    open_ = true;
    localPort_ = SocketLocalPort(candidate);
    finalStatus = TransportStatus::Ok;
    break;
  }

  freeaddrinfo(addresses);
  return finalStatus;
}

TransportStatus TcpServerTransport::Close()
{
  if (socket_ != static_cast<intptr_t>(kInvalidSocket)) {
    CloseNativeSocket(static_cast<NativeSocket>(socket_));
  }
  socket_ = static_cast<intptr_t>(kInvalidSocket);
  open_ = false;
  localPort_ = 0;
  return TransportStatus::Ok;
}

bool TcpServerTransport::IsOpen() const
{
  return open_;
}

std::uint16_t TcpServerTransport::LocalPort() const
{
  return localPort_;
}

TransportStatus TcpServerTransport::Accept(std::unique_ptr<IByteStream>& connection)
{
  connection.reset();

  if (!open_) {
    return TransportStatus::NotOpen;
  }

  NativeSocket listener = static_cast<NativeSocket>(socket_);
  const TransportStatus waitStatus = WaitForSocket(listener, true, options_.acceptTimeout);
  if (waitStatus == TransportStatus::Timeout) {
    return TransportStatus::Timeout;
  }
  if (waitStatus != TransportStatus::Ok || !open_) {
    return TransportStatus::OpenFailed;
  }

  NativeSocket client = accept(listener, 0, 0);
  if (client == kInvalidSocket) {
    return IsWouldBlock(LastSocketError()) ? TransportStatus::WouldBlock : TransportStatus::OpenFailed;
  }
  if (!SetNonBlocking(client)) {
    CloseNativeSocket(client);
    return TransportStatus::OpenFailed;
  }

  connection.reset(new AcceptedTcpStream(client, options_.readTimeout, options_.writeTimeout));
  return TransportStatus::Ok;
}

} // namespace transport
} // namespace dlms
