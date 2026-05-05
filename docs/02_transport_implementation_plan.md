# 02. Transport Implementation Plan

## 1. Purpose

This document defines the implementation plan for `dlms-transport`, the
protocol-neutral I/O layer used by the DLMS/COSEM framework.

The plan follows the phase-driven style used by `lib/dlms-hdlc/hdlc_codec_plan.md`:

- each phase has a narrow deliverable;
- each phase has explicit verification criteria;
- each completed phase should be committed separately;
- each phase lists the commit message to use after completion.

## 2. Sources Checked

Local sources:

- `E:/work/dlms/docs/system_architecture.md`;
- `E:/work/dlms/lib/dlms-hdlc/hdlc_codec_plan.md`;
- `E:/work/dlms/lib/dlms-hdlc/docs/00_hdlc_requirements.md`;
- current `dlms-transport` requirements, API, architecture, and test plan.

`doc-rag-remote` sources:

- IEC 62056-47:2006, COSEM transport layers for IPv4 networks;
- IEC 62056-9-7:2013, communication profile for TCP-UDP/IP networks;
- Green Book Ed. 8.3, DLMS/COSEM architecture and protocols;
- DLMS UA White Book glossary.

Relevant conclusions from the external documents:

- COSEM_on_IP has connection-less UDP and connection-oriented TCP transport
  profiles.
- Both UDP and TCP profiles include a wrapper sub-layer above UDP/TCP.
- The wrapper adds wPort addressing and transported-data length information.
- UDP transports one complete wrapper-prefixed APDU in a single datagram.
- TCP transport provides connection establishment, data exchange, connection
  release, and abort/error indication paths.
- Long COSEM messages are not a transport-layer fragmentation feature in this
  repository; higher layers own APDU block transfer and wrapper framing.
- The 3-layer HDLC profile is separate from COSEM_on_IP. Serial/TCP byte streams
  can carry bytes for an HDLC profile, but this repository must not parse HDLC.

## 3. Fixed Design Decisions

| Area | Decision |
|---|---|
| Language | C++11 |
| Build system | CMake 3.16+ |
| Runtime errors | Status codes only |
| Exceptions | Not used in public/runtime API paths |
| First API style | Synchronous caller-provided-buffer API |
| TCP model | Byte stream |
| UDP model | Datagram |
| Serial model | Byte stream |
| Timers | Small scheduler abstraction only |
| Wrapper WPDU | Out of scope; implemented by `dlms-wrapper` |
| HDLC session/framing | Out of scope; implemented by `dlms-hdlc` and `dlms-profile` |
| APDU block transfer | Out of scope; implemented above profile/association layers |
| Server sockets | Deferred until client transports are stable |
| Async/event loop | Out of scope for v1 |
| C ABI | Deferred until C++ API is stable |
| Tests | GoogleTest, loopback/fake only |

## 4. Target Layer Position

Wrapper/TCP profile:

```text
+-----------------------------+
| APDU / Association          |
+-----------------------------+
| dlms-profile                |
+-----------------------------+
| dlms-wrapper                |
+-----------------------------+
| dlms-transport TCP stream   |
+-----------------------------+
```

Wrapper/UDP profile:

```text
+-----------------------------+
| APDU / Association          |
+-----------------------------+
| dlms-profile                |
+-----------------------------+
| dlms-wrapper                |
+-----------------------------+
| dlms-transport UDP datagram |
+-----------------------------+
```

HDLC profile over serial or another byte stream:

```text
+-----------------------------+
| APDU                        |
+-----------------------------+
| dlms-llc                    |
+-----------------------------+
| dlms-hdlc session/codec     |
+-----------------------------+
| dlms-transport byte stream  |
+-----------------------------+
```

`dlms-transport` does not inspect the bytes in any of these profiles.

## 5. v1 Goal

Deliver a portable C++11 transport library that provides:

```text
protocol-neutral byte stream interface
protocol-neutral datagram interface
timer scheduler interface
fake byte stream and fake datagram test doubles
TCP client stream transport
UDP datagram transport
initial serial stream transport API and invalid-device handling
loopback/fake GoogleTest coverage
CMake integration compatible with the root workspace
```

## 6. v1 Non-Goals

```text
Wrapper WPDU parsing or length tracking
HDLC flags, CRC, session state, or retransmission
LLC LPDU handling
APDU parsing or block transfer
association open/release/abort state
COSEM object model
security/ciphering
TLS
async event loop
TCP server/listener
serial device discovery
platform-specific friendly serial names
```

