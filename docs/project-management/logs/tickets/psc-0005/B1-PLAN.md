# B1: PLAN

| Field | Value |
|-------|-------|
| Agent | code-architect |
| Timestamp | 2026-06-14T18:30:00Z |
| Step | B1 |

## Task Summary

PSC-0005: Clean up public API types — rename BaudRate → AtBaudParam, add PIMPL to ABSnifferSerialPort, add sniffer_baud_rate_bps constant, make close_connection() return void, add Doxygen to all public symbols, remove platform headers from public headers, add SerialReadException/SerialWriteException catch handling, add null check in driver constructor.

## Acceptance Criteria

1. **AC-1**: `AtBaudParam` enum class in `ble_sniffer` namespace (not `BaudRate`)
2. **AC-2**: `sniffer_baud_rate_bps` constant defined in `types.h`
3. **AC-3**: `close_connection()` returns `void` in both `SerialPort` interface and `ABSnifferSerialPort` implementation
4. **AC-4**: `set_baud_rate()` accepts `BaudRate` in `SerialPort` and `AtBaudParam` in `BluetoothATDriver`
5. **AC-5**: `baud_rate_from_num()` available in `SerialPort.h` as constexpr template function
6. **AC-6**: PIMPL pattern in `ab_sniffer_serial_port.h` — `struct Impl;` forward declaration, no `termios.h` in header
7. **AC-7**: No `#include <termios.h>` in any public header under `include/`
8. **AC-8**: No `speed_t` or `B115200` constants in any public header
9. **AC-9**: Doxygen `/** @brief */` on all public symbols
10. **AC-10**: Null check in `BluetoothATDriver` constructor for null serial_port
11. **AC-11**: `SerialReadException`/`SerialWriteException` caught in `bluetooth_at_driver.cpp`
12. **AC-12**: Build passes with zero warnings

## Logical Units (Verification — code already implemented)

| # | Unit | Files | AC Satisfied |
|---|------|-------|-------------|
| 1 | Public headers | `include/ble_sniffer/types.h`, `SerialPort.h`, `ab_sniffer_serial_port.h`, `bluetooth_at_driver.h` | AC-1 through AC-9 |
| 2 | Implementation files | `src/bluetooth_at_driver.cpp`, `src/ab_sniffer_serial_port.cpp`, `src/main.cpp` | AC-10, AC-11 |
| 3 | Build | Full project build | AC-12 |

## Approach

Code changes were implemented in Phase A. Phase B is verification: read all affected files, run T1 mechanical checks, confirm acceptance criteria are met, then build.

## Datasheet References

- AT command parameter mapping: https://wiki.aprbrother.com/en/AT_Commands_For_ABSniffer_528.html

## Risks / Unknowns

None identified — all changes are already implemented and verified in Phase A.