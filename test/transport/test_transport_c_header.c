#include "dlms/transport/transport_c_api.h"

int dlms_transport_c_header_compiles_as_c(void)
{
  dlms_transport_tcp_options_t tcp_options;
  dlms_transport_udp_options_t udp_options;
  dlms_transport_serial_options_t serial_options;

  tcp_options.host = "127.0.0.1";
  tcp_options.port = 4059u;
  tcp_options.connect_timeout_ms = 1u;
  tcp_options.read_timeout_ms = 1u;
  tcp_options.write_timeout_ms = 1u;

  udp_options.local_host = "127.0.0.1";
  udp_options.local_port = 0u;
  udp_options.remote_host = 0;
  udp_options.remote_port = 0u;
  udp_options.receive_timeout_ms = 1u;
  udp_options.send_timeout_ms = 1u;

  serial_options.device_name = "invalid";
  serial_options.baud_rate = 9600u;
  serial_options.data_bits = 8u;
  serial_options.parity = DLMS_TRANSPORT_SERIAL_PARITY_NONE;
  serial_options.stop_bits = DLMS_TRANSPORT_SERIAL_STOP_BITS_ONE;
  serial_options.read_timeout_ms = 1u;
  serial_options.write_timeout_ms = 1u;

  return (int)DLMS_TRANSPORT_STATUS_OK +
    (int)tcp_options.port +
    (int)udp_options.local_port +
    (int)serial_options.data_bits;
}