## 7. Project Layout

Target layout:

```text
include/dlms/transport/transport_status.hpp
include/dlms/transport/byte_stream.hpp
include/dlms/transport/datagram_transport.hpp
include/dlms/transport/timer_scheduler.hpp
include/dlms/transport/fake_transport.hpp
include/dlms/transport/tcp_stream_transport.hpp
include/dlms/transport/udp_transport.hpp
include/dlms/transport/serial_transport.hpp
src/transport/tcp_stream_transport.cpp
src/transport/udp_transport.cpp
src/transport/serial_transport.cpp
src/transport/timer_scheduler.cpp
test/CMakeLists.txt
test/transport/test_transport_status.cpp
test/transport/test_fake_transport.cpp
test/transport/test_tcp_stream_transport.cpp
test/transport/test_udp_transport.cpp
test/transport/test_serial_transport.cpp
```

Platform-specific implementations may be split under `src/transport/platform/`
only when a single portable source file becomes unclear.

## 8. Buffer and Blocking Policy

Runtime APIs use caller-provided buffers. Empty writes and empty datagrams are
valid only when the API explicitly documents them.

`ReadSome` may return fewer bytes than requested. `WriteAll` either writes the
complete input or returns a failure status.

`Receive` returns exactly one datagram. If the datagram is larger than the
caller-provided output buffer, it returns `OutputBufferTooSmall` and does not
pretend UDP is a stream.

Timeouts are transport I/O deadlines only. Retry policy belongs to profile,
association, or application layers.

## 9. Implementation Phases

### Phase 0. Requirements Alignment

Deliverables:

```text
docs/00_transport_requirements.md
docs/01_transport_api.md
docs/02_transport_implementation_plan.md
docs/03_transport_test_plan.md
docs/architecture.md
README.md
```

Verification:

```text
Layer boundaries match root system architecture
COSEM_on_IP constraints from IEC 62056-47/9-7 are reflected
HDLC and Wrapper responsibilities are explicitly out of scope
Every future phase has a verification gate and commit message
```

Commit message after completion:

```text
docs: align transport implementation plan with DLMS profiles
```

### Phase 1. Public Interface and Fake Transports

Deliverables:

```text
include/dlms/transport/transport_status.hpp
include/dlms/transport/byte_stream.hpp
include/dlms/transport/datagram_transport.hpp
include/dlms/transport/timer_scheduler.hpp
include/dlms/transport/fake_transport.hpp
CMakeLists.txt
```

Verification:

```text
cmake -S . -B build
cmake --build build
Interfaces compile as C++11
No dependency on HDLC, LLC, Wrapper, APDU, or profile headers
```

Commit message after completion:

```text
feat: add transport interfaces and fake channels
```

### Phase 2. Unit Test Harness

Deliverables:

```text
test/CMakeLists.txt
test/transport/test_transport_status.cpp
test/transport/test_fake_transport.cpp
```

Verification:

```text
DLMS_BUILD_TESTS=ON builds tests
DLMS_USE_SYSTEM_GTEST=ON/OFF path matches sibling repositories
ctest --test-dir build passes
Fake byte stream and datagram contract tests pass
```

Commit message after completion:

```text
test: cover transport interface contracts
```

### Phase 3. Timer Scheduler

Deliverables:

```text
include/dlms/transport/timer_scheduler.hpp
src/transport/timer_scheduler.cpp
test/transport/test_timer_scheduler.cpp
```

Verification:

```text
MonotonicMilliseconds returns non-decreasing values
SleepFor waits at least the requested duration within test tolerance
FakeTimerScheduler remains deterministic for upper-layer tests
No retry policy is introduced
```

Commit message after completion:

```text
feat: add synchronous timer scheduler
```

### Phase 4. TCP Client Stream Transport

Deliverables:

```text
include/dlms/transport/tcp_stream_transport.hpp
src/transport/tcp_stream_transport.cpp
test/transport/test_tcp_stream_transport.cpp
```

Verification:

```text
TcpStream_openLoopback
TcpStream_openInvalidHost
TcpStream_openInvalidPort
TcpStream_writeAllLoopback
TcpStream_readSomeLoopback
TcpStream_peerCloseReturnsConnectionClosed
TcpStream_connectTimeout
TcpStream_readTimeout
TcpStream_closeIsIdempotent
ctest --test-dir build passes
```

Scope notes:

```text
TCP transport does not detect Wrapper APDU boundaries
TCP transport does not add COSEM wPort addressing
TCP transport reports I/O lifecycle statuses only
```

