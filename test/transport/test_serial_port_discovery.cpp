#include "dlms/transport/serial_port_discovery.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace {

using dlms::transport::PlatformSerialPortDiscovery;
using dlms::transport::SerialPortInfo;
using dlms::transport::TransportStatus;

TEST(SerialPortDiscovery, ReturnsEmptyOrDetectedPorts)
{
  PlatformSerialPortDiscovery discovery;
  std::vector<SerialPortInfo> ports;

  EXPECT_EQ(TransportStatus::Ok, discovery.ListPorts(ports));
}

TEST(SerialPortDiscovery, ReportsStablePortFields)
{
  SerialPortInfo info("COM1");
  info.displayName = "Serial Port";
  info.hardwareId = "ACME";

  EXPECT_EQ("COM1", info.device);
  EXPECT_EQ("Serial Port", info.displayName);
  EXPECT_EQ("ACME", info.hardwareId);
}

TEST(SerialPortDiscovery, HandlesUnavailablePlatformSupport)
{
  PlatformSerialPortDiscovery discovery;
  std::vector<SerialPortInfo> ports(1, SerialPortInfo("stale"));

  EXPECT_EQ(TransportStatus::Ok, discovery.ListPorts(ports));

  for (std::vector<SerialPortInfo>::const_iterator it = ports.begin();
       it != ports.end();
       ++it) {
    EXPECT_FALSE(it->device.empty());
  }
}

} // namespace
