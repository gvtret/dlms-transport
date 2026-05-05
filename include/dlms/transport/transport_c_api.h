#ifndef DLMS_TRANSPORT_TRANSPORT_C_API_H
#define DLMS_TRANSPORT_TRANSPORT_C_API_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Stable C ABI status code returned by transport C entry points.
 *
 * Numeric values are part of the ABI contract. Do not reorder existing values;
 * add future values only at the end.
 */
typedef enum dlms_transport_status_t
{
  DLMS_TRANSPORT_STATUS_OK = 0,
  DLMS_TRANSPORT_STATUS_INVALID_ARGUMENT = 1,
  DLMS_TRANSPORT_STATUS_NOT_OPEN = 2,
  DLMS_TRANSPORT_STATUS_ALREADY_OPEN = 3,
  DLMS_TRANSPORT_STATUS_OPEN_FAILED = 4,
  DLMS_TRANSPORT_STATUS_READ_FAILED = 5,
  DLMS_TRANSPORT_STATUS_WRITE_FAILED = 6,
  DLMS_TRANSPORT_STATUS_TIMEOUT = 7,
  DLMS_TRANSPORT_STATUS_CONNECTION_CLOSED = 8,
  DLMS_TRANSPORT_STATUS_WOULD_BLOCK = 9,
  DLMS_TRANSPORT_STATUS_OUTPUT_BUFFER_TOO_SMALL = 10,
  DLMS_TRANSPORT_STATUS_UNSUPPORTED_FEATURE = 11,
  DLMS_TRANSPORT_STATUS_INTERNAL_ERROR = 12
} dlms_transport_status_t;

typedef enum dlms_transport_serial_parity_t
{
  DLMS_TRANSPORT_SERIAL_PARITY_NONE = 0,
  DLMS_TRANSPORT_SERIAL_PARITY_EVEN = 1,
  DLMS_TRANSPORT_SERIAL_PARITY_ODD = 2
} dlms_transport_serial_parity_t;

typedef enum dlms_transport_serial_stop_bits_t
{
  DLMS_TRANSPORT_SERIAL_STOP_BITS_ONE = 0,
  DLMS_TRANSPORT_SERIAL_STOP_BITS_TWO = 1
} dlms_transport_serial_stop_bits_t;

typedef struct dlms_transport_tcp_options_t
{
  const char* host;
  uint16_t port;
  uint32_t connect_timeout_ms;
  uint32_t read_timeout_ms;
  uint32_t write_timeout_ms;
} dlms_transport_tcp_options_t;

typedef struct dlms_transport_udp_options_t
{
  const char* local_host;
  uint16_t local_port;
  const char* remote_host;
  uint16_t remote_port;
  uint32_t receive_timeout_ms;
  uint32_t send_timeout_ms;
} dlms_transport_udp_options_t;

typedef struct dlms_transport_serial_options_t
{
  const char* device_name;
  uint32_t baud_rate;
  uint8_t data_bits;
  dlms_transport_serial_parity_t parity;
  dlms_transport_serial_stop_bits_t stop_bits;
  uint32_t read_timeout_ms;
  uint32_t write_timeout_ms;
} dlms_transport_serial_options_t;

typedef struct dlms_transport_timer_t dlms_transport_timer_t;
typedef struct dlms_transport_tcp_t dlms_transport_tcp_t;
typedef struct dlms_transport_udp_t dlms_transport_udp_t;
typedef struct dlms_transport_serial_t dlms_transport_serial_t;

const char* dlms_transport_status_to_string(dlms_transport_status_t status);

dlms_transport_status_t dlms_transport_timer_create(
  dlms_transport_timer_t** timer);
void dlms_transport_timer_destroy(
  dlms_transport_timer_t* timer);
dlms_transport_status_t dlms_transport_timer_monotonic_ms(
  const dlms_transport_timer_t* timer,
  uint64_t* value);
dlms_transport_status_t dlms_transport_timer_sleep_for(
  dlms_transport_timer_t* timer,
  uint32_t duration_ms);

dlms_transport_status_t dlms_transport_tcp_create(
  const dlms_transport_tcp_options_t* options,
  dlms_transport_tcp_t** transport);
void dlms_transport_tcp_destroy(
  dlms_transport_tcp_t* transport);
dlms_transport_status_t dlms_transport_tcp_open(
  dlms_transport_tcp_t* transport);
dlms_transport_status_t dlms_transport_tcp_close(
  dlms_transport_tcp_t* transport);
int dlms_transport_tcp_is_open(
  const dlms_transport_tcp_t* transport);
dlms_transport_status_t dlms_transport_tcp_read_some(
  dlms_transport_tcp_t* transport,
  uint8_t* output,
  size_t output_size,
  size_t* bytes_read);
dlms_transport_status_t dlms_transport_tcp_write_all(
  dlms_transport_tcp_t* transport,
  const uint8_t* input,
  size_t input_size);

dlms_transport_status_t dlms_transport_udp_create(
  const dlms_transport_udp_options_t* options,
  dlms_transport_udp_t** transport);
void dlms_transport_udp_destroy(
  dlms_transport_udp_t* transport);
dlms_transport_status_t dlms_transport_udp_open(
  dlms_transport_udp_t* transport);
dlms_transport_status_t dlms_transport_udp_close(
  dlms_transport_udp_t* transport);
int dlms_transport_udp_is_open(
  const dlms_transport_udp_t* transport);
uint16_t dlms_transport_udp_local_port(
  const dlms_transport_udp_t* transport);
dlms_transport_status_t dlms_transport_udp_send(
  dlms_transport_udp_t* transport,
  const uint8_t* input,
  size_t input_size);
dlms_transport_status_t dlms_transport_udp_receive(
  dlms_transport_udp_t* transport,
  uint8_t* output,
  size_t output_size,
  size_t* bytes_read);

dlms_transport_status_t dlms_transport_serial_create(
  const dlms_transport_serial_options_t* options,
  dlms_transport_serial_t** transport);
void dlms_transport_serial_destroy(
  dlms_transport_serial_t* transport);
dlms_transport_status_t dlms_transport_serial_open(
  dlms_transport_serial_t* transport);
dlms_transport_status_t dlms_transport_serial_close(
  dlms_transport_serial_t* transport);
int dlms_transport_serial_is_open(
  const dlms_transport_serial_t* transport);
dlms_transport_status_t dlms_transport_serial_read_some(
  dlms_transport_serial_t* transport,
  uint8_t* output,
  size_t output_size,
  size_t* bytes_read);
dlms_transport_status_t dlms_transport_serial_write_all(
  dlms_transport_serial_t* transport,
  const uint8_t* input,
  size_t input_size);

#ifdef __cplusplus
}
#endif

#endif
