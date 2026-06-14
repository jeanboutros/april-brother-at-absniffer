# B2-2: APPLY Unit 2

| Field | Value |
|-------|-------|
| Agent | code-architect |
| Timestamp | 2026-06-14T12:15:00Z |
| Step | B2-2 |

## Changes Made

### Change 1 — Add [[nodiscard]] to SerialPort::init()

**File:** `include/ble_sniffer/SerialPort.h` (line 197)

**Before:**
```cpp
    virtual bool init() = 0;
```

**After:**
```cpp
    [[nodiscard]] virtual bool init() = 0;
```

**Rationale (per ADR psc-adr-0029-1):** Ensures callers check the return value. Although `ABSnifferSerialPort::init()` always throws on failure (never returns false), the `SerialPort` interface contract permits alternative implementations that may return false. `[[nodiscard]]` enforces that callers don't ignore the return value.

### Change 2 — Add defensive programming comment in main.cpp

**File:** `src/main.cpp` (above line 112, now lines 112-114)

**Before:**
```cpp
        if (!port->init()) {
```

**After:**
```cpp
        // Defensive check: ABSnifferSerialPort::init() always throws on failure (never returns false),
        // but the SerialPort interface contract permits implementations that return false.
        // This check handles alternative SerialPort implementations correctly.
        if (!port->init()) {
```

**Rationale:** Documents why the `!port->init()` check exists despite the current concrete implementation always throwing. Future alternative `SerialPort` implementations may return false, and this check handles them correctly.

## Build Result

| Field | Value |
|-------|-------|
| Build command | `cmake --build --preset conan-debug` |
| Exit code | 0 |
| Warnings | 0 |
| Errors | 0 |

All targets built successfully:
- `libble_sniffer.a` — rebuilt (`ab_sniffer_serial_port.cpp.o`, `bluetooth_at_driver.cpp.o` recompiled)
- `bluetooth-at-driver` — rebuilt (`main.cpp.o` recompiled, relinked)

## Acceptance Criteria Mapping

| AC ID | Description | Status |
|-------|-------------|--------|
| AC-1 | `[[nodiscard]]` attribute present on `SerialPort::init()` virtual declaration | ✅ PASS |
| AC-2 | Defensive comment in `main.cpp` explaining why the `!port->init()` check exists | ✅ PASS |
| AC-3 | Project compiles with zero errors and zero warnings | ✅ PASS |

## Files Changed

| File | Lines Changed |
|------|---------------|
| `include/ble_sniffer/SerialPort.h` | Line 197: added `[[nodiscard]]` attribute |
| `src/main.cpp` | Lines 112-114: added 3-line defensive programming comment |