Commit message after completion:

```text
feat: implement TCP client byte stream transport
```

### Phase 5. UDP Datagram Transport

Deliverables:

```text
include/dlms/transport/udp_transport.hpp
src/transport/udp_transport.cpp
test/transport/test_udp_transport.cpp
```

Verification:

```text
Udp_openLoopback
Udp_sendReceiveLoopbackDatagram
Udp_preservesDatagramBoundary
Udp_receiveBufferTooSmall
Udp_receiveTimeout
Udp_closeIsIdempotent
ctest --test-dir build passes
```

Scope notes:

```text
One call to Send sends one datagram
One call to Receive receives one datagram
Wrapper-prefixed APDU size validation belongs above this layer
COSEM UDP source-port and wPort rules belong to profile/wrapper integration
```

Commit message after completion:

```text
feat: implement UDP datagram transport
```

### Phase 6. Serial Stream Transport Skeleton

Deliverables:

```text
include/dlms/transport/serial_transport.hpp
src/transport/serial_transport.cpp
test/transport/test_serial_transport.cpp
```

Verification:

```text
Serial_invalidDeviceReturnsOpenFailed
Serial_invalidOptionsReturnInvalidArgument
Serial_closeIsIdempotent
Build succeeds on the active platform
No virtual serial hardware is required for default tests
```

Scope notes:

```text
Serial transport is a byte stream for HDLC profile users
IEC 62056-21 mode switching is not implemented in v1
Serial discovery and friendly names are deferred
```

Commit message after completion:

```text
feat: add serial stream transport skeleton
```

### Phase 7. Cross-Repository Integration Hooks

Deliverables:

```text
CMake target properties consumed by root repository
DLMS_TRANSPORT_HAS_API cache/internal marker if still needed
Root integration test updates outside this repository when required
```

Verification:

```text
Root workspace configures with dlms-transport enabled
WrapperTcpProfileChannel can depend on IByteStream
WrapperUdpProfileChannel can depend on IDatagramTransport
HdlcProfileChannel can depend on IByteStream
No dependency cycle is introduced
```

Commit message after completion:

```text
build: expose transport targets for profile integration
```

### Phase 8. Documentation and API Comments

Deliverables:

```text
Public header comments for ownership, buffer, timeout, and status semantics
README usage snippets for fake, TCP, UDP, and serial construction
Updated docs/03_transport_test_plan.md if new tests were added
```

Verification:

```text
Every public method documents caller ownership and possible status values
README examples compile or are covered by tests
Documentation still states Wrapper/HDLC/APDU boundaries clearly
```

Commit message after completion:

```text
docs: document transport API ownership and status semantics
```

## 10. v2 Candidates

These are deliberately not part of v1:

```text
TCP server/listener
non-blocking APIs
event-loop integration
TLS transport wrapper
serial IEC 62056-21 mode E handshake helper
platform-specific serial discovery
diagnostic tracing hooks
```

Each candidate needs its own requirements update before implementation.

## 11. Main Risks and Controls

### 11.1 Layer Leakage

Risk:

```text
Transport starts parsing Wrapper, HDLC, LLC, or APDU bytes.
```

Control:

```text
Tests assert byte-for-byte pass-through.
Only profile/wrapper/hdlc repositories validate protocol content.
```

### 11.2 UDP Treated as Stream

Risk:

```text
Receive code truncates or pieces together datagrams.
```

Control:

```text
Dedicated boundary-preservation and output-buffer-too-small tests.
```

### 11.3 Timeout Policy in Wrong Layer

Risk:

```text
Transport grows retry, association, or HDLC retransmission behavior.
```

Control:

```text
Transport exposes I/O deadlines only.
Retry scheduling remains above transport.
```

### 11.4 Platform-Specific Sprawl

Risk:

```text
TCP, UDP, and serial APIs diverge across Windows and POSIX.
```

Control:

```text
Keep public headers platform-neutral.
Hide OS details in src/transport implementation files.
```

## 12. Milestone v1

```text
M1: Portable synchronous DLMS/COSEM transport primitives
```

Included:

```text
C++11
CMake 3.16
status-code API
caller-provided buffers
fake byte stream and datagram channels
timer scheduler
TCP client byte stream
UDP datagram transport
serial stream skeleton
loopback/fake GoogleTest coverage
root workspace integration hooks
```

Not included:

```text
Wrapper codec
HDLC codec/session
APDU parsing
association state
COSEM object model
security
async APIs
TCP server/listener
TLS
```
