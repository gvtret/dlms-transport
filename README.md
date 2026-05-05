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

See:

- [requirements](docs/00_transport_requirements.md)
- [API](docs/01_transport_api.md)
- [architecture](docs/architecture.md)
- [test plan](docs/03_transport_test_plan.md)
