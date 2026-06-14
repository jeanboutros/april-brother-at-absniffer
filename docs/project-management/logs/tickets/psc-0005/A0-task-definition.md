# A0: Task Definition — psc-0005

## Task
Remove termios.h from public header (POSIX testability)

## Domain Classification
- **Task type:** bugfix (POSIX leak in public headers)
- **Domain signals:** None — this is a code quality/refactoring task touching serial I/O abstraction. No hardware registers, no wireless protocol, no auth/crypto, no UI/UX.
- **Security auto-inclusion:** Not triggered — serial I/O is local, not network-facing.

## Specialist Roster
```
Roster: SW, TX, DX
Total: 3 specialists
Domain signals detected: [none]
```

- SW: Software Engineer — architecture review, API design, SOLID compliance
- TX: Test Engineer — test strategy, mock testability assessment
- DX: Docs Writer — documentation quality, cross-references

## Files Affected
- `include/ble_sniffer/bluetooth_at_driver.h` — remove termios, update Doxygen
- `include/ble_sniffer/ab_sniffer_serial_port.h` — PIMPL to hide termios
- `include/ble_sniffer/SerialPort.h` — close_connection return type, set_baud_rate
- `include/ble_sniffer/types.h` — AtBaudParam rename, sniffer_baud_rate_bps constant
- `src/bluetooth_at_driver.cpp` — remove POSIX headers, fix errno catch
- `src/ab_sniffer_serial_port.cpp` — PIMPL implementation, close_connection void

## Acceptance Criteria
1. `#include <termios.h>` NOT in any public header
2. `speed_t` and `B115200` do NOT appear in any public header
3. Public API exposes `constexpr int sniffer_baud_rate_bps = 115200`
4. `<termios.h>` only in implementation `.cpp` files
5. `B115200` only used in serial port implementation `.cpp`
6. Build passes on Linux and macOS

## Test Strategy
- No unit tests exist yet (tracked in T1-T3 as separate ticket)
- Build verification is the primary test
- Future: MockSerialPort tests for BluetoothATDriver

## Doc Plan
- Update all Doxygen in affected headers
- Add `@file`/`@brief`/`@example` to ab_sniffer_serial_port.h
- Cross-reference AtBaudParam ↔ serial::BaudRate in Doxygen