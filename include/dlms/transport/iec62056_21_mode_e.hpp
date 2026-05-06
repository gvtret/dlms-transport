#pragma once

#include "dlms/transport/transport_status.hpp"

#include <cstdint>
#include <string>

namespace dlms {
namespace transport {

enum class Iec62056_21BaudRate
{
  Baud300,
  Baud600,
  Baud1200,
  Baud2400,
  Baud4800,
  Baud9600,
  Baud19200
};

struct Iec62056_21Identification
{
  std::string raw;
  bool supportsModeE;
  Iec62056_21BaudRate maxBaudRate;

  Iec62056_21Identification()
    : supportsModeE(false)
    , maxBaudRate(Iec62056_21BaudRate::Baud300)
  {
  }
};

// Builds the IEC 62056-21 sign-on request: /?! CR LF.
TransportStatus BuildIec62056_21SignOnRequest(std::string& output);

// Parses a meter identification string enough to detect protocol mode E support.
// This helper does not parse HDLC frames or switch the serial device itself.
TransportStatus ParseIec62056_21Identification(
  const std::string& input,
  Iec62056_21Identification& identification);

TransportStatus SelectIec62056_21ModeEBaudRate(
  const Iec62056_21Identification& identification,
  Iec62056_21BaudRate requestedMax,
  Iec62056_21BaudRate& selected);

// Builds ACK 2 Z 2 CR LF for protocol mode E, where Z is the selected baud code.
TransportStatus BuildIec62056_21ModeEAck(
  Iec62056_21BaudRate baudRate,
  std::string& output);

char Iec62056_21BaudRateCode(Iec62056_21BaudRate baudRate);
std::uint32_t Iec62056_21BaudRateValue(Iec62056_21BaudRate baudRate);

} // namespace transport
} // namespace dlms
