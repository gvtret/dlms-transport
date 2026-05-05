# 00. Transport Requirements

## 1. Scope

`dlms-transport` provides protocol-neutral I/O abstractions used by higher
DLMS/COSEM layers.

The transport layer moves bytes or datagrams. It does not frame DLMS/COSEM
messages and does not parse protocol data units.

Target users:

- `dlms-profile`, which binds transports to HDLC/LLC or Wrapper profile logic;
- integration tests that need fake, loopback, or deterministic I/O channels;
- future client and server facades that create concrete transport instances from
  configuration.

## 2. Fixed Design Decisions

| Area | Decision |
|---|---|
| Language | C++11 |
| Build system | CMake 3.16+ |
| Error handling | Status codes only |
| Exceptions | Not used in public/runtime API paths |
| Layering | Transport is protocol-neutral |
| TCP model | Byte stream |
| Serial model | Byte stream |
| UDP model | Datagram |
| Blocking model v1 | Synchronous API first |
| Async model | Out of scope for v1 |
| C ABI | Out of scope until C++ API stabilizes |
| Tests | GoogleTest |

## 3. Layering Rules

`dlms-transport` must not depend on:

- `dlms-hdlc`;
- `dlms-llc`;
- `dlms-wrapper`;
- `dlms-apdu`;
- `dlms-profile`;
- `dlms-association`;
- `dlms-security`;
- `dlms-cosem`.

Allowed future dependency:

- `dlms-common`, if shared byte-view and status helpers are extracted.

Expected stack position:

```text
DLMS/COSEM client or server facade
xDLMS service layer
Application Association layer
Profile layer
Transport layer
Operating system sockets or serial APIs
```

## 4. Public API Requirements

The first C++ API should expose three separate concepts:

```text
IByteStream
IDatagramTransport
ITimerScheduler
```

`IByteStream` shall support:

```text
open
close
read_some
write_all
is_open
```

`IDatagramTransport` shall support:

```text
open
close
send
receive
is_open
```

`ITimerScheduler` shall support enough behavior for higher layers to implement
timeouts without binding to a specific operating system timer API.

## 5. Transport Status Model

The status model must distinguish at least:

```text
Ok
InvalidArgument
NotOpen
AlreadyOpen
OpenFailed
ReadFailed
WriteFailed
Timeout
ConnectionClosed
WouldBlock
UnsupportedFeature
InternalError
```

Malformed DLMS/COSEM payloads are not transport errors. They belong to higher
layers.

## 6. TCP Requirements

The TCP transport shall:

- open a client connection to a host and port;
- close idempotently;
- read available bytes into caller-provided storage;
- write all bytes from caller-provided input;
- report peer close as `ConnectionClosed`;
- support configurable connect, read, and write timeouts.

TCP server/listener support is planned after the client stream API is stable.

## 7. UDP Requirements

The UDP transport shall:

- send one datagram from caller-provided bytes;
- receive one datagram into caller-provided storage;
- preserve datagram boundaries;
- reject datagrams larger than the caller-provided output buffer.

UDP must not emulate a byte stream.

## 8. Serial Requirements

The serial transport shall:

- open a serial device by path/name;
- configure baud rate, data bits, parity, stop bits, and flow control;
- read and write bytes;
- close idempotently.

Serial discovery and platform-specific friendly names are out of scope for v1.

## 9. Buffer Policy

Primary runtime APIs should use caller-provided buffers.

Convenience APIs may allocate internally only after the strict API is stable.
Allocation failures must be translated to status codes.

## 10. Out of Scope for v1

The following are not implemented in v1:

- DLMS/COSEM APDU framing;
- HDLC flags, CRC, session state, or retransmission;
- Wrapper WPDU parsing;
- Application Association state;
- xDLMS invoke-id handling;
- security and ciphering;
- persistent configuration;
- asynchronous event loops;
- TLS.
