# 03. Transport Test Plan

## Interface Contract Tests

```text
ByteStream_closeIsIdempotent
ByteStream_readBeforeOpenReturnsNotOpen
ByteStream_writeBeforeOpenReturnsNotOpen
ByteStream_readNullBufferReturnsInvalidArgument
ByteStream_writeNullBufferReturnsInvalidArgument
ByteStream_writeAllEmptyInputSucceeds

Datagram_closeIsIdempotent
Datagram_receiveBeforeOpenReturnsNotOpen
Datagram_sendBeforeOpenReturnsNotOpen
Datagram_receiveNullBufferReturnsInvalidArgument
Datagram_sendNullBufferReturnsInvalidArgument
Datagram_sendEmptyDatagramPolicyIsDocumented
```

## Fake Transport Tests

```text
FakeByteStream_readScriptedChunks
FakeByteStream_writeCapturesBytes
FakeByteStream_readTimeout
FakeByteStream_peerClose
FakeByteStream_resetClearsState

FakeDatagram_receiveScriptedDatagrams
FakeDatagram_sendCapturesDatagrams
FakeDatagram_receiveTimeout
FakeDatagram_outputBufferTooSmall
FakeDatagram_resetClearsState
```

## TCP Tests

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
```

TCP tests should use a local loopback server owned by the test process. They
must not require external network access.

## UDP Tests

```text
Udp_openLoopback
Udp_sendReceiveLoopbackDatagram
Udp_preservesDatagramBoundary
Udp_receiveBufferTooSmall
Udp_receiveTimeout
Udp_closeIsIdempotent
Udp_sendWithoutRemoteIsInvalid
```

UDP tests should use loopback sockets owned by the test process.

## Serial Tests

```text
Serial_invalidDeviceReturnsOpenFailed
Serial_invalidOptionsReturnInvalidArgument
Serial_closeIsIdempotent
Serial_readAndWriteBeforeOpenReturnNotOpen
```

Serial loopback tests should be optional because they require platform-specific
virtual serial setup.

## C ABI Tests

```text
TransportCApi_statusValuesMatchStableAbi
TransportCApi_timerLifecycle
TransportCApi_tcpLifecycleAndInvalidArguments
TransportCApi_udpLoopbackDatagram
TransportCApi_udpReportsSmallOutputBuffer
TransportCApi_serialLifecycleAndInvalidDevice
TransportCApi_headerCompilesAsC
```

C ABI tests must verify both `DLMS_BUILD_C_API=ON` and `DLMS_BUILD_C_API=OFF`.
The OFF build must not compile or link C ABI tests.

## Integration Tests

Root integration tests should verify only cross-layer contracts:

```text
WrapperTcpProfileChannel_sendsAndReceivesApduBytes
WrapperUdpProfileChannel_sendsOneDatagram
HdlcProfileChannel_readsAndWritesByteStream
```

Transport unit tests remain in `dlms-transport`; protocol integration tests
belong in the root repository.

## v2 Tests

TCP server/listener tests:

```text
TcpServer_openLoopback
TcpServer_acceptsClientConnection
TcpServer_acceptedConnectionReadsAndWritesBytes
TcpServer_acceptTimeout
TcpServer_closeUnblocksAccept
TcpServer_closeIsIdempotent
```

Diagnostic tracing tests:

```text
TraceSink_receivesLifecycleEvents
TraceSink_canBeDisabled
TraceSink_doesNotCopyPayloadByDefault
```

Non-blocking transport tests:

```text
NonBlockingByteStream_returnsWouldBlockWithoutClosing
NonBlockingDatagram_returnsWouldBlockWithoutDroppingDatagram
NonBlockingInterfaces_doNotChangeBlockingContracts
```

Event loop integration tests:

```text
EventLoop_registersReadableWritableTimers
EventLoop_dispatchesTransportCallbacks
EventLoop_unregisterIsIdempotent
```

TLS wrapper tests:

```text
TlsStream_invalidConfigurationReturnsInvalidArgument
TlsStream_wrapsByteStreamLifecycle
TlsStream_reportsHandshakeFailure
```

IEC 62056-21 mode E helper tests:

```text
ModeE_buildsSignOnRequest
ModeE_parsesIdentificationOptions
ModeE_selectsRequestedBaudRate
ModeE_rejectsUnsupportedMode
ModeE_doesNotParseHdlcFrames
```

Serial discovery tests:

```text
SerialDiscovery_returnsEmptyOrDetectedPorts
SerialDiscovery_reportsStablePortFields
SerialDiscovery_handlesUnavailablePlatformSupport
```
