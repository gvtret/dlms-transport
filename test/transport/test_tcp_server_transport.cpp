#include "dlms/transport/tcp_server_transport.hpp"
#include "dlms/transport/tcp_stream_transport.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <future>
#include <memory>
#include <thread>

namespace {

using dlms::transport::IByteStream;
using dlms::transport::TcpServerTransport;
using dlms::transport::TcpServerTransportOptions;
using dlms::transport::TcpStreamTransport;
using dlms::transport::TcpStreamTransportOptions;
using dlms::transport::TransportDuration;
using dlms::transport::TransportStatus;

TcpServerTransportOptions ServerOptions()
{
  TcpServerTransportOptions options;
  options.host = "127.0.0.1";
  options.port = 0;
  options.backlog = 1;
  options.acceptTimeout = TransportDuration{ 500 };
  options.readTimeout = TransportDuration{ 500 };
  options.writeTimeout = TransportDuration{ 500 };
  return options;
}

TcpStreamTransportOptions ClientOptions(std::uint16_t port)
{
  TcpStreamTransportOptions options;
  options.host = "127.0.0.1";
  options.port = port;
  options.connectTimeout = TransportDuration{ 500 };
  options.readTimeout = TransportDuration{ 500 };
  options.writeTimeout = TransportDuration{ 500 };
  return options;
}

TEST(TcpServerTransport, OpenLoopback)
{
  TcpServerTransport server(ServerOptions());

  EXPECT_EQ(TransportStatus::Ok, server.Open());
  EXPECT_TRUE(server.IsOpen());
  EXPECT_NE(0u, server.LocalPort());
  EXPECT_EQ(TransportStatus::Ok, server.Close());
}

TEST(TcpServerTransport, AcceptsClientConnection)
{
  TcpServerTransport server(ServerOptions());
  ASSERT_EQ(TransportStatus::Ok, server.Open());

  TcpStreamTransport client(ClientOptions(server.LocalPort()));
  ASSERT_EQ(TransportStatus::Ok, client.Open());

  std::unique_ptr<IByteStream> accepted;
  EXPECT_EQ(TransportStatus::Ok, server.Accept(accepted));
  ASSERT_TRUE(accepted.get() != 0);
  EXPECT_TRUE(accepted->IsOpen());
}

TEST(TcpServerTransport, AcceptedConnectionReadsAndWritesBytes)
{
  TcpServerTransport server(ServerOptions());
  ASSERT_EQ(TransportStatus::Ok, server.Open());

  TcpStreamTransport client(ClientOptions(server.LocalPort()));
  ASSERT_EQ(TransportStatus::Ok, client.Open());

  std::unique_ptr<IByteStream> accepted;
  ASSERT_EQ(TransportStatus::Ok, server.Accept(accepted));

  const std::uint8_t request[] = { 0x10, 0x20, 0x30 };
  ASSERT_EQ(TransportStatus::Ok, client.WriteAll(request, sizeof(request)));

  std::uint8_t serverBuffer[8] = {};
  std::size_t serverBytes = 0;
  ASSERT_EQ(TransportStatus::Ok, accepted->ReadSome(serverBuffer, sizeof(serverBuffer), serverBytes));
  ASSERT_EQ(3u, serverBytes);
  EXPECT_EQ(0x10, serverBuffer[0]);
  EXPECT_EQ(0x20, serverBuffer[1]);
  EXPECT_EQ(0x30, serverBuffer[2]);

  const std::uint8_t response[] = { 0xA0, 0xB0 };
  ASSERT_EQ(TransportStatus::Ok, accepted->WriteAll(response, sizeof(response)));

  std::uint8_t clientBuffer[8] = {};
  std::size_t clientBytes = 0;
  ASSERT_EQ(TransportStatus::Ok, client.ReadSome(clientBuffer, sizeof(clientBuffer), clientBytes));
  ASSERT_EQ(2u, clientBytes);
  EXPECT_EQ(0xA0, clientBuffer[0]);
  EXPECT_EQ(0xB0, clientBuffer[1]);
}

TEST(TcpServerTransport, AcceptTimeout)
{
  TcpServerTransportOptions options = ServerOptions();
  options.acceptTimeout = TransportDuration{ 1 };
  TcpServerTransport server(options);
  ASSERT_EQ(TransportStatus::Ok, server.Open());

  std::unique_ptr<IByteStream> accepted;
  EXPECT_EQ(TransportStatus::Timeout, server.Accept(accepted));
  EXPECT_TRUE(accepted.get() == 0);
}

TEST(TcpServerTransport, CloseUnblocksAccept)
{
  TcpServerTransportOptions options = ServerOptions();
  options.acceptTimeout = TransportDuration{ 5000 };
  TcpServerTransport server(options);
  ASSERT_EQ(TransportStatus::Ok, server.Open());

  std::promise<TransportStatus> statusPromise;
  std::future<TransportStatus> statusFuture = statusPromise.get_future();
  std::thread worker([&server, &statusPromise]() {
    std::unique_ptr<IByteStream> accepted;
    statusPromise.set_value(server.Accept(accepted));
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_EQ(TransportStatus::Ok, server.Close());

  ASSERT_EQ(std::future_status::ready, statusFuture.wait_for(std::chrono::seconds(2)));
  EXPECT_NE(TransportStatus::Ok, statusFuture.get());
  worker.join();
}

TEST(TcpServerTransport, CloseIsIdempotent)
{
  TcpServerTransport server(ServerOptions());

  EXPECT_EQ(TransportStatus::Ok, server.Close());
  EXPECT_EQ(TransportStatus::Ok, server.Close());
}

} // namespace
