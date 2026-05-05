#include "dlms/transport/udp_transport.hpp"

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
#include <sys/ioctl.h>
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
  return error == WSAEWOULDBLOCK;
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

bool PendingDatagramSize(NativeSocket socket, std::size_t& size)
{
  u_long pending = 0;
  if (ioctlsocket(socket, FIONREAD, &pending) != 0) {
    return false;
  }
  size = static_cast<std::size_t>(pending);
  return true;
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
  return error == EWOULDBLOCK || error == EAGAIN;
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

bool PendingDatagramSize(NativeSocket socket, std::size_t& size)
{
  int pending = 0;
  if (ioctl(socket, FIONREAD, &pending) != 0 || pending < 0) {
    return false;
  }
  size = static_cast<std::size_t>(pending);
  return true;
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

void BuildPort(std::uint16_t portValue, char* port, std::size_t portSize)
{
  std::snprintf(port, portSize, "%u", static_cast<unsigned>(portValue));
}

addrinfo* ResolveUdpAddress(
  const std::string& host,
  std::uint16_t portValue,
  int family,
  int flags)
{
  addrinfo hints;
  std::memset(&hints, 0, sizeof(hints));
  hints.ai_family = family;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = IPPROTO_UDP;
  hints.ai_flags = flags;

  char port[16];
  BuildPort(portValue, port, sizeof(port));

  addrinfo* addresses = 0;
  const char* node = host.empty() ? 0 : host.c_str();
  if (getaddrinfo(node, port, &hints, &addresses) != 0) {
    return 0;
  }
  return addresses;
}

bool BoundLocalPort(NativeSocket socket, std::uint16_t& port)
{
  sockaddr_storage address;
  std::memset(&address, 0, sizeof(address));
#if defined(_WIN32)
  int addressSize = sizeof(address);
#else
  socklen_t addressSize = sizeof(address);
#endif

  if (getsockname(socket, reinterpret_cast<sockaddr*>(&address), &addressSize) != 0) {
    return false;
  }

  if (address.ss_family == AF_INET) {
    sockaddr_in* ipv4 = reinterpret_cast<sockaddr_in*>(&address);
    port = ntohs(ipv4->sin_port);
    return true;
  }
  if (address.ss_family == AF_INET6) {
    sockaddr_in6* ipv6 = reinterpret_cast<sockaddr_in6*>(&address);
    port = ntohs(ipv6->sin6_port);
    return true;
  }

  return false;
}

} // namespace

UdpTransport::UdpTransport(const UdpTransportOptions& options)
  : options_(options)
  , socket_(static_cast<intptr_t>(kInvalidSocket))
  , open_(false)
  , remoteConfigured_(false)
{
}

UdpTransport::~UdpTransport()
{
  Close();
}

TransportStatus UdpTransport::Open()
{
  if (open_) {
    return TransportStatus::AlreadyOpen;
  }
  if ((options_.remoteHost.empty() && options_.remotePort != 0) ||
      (!options_.remoteHost.empty() && options_.remotePort == 0)) {
    return TransportStatus::InvalidArgument;
  }
  if (!SocketRuntimeReady()) {
    return TransportStatus::OpenFailed;
  }

  const bool hasRemote = !options_.remoteHost.empty();
  addrinfo* remoteAddresses = hasRemote
    ? ResolveUdpAddress(options_.remoteHost, options_.remotePort, AF_UNSPEC, 0)
    : 0;
  if (hasRemote && remoteAddresses == 0) {
    return TransportStatus::OpenFailed;
  }

  TransportStatus finalStatus = TransportStatus::OpenFailed;
  addrinfo* firstAddress = hasRemote ? remoteAddresses : 0;

  if (firstAddress == 0) {
    firstAddress = ResolveUdpAddress(options_.localHost, options_.localPort, AF_UNSPEC, AI_PASSIVE);
    if (firstAddress == 0) {
      return TransportStatus::OpenFailed;
    }
  }

  for (addrinfo* address = firstAddress; address != 0; address = address->ai_next) {
    NativeSocket candidate = socket(address->ai_family, SOCK_DGRAM, IPPROTO_UDP);
    if (candidate == kInvalidSocket) {
      continue;
    }
    if (!SetNonBlocking(candidate)) {
      CloseNativeSocket(candidate);
      continue;
    }

    addrinfo* localAddresses =
      ResolveUdpAddress(options_.localHost, options_.localPort, address->ai_family, AI_PASSIVE);
    bool bound = false;
    for (addrinfo* local = localAddresses; local != 0; local = local->ai_next) {
      if (bind(candidate, local->ai_addr, static_cast<int>(local->ai_addrlen)) == 0) {
        bound = true;
        break;
      }
    }
    if (localAddresses != 0) {
      freeaddrinfo(localAddresses);
    }
    if (!bound) {
      CloseNativeSocket(candidate);
      continue;
    }

    if (hasRemote && connect(candidate, address->ai_addr, static_cast<int>(address->ai_addrlen)) != 0) {
      CloseNativeSocket(candidate);
      continue;
    }

    socket_ = static_cast<intptr_t>(candidate);
    open_ = true;
    remoteConfigured_ = hasRemote;
    finalStatus = TransportStatus::Ok;
    break;
  }

  if (hasRemote) {
    freeaddrinfo(remoteAddresses);
  } else if (firstAddress != 0) {
    freeaddrinfo(firstAddress);
  }

  return finalStatus;
}

TransportStatus UdpTransport::Close()
{
  if (socket_ != static_cast<intptr_t>(kInvalidSocket)) {
    CloseNativeSocket(static_cast<NativeSocket>(socket_));
  }
  socket_ = static_cast<intptr_t>(kInvalidSocket);
  open_ = false;
  remoteConfigured_ = false;
  return TransportStatus::Ok;
}

bool UdpTransport::IsOpen() const
{
  return open_;
}

TransportStatus UdpTransport::Send(
  const std::uint8_t* input,
  std::size_t inputSize)
{
  if (!open_) {
    return TransportStatus::NotOpen;
  }
  if (!remoteConfigured_) {
    return TransportStatus::InvalidArgument;
  }
  if (input == 0 && inputSize != 0) {
    return TransportStatus::InvalidArgument;
  }

  const TransportStatus waitStatus =
    WaitForSocket(static_cast<NativeSocket>(socket_), false, options_.sendTimeout);
  if (waitStatus != TransportStatus::Ok) {
    return waitStatus == TransportStatus::Timeout ? TransportStatus::Timeout : TransportStatus::WriteFailed;
  }

  const int sent = send(
    static_cast<NativeSocket>(socket_),
    reinterpret_cast<const char*>(input),
    static_cast<int>(inputSize),
    0);
  if (sent < 0) {
    return IsWouldBlock(LastSocketError()) ? TransportStatus::WouldBlock : TransportStatus::WriteFailed;
  }
  if (static_cast<std::size_t>(sent) != inputSize) {
    return TransportStatus::WriteFailed;
  }
  return TransportStatus::Ok;
}

TransportStatus UdpTransport::Receive(
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

  const TransportStatus waitStatus =
    WaitForSocket(static_cast<NativeSocket>(socket_), true, options_.receiveTimeout);
  if (waitStatus != TransportStatus::Ok) {
    return waitStatus;
  }

  std::size_t pending = 0;
  if (!PendingDatagramSize(static_cast<NativeSocket>(socket_), pending)) {
    return TransportStatus::ReadFailed;
  }
  if (pending > outputSize) {
    return TransportStatus::OutputBufferTooSmall;
  }

  const int received = recv(
    static_cast<NativeSocket>(socket_),
    reinterpret_cast<char*>(output),
    static_cast<int>(outputSize),
    0);
  if (received >= 0) {
    bytesRead = static_cast<std::size_t>(received);
    return TransportStatus::Ok;
  }

  return IsWouldBlock(LastSocketError()) ? TransportStatus::WouldBlock : TransportStatus::ReadFailed;
}

std::uint16_t UdpTransport::LocalPort() const
{
  if (!open_) {
    return 0;
  }

  std::uint16_t port = 0;
  return BoundLocalPort(static_cast<NativeSocket>(socket_), port) ? port : 0;
}

} // namespace transport
} // namespace dlms
