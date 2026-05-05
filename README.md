# dlms-transport

`dlms-transport` will provide protocol-neutral I/O primitives for the
DLMS/COSEM framework.

The library is a lower infrastructure layer. It must not know about HDLC, LLC,
Wrapper, APDU, Application Associations, COSEM objects, or security policy.

Planned transports:

- TCP byte stream;
- UDP datagram transport;
- serial byte stream;
- timer abstraction for timeout and retry policies owned by higher layers.

## Usage

Fake channels are intended for upper-layer unit tests. They use caller-owned
buffers and preserve the same status semantics as concrete transports:

```cpp
#include "dlms/transport/fake_transport.hpp"

dlms::transport::FakeByteStream stream;
stream.Open();

const std::uint8_t request[] = {0x01, 0x02};
stream.WriteAll(request, sizeof(request));
```

TCP is a byte stream. Wrapper framing, APDU parsing, retries, and association
state are owned by higher layers:

```cpp
#include "dlms/transport/tcp_stream_transport.hpp"

dlms::transport::TcpStreamTransportOptions options;
options.host = "127.0.0.1";
options.port = 4059;

dlms::transport::TcpStreamTransport stream(options);
dlms::transport::TransportStatus status = stream.Open();
```

UDP is datagram-oriented. One `Send` is one UDP datagram, and one `Receive`
returns one UDP datagram:

```cpp
#include "dlms/transport/udp_transport.hpp"

dlms::transport::UdpTransportOptions options;
options.localHost = "127.0.0.1";
options.remoteHost = "127.0.0.1";
options.remotePort = 4059;

dlms::transport::UdpTransport datagram(options);
dlms::transport::TransportStatus status = datagram.Open();
```

Serial is a byte stream for HDLC profile users. IEC 62056-21 mode switching,
serial discovery, and HDLC framing are outside this layer:

```cpp
#include "dlms/transport/serial_transport.hpp"

dlms::transport::SerialTransportOptions options;
options.deviceName = "\\\\.\\COM3";
options.baudRate = 9600;

dlms::transport::SerialTransport stream(options);
dlms::transport::TransportStatus status = stream.Open();
```

See:

- [requirements](docs/00_transport_requirements.md)
- [API](docs/01_transport_api.md)
- [implementation plan](docs/02_transport_implementation_plan.md)
- [architecture](docs/architecture.md)
- [test plan](docs/03_transport_test_plan.md)
