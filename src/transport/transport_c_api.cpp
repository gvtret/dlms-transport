#include "dlms/transport/transport_c_api.h"

#include "dlms/transport/serial_transport.hpp"
#include "dlms/transport/tcp_stream_transport.hpp"
#include "dlms/transport/timer_scheduler.hpp"
#include "dlms/transport/udp_transport.hpp"

#include <new>

struct dlms_transport_timer_t
{
  dlms::transport::TimerScheduler timer;
};

struct dlms_transport_tcp_t
{
  explicit dlms_transport_tcp_t(
    const dlms::transport::TcpStreamTransportOptions& options)
    : transport(options)
  {
  }

  dlms::transport::TcpStreamTransport transport;
};

struct dlms_transport_udp_t
{
  explicit dlms_transport_udp_t(
    const dlms::transport::UdpTransportOptions& options)
    : transport(options)
  {
  }

  dlms::transport::UdpTransport transport;
};

struct dlms_transport_serial_t
{
  explicit dlms_transport_serial_t(
    const dlms::transport::SerialTransportOptions& options)
    : transport(options)
  {
  }

  dlms::transport::SerialTransport transport;
};

namespace {

dlms_transport_status_t ToCStatus(dlms::transport::TransportStatus status)
{
  return static_cast<dlms_transport_status_t>(static_cast<int>(status));
}

dlms::transport::TransportDuration Duration(uint32_t milliseconds)
{
  dlms::transport::TransportDuration duration;
  duration.milliseconds = milliseconds;
  return duration;
}

dlms::transport::SerialParity ToCppParity(
  dlms_transport_serial_parity_t parity)
{
  switch (parity) {
  case DLMS_TRANSPORT_SERIAL_PARITY_NONE:
    return dlms::transport::SerialParity::None;
  case DLMS_TRANSPORT_SERIAL_PARITY_EVEN:
    return dlms::transport::SerialParity::Even;
  case DLMS_TRANSPORT_SERIAL_PARITY_ODD:
    return dlms::transport::SerialParity::Odd;
  }
  return dlms::transport::SerialParity::None;
}

dlms::transport::SerialStopBits ToCppStopBits(
  dlms_transport_serial_stop_bits_t stopBits)
{
  switch (stopBits) {
  case DLMS_TRANSPORT_SERIAL_STOP_BITS_ONE:
    return dlms::transport::SerialStopBits::One;
  case DLMS_TRANSPORT_SERIAL_STOP_BITS_TWO:
    return dlms::transport::SerialStopBits::Two;
  }
  return dlms::transport::SerialStopBits::One;
}

bool ValidParity(dlms_transport_serial_parity_t parity)
{
  return parity == DLMS_TRANSPORT_SERIAL_PARITY_NONE ||
    parity == DLMS_TRANSPORT_SERIAL_PARITY_EVEN ||
    parity == DLMS_TRANSPORT_SERIAL_PARITY_ODD;
}

bool ValidStopBits(dlms_transport_serial_stop_bits_t stopBits)
{
  return stopBits == DLMS_TRANSPORT_SERIAL_STOP_BITS_ONE ||
    stopBits == DLMS_TRANSPORT_SERIAL_STOP_BITS_TWO;
}

} // namespace

