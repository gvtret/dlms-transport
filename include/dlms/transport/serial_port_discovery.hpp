#pragma once

#include "dlms/transport/transport_status.hpp"

#include <string>
#include <vector>

namespace dlms {
namespace transport {

struct SerialPortInfo
{
  // Device path/name accepted by SerialTransportOptions::device.
  std::string device;

  // Optional user-facing name when the platform exposes one.
  std::string displayName;

  // Optional hardware identifier when the platform exposes one.
  std::string hardwareId;

  SerialPortInfo()
  {
  }

  explicit SerialPortInfo(const std::string& deviceValue)
    : device(deviceValue)
  {
  }
};

class ISerialPortDiscovery
{
public:
  virtual ~ISerialPortDiscovery() {}

  virtual TransportStatus ListPorts(std::vector<SerialPortInfo>& ports) = 0;
};

class PlatformSerialPortDiscovery : public ISerialPortDiscovery
{
public:
  TransportStatus ListPorts(std::vector<SerialPortInfo>& ports);
};

} // namespace transport
} // namespace dlms
