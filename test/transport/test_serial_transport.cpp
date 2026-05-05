#include "dlms/transport/serial_transport.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>

namespace {

using dlms::transport::SerialTransport;
using dlms::transport::SerialTransportOptions;
using dlms::transport::TransportStatus;

SerialTransportOptions InvalidDeviceOptions()
{
  SerialTransportOptions options;
#if defined(_WIN32)
  options.deviceName = "\\\\.\\COM_DOES_NOT_EXIST_DLMS_TRANSPORT";
#else
  options.deviceName = "/dev/dlms-transport-does-not-exist";
#endif
  return options;
}

TEST(SerialTransport, InvalidDeviceReturnsOpenFailed)
{
  SerialTransport transport(InvalidDeviceOptions());

  EXPECT_EQ(TransportStatus::OpenFailed, transport.Open());
  EXPECT_FALSE(transport.IsOpen());
}

TEST(SerialTransport, InvalidOptionsReturnInvalidArgument)
{
  SerialTransportOptions options = InvalidDeviceOptions();
  options.deviceName.clear();
  EXPECT_EQ(TransportStatus::InvalidArgument, SerialTransport(options).Open());

  options = InvalidDeviceOptions();
  options.baudRate = 0;
  EXPECT_EQ(TransportStatus::InvalidArgument, SerialTransport(options).Open());

  options = InvalidDeviceOptions();
  options.dataBits = 4;
  EXPECT_EQ(TransportStatus::InvalidArgument, SerialTransport(options).Open());

  options = InvalidDeviceOptions();
  options.dataBits = 9;
  EXPECT_EQ(TransportStatus::InvalidArgument, SerialTransport(options).Open());
}

TEST(SerialTransport, CloseIsIdempotent)
{
  SerialTransport transport(InvalidDeviceOptions());

  EXPECT_EQ(TransportStatus::Ok, transport.Close());
  EXPECT_EQ(TransportStatus::Ok, transport.Close());
  EXPECT_FALSE(transport.IsOpen());
}

TEST(SerialTransport, ReadAndWriteBeforeOpenReturnNotOpen)
{
  SerialTransport transport(InvalidDeviceOptions());
  std::uint8_t output[1] = {};
  std::size_t bytesRead = 7;
  const std::uint8_t input[] = { 0x01 };

  EXPECT_EQ(TransportStatus::NotOpen, transport.ReadSome(output, sizeof(output), bytesRead));
  EXPECT_EQ(0u, bytesRead);
  EXPECT_EQ(TransportStatus::NotOpen, transport.WriteAll(input, sizeof(input)));
}

} // namespace
