# A2a: ADR Creation — psc-0029

## Ticket
psc-0029 — Silent failure: init() never called and no-flag exit gives no feedback

## ADRs Created

| ADR ID | Title | Decision | Priority |
|--------|-------|----------|----------|
| psc-adr-0029-1 | SerialPort::init() Error Reporting Contract | Keep bool init() interface, add [[nodiscard]], document contract. Defensive check in main.cpp:112 is intentional. | High |
| psc-adr-0029-2 | BluetoothATDriver Constructor Precondition | Throw std::invalid_argument if !port->is_open() in constructor. Document precondition in Doxygen. | High |
| psc-adr-0029-3 | SerialPort Initialization Pattern | Caller calls init(), driver validates is_open() precondition. Follows DIP. | Medium |

## Deferred Decisions
- SerialPort::init() return type change (bool → void) deferred to psc-0014 (MockSerialPort)
- Device responsiveness validation after init() deferred to separate ticket
- --stat without -s UX gap — low priority, deferred

## Related
- psc-adr-0005 (close_connection() returns void)
- psc-0014 (MockSerialPort — deferred test infrastructure)