#include "dlms/transport/iec62056_21_mode_e.hpp"

#include <algorithm>

namespace dlms {
namespace transport {
namespace {

bool IsKnownBaudRate(Iec62056_21BaudRate baudRate)
{
  switch (baudRate) {
  case Iec62056_21BaudRate::Baud300:
  case Iec62056_21BaudRate::Baud600:
  case Iec62056_21BaudRate::Baud1200:
  case Iec62056_21BaudRate::Baud2400:
  case Iec62056_21BaudRate::Baud4800:
  case Iec62056_21BaudRate::Baud9600:
  case Iec62056_21BaudRate::Baud19200:
    return true;
  }
  return false;
}

TransportStatus BaudRateFromCode(char code, Iec62056_21BaudRate& baudRate)
{
  switch (code) {
  case '0':
    baudRate = Iec62056_21BaudRate::Baud300;
    return TransportStatus::Ok;
  case '1':
    baudRate = Iec62056_21BaudRate::Baud600;
    return TransportStatus::Ok;
  case '2':
    baudRate = Iec62056_21BaudRate::Baud1200;
    return TransportStatus::Ok;
  case '3':
    baudRate = Iec62056_21BaudRate::Baud2400;
    return TransportStatus::Ok;
  case '4':
    baudRate = Iec62056_21BaudRate::Baud4800;
    return TransportStatus::Ok;
  case '5':
    baudRate = Iec62056_21BaudRate::Baud9600;
    return TransportStatus::Ok;
  case '6':
    baudRate = Iec62056_21BaudRate::Baud19200;
    return TransportStatus::Ok;
  default:
    return TransportStatus::InvalidArgument;
  }
}

int BaudRank(Iec62056_21BaudRate baudRate)
{
  return static_cast<int>(baudRate);
}

} // namespace

TransportStatus BuildIec62056_21SignOnRequest(std::string& output)
{
  output = "/?!\r\n";
  return TransportStatus::Ok;
}

TransportStatus ParseIec62056_21Identification(
  const std::string& input,
  Iec62056_21Identification& identification)
{
  identification = Iec62056_21Identification();

  if (input.size() < 5 || input[0] != '/') {
    return TransportStatus::InvalidArgument;
  }
  if (input.size() < 2 || input[input.size() - 2] != '\r' || input[input.size() - 1] != '\n') {
    return TransportStatus::InvalidArgument;
  }

  identification.raw = input;

  const TransportStatus baudStatus = BaudRateFromCode(input[4], identification.maxBaudRate);
  if (baudStatus != TransportStatus::Ok) {
    return baudStatus;
  }

  const std::string modeMarker = "\\W";
  const std::string::size_type marker = input.find(modeMarker);
  if (marker == std::string::npos || marker + modeMarker.size() >= input.size()) {
    identification.supportsModeE = false;
    return TransportStatus::Ok;
  }

  identification.supportsModeE = input[marker + modeMarker.size()] == '2';
  return TransportStatus::Ok;
}

TransportStatus SelectIec62056_21ModeEBaudRate(
  const Iec62056_21Identification& identification,
  Iec62056_21BaudRate requestedMax,
  Iec62056_21BaudRate& selected)
{
  selected = Iec62056_21BaudRate::Baud300;
  if (!identification.supportsModeE || !IsKnownBaudRate(requestedMax)) {
    return TransportStatus::UnsupportedFeature;
  }

  selected = BaudRank(identification.maxBaudRate) < BaudRank(requestedMax)
    ? identification.maxBaudRate
    : requestedMax;
  return TransportStatus::Ok;
}

TransportStatus BuildIec62056_21ModeEAck(
  Iec62056_21BaudRate baudRate,
  std::string& output)
{
  if (!IsKnownBaudRate(baudRate)) {
    output.clear();
    return TransportStatus::InvalidArgument;
  }

  output.clear();
  output.push_back(0x06);
  output.push_back('2');
  output.push_back(Iec62056_21BaudRateCode(baudRate));
  output.push_back('2');
  output.push_back('\r');
  output.push_back('\n');
  return TransportStatus::Ok;
}

char Iec62056_21BaudRateCode(Iec62056_21BaudRate baudRate)
{
  switch (baudRate) {
  case Iec62056_21BaudRate::Baud300:
    return '0';
  case Iec62056_21BaudRate::Baud600:
    return '1';
  case Iec62056_21BaudRate::Baud1200:
    return '2';
  case Iec62056_21BaudRate::Baud2400:
    return '3';
  case Iec62056_21BaudRate::Baud4800:
    return '4';
  case Iec62056_21BaudRate::Baud9600:
    return '5';
  case Iec62056_21BaudRate::Baud19200:
    return '6';
  }
  return '?';
}

std::uint32_t Iec62056_21BaudRateValue(Iec62056_21BaudRate baudRate)
{
  switch (baudRate) {
  case Iec62056_21BaudRate::Baud300:
    return 300;
  case Iec62056_21BaudRate::Baud600:
    return 600;
  case Iec62056_21BaudRate::Baud1200:
    return 1200;
  case Iec62056_21BaudRate::Baud2400:
    return 2400;
  case Iec62056_21BaudRate::Baud4800:
    return 4800;
  case Iec62056_21BaudRate::Baud9600:
    return 9600;
  case Iec62056_21BaudRate::Baud19200:
    return 19200;
  }
  return 0;
}

} // namespace transport
} // namespace dlms