extern "C" {

const char* dlms_transport_status_to_string(dlms_transport_status_t status)
{
  return dlms::transport::ToString(
    static_cast<dlms::transport::TransportStatus>(status));
}

dlms_transport_status_t dlms_transport_timer_create(
  dlms_transport_timer_t** timer)
{
  if (timer == 0) {
    return DLMS_TRANSPORT_STATUS_INVALID_ARGUMENT;
  }

  *timer = 0;
  try {
    *timer = new dlms_transport_timer_t();
    return DLMS_TRANSPORT_STATUS_OK;
  } catch (...) {
    return DLMS_TRANSPORT_STATUS_INTERNAL_ERROR;
  }
}

void dlms_transport_timer_destroy(
  dlms_transport_timer_t* timer)
{
  delete timer;
}

dlms_transport_status_t dlms_transport_timer_monotonic_ms(
  const dlms_transport_timer_t* timer,
  uint64_t* value)
{
  if (value != 0) {
    *value = 0u;
  }
  if (timer == 0 || value == 0) {
    return DLMS_TRANSPORT_STATUS_INVALID_ARGUMENT;
  }

  try {
    std::uint64_t cppValue = 0u;
    const dlms::transport::TransportStatus status =
      timer->timer.MonotonicMilliseconds(cppValue);
    *value = cppValue;
    return ToCStatus(status);
  } catch (...) {
    return DLMS_TRANSPORT_STATUS_INTERNAL_ERROR;
  }
}

dlms_transport_status_t dlms_transport_timer_sleep_for(
  dlms_transport_timer_t* timer,
  uint32_t duration_ms)
{
  if (timer == 0) {
    return DLMS_TRANSPORT_STATUS_INVALID_ARGUMENT;
  }

  try {
    return ToCStatus(timer->timer.SleepFor(Duration(duration_ms)));
  } catch (...) {
    return DLMS_TRANSPORT_STATUS_INTERNAL_ERROR;
  }
}

dlms_transport_status_t dlms_transport_tcp_create(
  const dlms_transport_tcp_options_t* options,
  dlms_transport_tcp_t** transport)
{
  if (transport != 0) {
    *transport = 0;
  }
  if (options == 0 || transport == 0 || options->host == 0) {
    return DLMS_TRANSPORT_STATUS_INVALID_ARGUMENT;
  }

  try {
    dlms::transport::TcpStreamTransportOptions cppOptions;
    cppOptions.host = options->host;
    cppOptions.port = options->port;
    cppOptions.connectTimeout = Duration(options->connect_timeout_ms);
    cppOptions.readTimeout = Duration(options->read_timeout_ms);
    cppOptions.writeTimeout = Duration(options->write_timeout_ms);
    *transport = new dlms_transport_tcp_t(cppOptions);
    return DLMS_TRANSPORT_STATUS_OK;
  } catch (...) {
    return DLMS_TRANSPORT_STATUS_INTERNAL_ERROR;
  }
}

void dlms_transport_tcp_destroy(
  dlms_transport_tcp_t* transport)
{
  delete transport;
}

dlms_transport_status_t dlms_transport_tcp_open(
  dlms_transport_tcp_t* transport)
{
  if (transport == 0) {
    return DLMS_TRANSPORT_STATUS_INVALID_ARGUMENT;
  }
  try {
    return ToCStatus(transport->transport.Open());
  } catch (...) {
    return DLMS_TRANSPORT_STATUS_INTERNAL_ERROR;
  }
}

dlms_transport_status_t dlms_transport_tcp_close(
  dlms_transport_tcp_t* transport)
{
  if (transport == 0) {
    return DLMS_TRANSPORT_STATUS_INVALID_ARGUMENT;
  }
  try {
    return ToCStatus(transport->transport.Close());
  } catch (...) {
    return DLMS_TRANSPORT_STATUS_INTERNAL_ERROR;
  }
}

int dlms_transport_tcp_is_open(
  const dlms_transport_tcp_t* transport)
{
  return transport != 0 && transport->transport.IsOpen() ? 1 : 0;
}

dlms_transport_status_t dlms_transport_tcp_read_some(
  dlms_transport_tcp_t* transport,
  uint8_t* output,
  size_t output_size,
  size_t* bytes_read)
{
  if (bytes_read != 0) {
    *bytes_read = 0u;
  }
  if (transport == 0 || bytes_read == 0) {
    return DLMS_TRANSPORT_STATUS_INVALID_ARGUMENT;
  }
  try {
    std::size_t cppBytesRead = 0u;
    const dlms::transport::TransportStatus status =
      transport->transport.ReadSome(output, output_size, cppBytesRead);
    *bytes_read = cppBytesRead;
    return ToCStatus(status);
  } catch (...) {
    return DLMS_TRANSPORT_STATUS_INTERNAL_ERROR;
  }
}

dlms_transport_status_t dlms_transport_tcp_write_all(
  dlms_transport_tcp_t* transport,
  const uint8_t* input,
  size_t input_size)
{
  if (transport == 0) {
    return DLMS_TRANSPORT_STATUS_INVALID_ARGUMENT;
  }
  try {
    return ToCStatus(transport->transport.WriteAll(input, input_size));
  } catch (...) {
    return DLMS_TRANSPORT_STATUS_INTERNAL_ERROR;
  }
}

dlms_transport_status_t dlms_transport_udp_create(
  const dlms_transport_udp_options_t* options,
  dlms_transport_udp_t** transport)
{
  if (transport != 0) {
    *transport = 0;
  }
  if (options == 0 || transport == 0) {
    return DLMS_TRANSPORT_STATUS_INVALID_ARGUMENT;
  }

  try {
    dlms::transport::UdpTransportOptions cppOptions;
    if (options->local_host != 0) {
      cppOptions.localHost = options->local_host;
    }
    cppOptions.localPort = options->local_port;
    if (options->remote_host != 0) {
      cppOptions.remoteHost = options->remote_host;
    }
    cppOptions.remotePort = options->remote_port;
    cppOptions.receiveTimeout = Duration(options->receive_timeout_ms);
    cppOptions.sendTimeout = Duration(options->send_timeout_ms);
    *transport = new dlms_transport_udp_t(cppOptions);
    return DLMS_TRANSPORT_STATUS_OK;
  } catch (...) {
    return DLMS_TRANSPORT_STATUS_INTERNAL_ERROR;
  }
}

void dlms_transport_udp_destroy(
  dlms_transport_udp_t* transport)
{
  delete transport;
}

dlms_transport_status_t dlms_transport_udp_open(
  dlms_transport_udp_t* transport)
{
  if (transport == 0) {
    return DLMS_TRANSPORT_STATUS_INVALID_ARGUMENT;
  }
  try {
    return ToCStatus(transport->transport.Open());
  } catch (...) {
    return DLMS_TRANSPORT_STATUS_INTERNAL_ERROR;
  }
}

dlms_transport_status_t dlms_transport_udp_close(
  dlms_transport_udp_t* transport)
{
  if (transport == 0) {
    return DLMS_TRANSPORT_STATUS_INVALID_ARGUMENT;
  }
  try {
    return ToCStatus(transport->transport.Close());
  } catch (...) {
    return DLMS_TRANSPORT_STATUS_INTERNAL_ERROR;
  }
}

int dlms_transport_udp_is_open(
  const dlms_transport_udp_t* transport)
{
  return transport != 0 && transport->transport.IsOpen() ? 1 : 0;
}

uint16_t dlms_transport_udp_local_port(
  const dlms_transport_udp_t* transport)
{
  return transport != 0 ? transport->transport.LocalPort() : 0u;
}

dlms_transport_status_t dlms_transport_udp_send(
  dlms_transport_udp_t* transport,
  const uint8_t* input,
  size_t input_size)
{
  if (transport == 0) {
    return DLMS_TRANSPORT_STATUS_INVALID_ARGUMENT;
  }
  try {
    return ToCStatus(transport->transport.Send(input, input_size));
  } catch (...) {
    return DLMS_TRANSPORT_STATUS_INTERNAL_ERROR;
  }
}

dlms_transport_status_t dlms_transport_udp_receive(
  dlms_transport_udp_t* transport,
  uint8_t* output,
  size_t output_size,
  size_t* bytes_read)
{
  if (bytes_read != 0) {
    *bytes_read = 0u;
  }
  if (transport == 0 || bytes_read == 0) {
    return DLMS_TRANSPORT_STATUS_INVALID_ARGUMENT;
  }
  try {
    std::size_t cppBytesRead = 0u;
    const dlms::transport::TransportStatus status =
      transport->transport.Receive(output, output_size, cppBytesRead);
    *bytes_read = cppBytesRead;
    return ToCStatus(status);
  } catch (...) {
    return DLMS_TRANSPORT_STATUS_INTERNAL_ERROR;
  }
}

dlms_transport_status_t dlms_transport_serial_create(
  const dlms_transport_serial_options_t* options,
  dlms_transport_serial_t** transport)
{
  if (transport != 0) {
    *transport = 0;
  }
  if (options == 0 || transport == 0 || options->device_name == 0 ||
      !ValidParity(options->parity) || !ValidStopBits(options->stop_bits)) {
    return DLMS_TRANSPORT_STATUS_INVALID_ARGUMENT;
  }

  try {
    dlms::transport::SerialTransportOptions cppOptions;
    cppOptions.deviceName = options->device_name;
    cppOptions.baudRate = options->baud_rate;
    cppOptions.dataBits = options->data_bits;
    cppOptions.parity = ToCppParity(options->parity);
    cppOptions.stopBits = ToCppStopBits(options->stop_bits);
    cppOptions.readTimeout = Duration(options->read_timeout_ms);
    cppOptions.writeTimeout = Duration(options->write_timeout_ms);
    *transport = new dlms_transport_serial_t(cppOptions);
    return DLMS_TRANSPORT_STATUS_OK;
  } catch (...) {
    return DLMS_TRANSPORT_STATUS_INTERNAL_ERROR;
  }
}

void dlms_transport_serial_destroy(
  dlms_transport_serial_t* transport)
{
  delete transport;
}

dlms_transport_status_t dlms_transport_serial_open(
  dlms_transport_serial_t* transport)
{
  if (transport == 0) {
    return DLMS_TRANSPORT_STATUS_INVALID_ARGUMENT;
  }
  try {
    return ToCStatus(transport->transport.Open());
  } catch (...) {
    return DLMS_TRANSPORT_STATUS_INTERNAL_ERROR;
  }
}

dlms_transport_status_t dlms_transport_serial_close(
  dlms_transport_serial_t* transport)
{
  if (transport == 0) {
    return DLMS_TRANSPORT_STATUS_INVALID_ARGUMENT;
  }
  try {
    return ToCStatus(transport->transport.Close());
  } catch (...) {
    return DLMS_TRANSPORT_STATUS_INTERNAL_ERROR;
  }
}

int dlms_transport_serial_is_open(
  const dlms_transport_serial_t* transport)
{
  return transport != 0 && transport->transport.IsOpen() ? 1 : 0;
}

dlms_transport_status_t dlms_transport_serial_read_some(
  dlms_transport_serial_t* transport,
  uint8_t* output,
  size_t output_size,
  size_t* bytes_read)
{
  if (bytes_read != 0) {
    *bytes_read = 0u;
  }
  if (transport == 0 || bytes_read == 0) {
    return DLMS_TRANSPORT_STATUS_INVALID_ARGUMENT;
  }
  try {
    std::size_t cppBytesRead = 0u;
    const dlms::transport::TransportStatus status =
      transport->transport.ReadSome(output, output_size, cppBytesRead);
    *bytes_read = cppBytesRead;
    return ToCStatus(status);
  } catch (...) {
    return DLMS_TRANSPORT_STATUS_INTERNAL_ERROR;
  }
}

dlms_transport_status_t dlms_transport_serial_write_all(
  dlms_transport_serial_t* transport,
  const uint8_t* input,
  size_t input_size)
{
  if (transport == 0) {
    return DLMS_TRANSPORT_STATUS_INVALID_ARGUMENT;
  }
  try {
    return ToCStatus(transport->transport.WriteAll(input, input_size));
  } catch (...) {
    return DLMS_TRANSPORT_STATUS_INTERNAL_ERROR;
  }
}

} // extern "C"
