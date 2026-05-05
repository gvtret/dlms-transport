#include "dlms/transport/tcp_stream_transport.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <thread>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace {

using dlms::transport::TcpStreamTransport;
using dlms::transport::TcpStreamTransportOptions;
using dlms::transport::TransportDuration;
using dlms::transport::TransportStatus;

#if defined(_WIN32)
typedef SOCKET TestSocket;
const TestSocket kInvalidTestSocket = INVALID_SOCKET;

class TestSocketRuntime
{
public:
  TestSocketRuntime()
  {
    WSADATA data;
    WSAStartup(MAKEWORD(2, 2), &data);
  }

  ~TestSocketRuntime()
  {
    WSACleanup();
  }
};

void CloseTestSocket(TestSocket socket)
{
  closesocket(socket);
}

void ShutdownTestSocket(TestSocket socket)
{
  shutdown(socket, SD_BOTH);
}
#else
typedef int TestSocket;
const TestSocket kInvalidTestSocket = -1;

class TestSocketRuntime
{
};

void CloseTestSocket(TestSocket socket)
{
  close(socket);
}

void ShutdownTestSocket(TestSocket socket)
{
  shutdown(socket, SHUT_RDWR);
}
#endif

struct LoopbackServer
{
  TestSocket listener;
  std::uint16_t port;
  std::thread worker;
  std::atomic<bool> ready;
  bool sendGreeting;
  bool closeAfterGreeting;
  std::vector<std::uint8_t> received;

  LoopbackServer()
    : listener(kInvalidTestSocket)
    , port(0)
    , ready(false)
    , sendGreeting(true)
    , closeAfterGreeting(false)
  {
  }

  ~LoopbackServer()
  {
    Stop();
  }

  bool Start(bool sendGreetingValue = true, bool closeAfterGreetingValue = false)
  {
    sendGreeting = sendGreetingValue;
    closeAfterGreeting = closeAfterGreetingValue;
    listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == kInvalidTestSocket) {
      return false;
    }

    sockaddr_in address;
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = 0;

    if (bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
      CloseTestSocket(listener);
      listener = kInvalidTestSocket;
      return false;
    }
    if (listen(listener, 1) != 0) {
      CloseTestSocket(listener);
      listener = kInvalidTestSocket;
      return false;
    }

    sockaddr_in boundAddress;
    std::memset(&boundAddress, 0, sizeof(boundAddress));
#if defined(_WIN32)
    int boundSize = sizeof(boundAddress);
#else
    socklen_t boundSize = sizeof(boundAddress);
#endif
    if (getsockname(listener, reinterpret_cast<sockaddr*>(&boundAddress), &boundSize) != 0) {
      CloseTestSocket(listener);
      listener = kInvalidTestSocket;
      return false;
    }
    port = ntohs(boundAddress.sin_port);
    ready = true;

    worker = std::thread(&LoopbackServer::Run, this);
    return true;
  }

  void Stop()
  {
    if (listener != kInvalidTestSocket) {
      CloseTestSocket(listener);
      listener = kInvalidTestSocket;
    }
    if (worker.joinable()) {
      worker.join();
    }
  }

  void Run()
  {
    TestSocket client = accept(listener, 0, 0);
    if (client == kInvalidTestSocket) {
      return;
    }

    if (sendGreeting) {
      const std::uint8_t greeting[] = { 0xA1, 0xB2, 0xC3 };
      send(client, reinterpret_cast<const char*>(greeting), sizeof(greeting), 0);
    }

    if (closeAfterGreeting) {
      ShutdownTestSocket(client);
      CloseTestSocket(client);
      return;
    }

    char buffer[16] = {};
    const int count = recv(client, buffer, sizeof(buffer), 0);
    if (count > 0) {
      received.assign(
        reinterpret_cast<std::uint8_t*>(buffer),
        reinterpret_cast<std::uint8_t*>(buffer) + count);
    }

    CloseTestSocket(client);
  }
};

TcpStreamTransportOptions LoopbackOptions(std::uint16_t port)
{
  TcpStreamTransportOptions options;
  options.host = "127.0.0.1";
  options.port = port;
  options.connectTimeout = TransportDuration{ 500 };
  options.readTimeout = TransportDuration{ 500 };
  options.writeTimeout = TransportDuration{ 500 };
  return options;
}

TEST(TcpStreamTransport, OpenLoopback)
{
  TestSocketRuntime runtime;
  LoopbackServer server;
  ASSERT_TRUE(server.Start());

  TcpStreamTransport stream(LoopbackOptions(server.port));

  EXPECT_EQ(TransportStatus::Ok, stream.Open());
  EXPECT_TRUE(stream.IsOpen());
  EXPECT_EQ(TransportStatus::Ok, stream.Close());
}

