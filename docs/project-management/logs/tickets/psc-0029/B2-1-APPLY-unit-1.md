# B2-1: APPLY Unit 1

| Field | Value |
|-------|-------|
| Agent | code-architect |
| Timestamp | 2026-06-14T12:00:00Z |
| Step | B2-1 |

## Changes Made

### F1 — Fix Doxygen in bluetooth_at_driver.h

**File:** `include/ble_sniffer/bluetooth_at_driver.h` (line 50)

**Before:**
```cpp
 * Opens the serial port on construction and closes it on destruction (RAII).
```

**After:**
```cpp
 * Takes ownership of an already-initialized serial port on construction and closes it on destruction (RAII).
 * The port must be open (is_open() returns true) before construction; otherwise std::invalid_argument is thrown.
```

**@example block verification:** Both `@example` blocks (lines 18-20 and 55-57) already show `port->init()` being called before `BluetoothATDriver driver(std::move(port))`. No changes needed.

### F4 — Add is_open() precondition check in constructor

**File:** `src/bluetooth_at_driver.cpp` (lines 14-22)

**Before:**
```cpp
BluetoothATDriver::BluetoothATDriver(std::unique_ptr<serial::SerialPort> serial_port) : m_serial_port(std::move(serial_port)) {
    if (!m_serial_port) {
        throw std::invalid_argument("serial_port must not be null");
    }
}
```

**After:**
```cpp
BluetoothATDriver::BluetoothATDriver(std::unique_ptr<serial::SerialPort> serial_port) : m_serial_port(std::move(serial_port)) {
    if (!m_serial_port) {
        throw std::invalid_argument("serial_port must not be null");
    }
    if (!m_serial_port->is_open()) {
        throw std::invalid_argument("serial_port must be open before constructing BluetoothATDriver (call init() first)");
    }
}
```

## Build Result

| Field | Value |
|-------|-------|
| Build command | `cmake --build --preset conan-debug` |
| Exit code | 0 |
| Warnings | 0 |
| Errors | 0 |

All targets built successfully:
- `libble_sniffer.a` — rebuilt (`bluetooth_at_driver.cpp.o` recompiled)
- `bluetooth-at-driver` — relinked

## Acceptance Criteria Mapping

| AC ID | Description | Status |
|-------|-------------|--------|
| F1 | Doxygen comment accurately describes constructor ownership semantics (takes ownership, not opens) | ✅ PASS |
| F1 | @example blocks show `port->init()` before constructing driver | ✅ PASS (already correct, no change needed) |
| F4 | Constructor throws `std::invalid_argument` if port is not open | ✅ PASS |
| F4 | Error message mentions calling `init()` first | ✅ PASS |
| Build | Project compiles with zero errors and zero warnings | ✅ PASS |

## Files Changed

| File | Lines Changed |
|------|---------------|
| `include/ble_sniffer/bluetooth_at_driver.h` | Line 50: replaced 1 line with 2 lines |
| `src/bluetooth_at_driver.cpp` | Lines 18-19: added 3 lines (is_open check) |