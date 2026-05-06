#include "dlms/transport/serial_port_discovery.hpp"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <algorithm>
#include <dirent.h>
#endif

namespace dlms {
namespace transport {
namespace {

#if defined(_WIN32)

bool QueryDosDeviceExists(const char* name)
{
  char target[256] = {};
  return QueryDosDeviceA(name, target, sizeof(target)) != 0;
}

#else

bool HasPrefix(const std::string& value, const char* prefix)
{
  return value.compare(0, std::string(prefix).size(), prefix) == 0;
}

void AppendUnixPortsFromDirectory(
  const char* directory,
  std::vector<SerialPortInfo>& ports)
{
  DIR* dir = opendir(directory);
  if (dir == 0) {
    return;
  }

  while (dirent* entry = readdir(dir)) {
    const std::string name(entry->d_name);
    if (HasPrefix(name, "ttyS") ||
        HasPrefix(name, "ttyUSB") ||
        HasPrefix(name, "ttyACM") ||
        HasPrefix(name, "cu.")) {
      SerialPortInfo info(std::string(directory) + "/" + name);
      info.displayName = name;
      ports.push_back(info);
    }
  }

  closedir(dir);
}

#endif

} // namespace

TransportStatus PlatformSerialPortDiscovery::ListPorts(std::vector<SerialPortInfo>& ports)
{
  ports.clear();

#if defined(_WIN32)
  for (int index = 1; index <= 256; ++index) {
    char name[16] = {};
    wsprintfA(name, "COM%d", index);
    if (QueryDosDeviceExists(name)) {
      SerialPortInfo info(index > 9 ? std::string("\\\\.\\") + name : std::string(name));
      info.displayName = name;
      ports.push_back(info);
    }
  }
#else
  AppendUnixPortsFromDirectory("/dev", ports);
  std::sort(
    ports.begin(),
    ports.end(),
    [](const SerialPortInfo& left, const SerialPortInfo& right) {
      return left.device < right.device;
    });
#endif

  return TransportStatus::Ok;
}

} // namespace transport
} // namespace dlms