TEST(TcpStreamTransport, OpenInvalidHost)
{
  TcpStreamTransportOptions options;
  options.host = "invalid.invalid.invalid";
  options.port = 4059;
  options.connectTimeout = TransportDuration{ 10 };

  TcpStreamTransport stream(options);

  EXPECT_EQ(TransportStatus::OpenFailed, stream.Open());
}

TEST(TcpStreamTransport, OpenInvalidPort)
{
  TcpStreamTransportOptions options;
  options.host = "127.0.0.1";
  options.port = 0;

  TcpStreamTransport stream(options);

  EXPECT_EQ(TransportStatus::InvalidArgument, stream.Open());
}

TEST(TcpStreamTransport, ConnectTimeout)
{
  TcpStreamTransportOptions options;
  options.host = "10.255.255.1";
  options.port = 65000;
  options.connectTimeout = TransportDuration{ 1 };

  TcpStreamTransport stream(options);
  const TransportStatus status = stream.Open();

  EXPECT_TRUE(status == TransportStatus::Timeout || status == TransportStatus::OpenFailed);
}

TEST(TcpStreamTransport, WriteAllLoopback)
{
  TestSocketRuntime runtime;
  LoopbackServer server;
  ASSERT_TRUE(server.Start(false));

  TcpStreamTransport stream(LoopbackOptions(server.port));
  const std::uint8_t request[] = { 0x01, 0x02, 0x03, 0x04 };

  ASSERT_EQ(TransportStatus::Ok, stream.Open());
  EXPECT_EQ(TransportStatus::Ok, stream.WriteAll(request, sizeof(request)));
  EXPECT_EQ(TransportStatus::Ok, stream.Close());
  server.Stop();

  ASSERT_EQ(4u, server.received.size());
  EXPECT_EQ(0x01, server.received[0]);
  EXPECT_EQ(0x02, server.received[1]);
  EXPECT_EQ(0x03, server.received[2]);
  EXPECT_EQ(0x04, server.received[3]);
}

TEST(TcpStreamTransport, ReadSomeLoopback)
{
  TestSocketRuntime runtime;
  LoopbackServer server;
  ASSERT_TRUE(server.Start());

  TcpStreamTransport stream(LoopbackOptions(server.port));
  std::uint8_t output[8] = {};
  std::size_t bytesRead = 0;

  ASSERT_EQ(TransportStatus::Ok, stream.Open());
  EXPECT_EQ(TransportStatus::Ok, stream.ReadSome(output, sizeof(output), bytesRead));
  EXPECT_EQ(3u, bytesRead);
  EXPECT_EQ(0xA1, output[0]);
  EXPECT_EQ(0xB2, output[1]);
  EXPECT_EQ(0xC3, output[2]);
}

TEST(TcpStreamTransport, PeerCloseReturnsConnectionClosed)
{
  TestSocketRuntime runtime;
  LoopbackServer server;
  ASSERT_TRUE(server.Start(true, true));

  TcpStreamTransport stream(LoopbackOptions(server.port));
  std::uint8_t output[8] = {};
  std::size_t bytesRead = 0;

  ASSERT_EQ(TransportStatus::Ok, stream.Open());
  ASSERT_EQ(TransportStatus::Ok, stream.ReadSome(output, sizeof(output), bytesRead));
  EXPECT_EQ(TransportStatus::ConnectionClosed, stream.ReadSome(output, sizeof(output), bytesRead));
  EXPECT_FALSE(stream.IsOpen());
}

TEST(TcpStreamTransport, ReadTimeout)
{
  TestSocketRuntime runtime;
  LoopbackServer server;
  ASSERT_TRUE(server.Start());

  TcpStreamTransportOptions options = LoopbackOptions(server.port);
  options.readTimeout = TransportDuration{ 1 };
  TcpStreamTransport stream(options);
  std::uint8_t output[8] = {};
  std::size_t bytesRead = 0;

  ASSERT_EQ(TransportStatus::Ok, stream.Open());
  ASSERT_EQ(TransportStatus::Ok, stream.ReadSome(output, sizeof(output), bytesRead));
  EXPECT_EQ(TransportStatus::Timeout, stream.ReadSome(output, sizeof(output), bytesRead));
}

TEST(TcpStreamTransport, CloseIsIdempotent)
{
  TcpStreamTransport stream(LoopbackOptions(4059));

  EXPECT_EQ(TransportStatus::Ok, stream.Close());
  EXPECT_EQ(TransportStatus::Ok, stream.Close());
}

} // namespace
