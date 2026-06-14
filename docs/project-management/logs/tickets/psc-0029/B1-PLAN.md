# B1: PLAN — psc-0029

| Field | Value |
|-------|-------|
| Agent | code-architect |
| Timestamp | 2026-06-14T00:00:00Z |
| Step | B1 |

## Acceptance Criteria

| # | Criterion | Satisfied By | Status |
|---|-----------|-------------|--------|
| AC1 | `ABSnifferSerialPort::init()` is called before the port is passed to `BluetoothATDriver`, and failure to initialize produces a clear error message to stderr and a non-zero exit code | main.cpp:108-119 (already applied) + F4 (constructor precondition) | Already applied + Unit 1 |
| AC2 | If the user provides a device path but no action flag (`-i`, `-s`, or `--stop-scan`), the program prints an error message to stderr and exits with a non-zero exit code | main.cpp:103-106 (already applied) | Already applied |
| AC3 | No silent exit with return code 0 when the driver is unusable or no action is requested | AC1 + AC2 + F4 | Already applied + Unit 1 |
| AC4 | All existing tests continue to pass | Build verification | Unit 1, Unit 2 |
| AC5 | New test cases cover: (a) missing action flag → error + non-zero exit, (b) init failure → error + non-zero exit | Deferred to psc-0014 (test infrastructure) | Deferred |
| AC6 | The code changes are reviewed through the full pipeline (A→B→C→C4) to ensure the process violation does not recur | This pipeline execution | In progress |

## Logical Units

| # | Unit | Files | AC Satisfied | Validation |
|---|------|-------|-------------|------------|
| 1 | F1 + F4: Doxygen fix + constructor precondition | `bluetooth_at_driver.h`, `bluetooth_at_driver.cpp` | AC1, AC3 | Build passes |
| 2 | ADR-mandated: `[[nodiscard]]` + defensive comment | `SerialPort.h`, `main.cpp` | AC1, AC3 (documentation) | Build passes |

## Unit 1: F1 (Doxygen Fix) + F4 (Constructor Precondition)

### Files to Change

1. `include/ble_sniffer/bluetooth_at_driver.h` — line 50
2. `src/bluetooth_at_driver.cpp` — lines 14-18

### Change 1a: F1 — Fix Doxygen in `bluetooth_at_driver.h:50`

**Before (line 50):**
```
 * Opens the serial port on construction and closes it on destruction (RAII).
```

**After:**
```
 * Takes ownership of an already-initialized serial port. The port must be
 * open (is_open() returns true) before construction; otherwise
 * std::invalid_argument is thrown. The port is closed on destruction (RAII).
```

**Rationale:** The original Doxygen says "Opens the serial port on construction," which is false — the driver does NOT open the port. It takes ownership of an already-opened port. Per psc-adr-0029-3, the caller is responsible for calling `init()` before passing the port. Per psc-adr-0029-2, the constructor enforces the precondition by throwing if `is_open()` returns false.

### Change 1b: F4 — Add `is_open()` precondition check in `bluetooth_at_driver.cpp:14-18`

**Before (lines 14-18):**
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

**Rationale:** Per psc-adr-0029-2, the constructor must validate the precondition that the port is open. This prevents the exact bug in psc-0029 (constructing a driver with an un-opened port that silently no-ops). The `std::invalid_argument` exception type signals a programmer error (precondition violation), not a runtime error. The existing `main.cpp` code already calls `init()` before construction, so this change is safe for the CLI. Any future caller that forgets to call `init()` will get an immediate, clear exception instead of silent degradation.

### Build Verification Command

```bash
cmake --build --preset conan-debug
```

### Acceptance Criteria Mapping

- **AC1** (init() called before driver, clear error on failure): F4 makes it impossible to construct a driver with an un-opened port — if `init()` was not called, `is_open()` returns false, and the constructor throws.
- **AC3** (no silent exit with code 0 when driver is unusable): F4 ensures that constructing a driver with an unusable port immediately throws `std::invalid_argument`, preventing silent operation.

---

## Unit 2: ADR-Mandated Changes (`[[nodiscard]]` + Defensive Comment)

### Files to Change

1. `include/ble_sniffer/SerialPort.h` — line 197
2. `src/main.cpp` — line 112

### Change 2a: Add `[[nodiscard]]` to `SerialPort::init()` in `SerialPort.h:197`

**Before (line 197):**
```cpp
    virtual bool init() = 0;
```

**After:**
```cpp
    [[nodiscard]] virtual bool init() = 0;
```

**Rationale:** Per psc-adr-0029-1, `init()` returns `bool` to allow alternative implementations to signal failure without throwing. The `[[nodiscard]]` attribute prevents callers from ignoring the return value, which was the root cause of psc-0029. Even though `ABSnifferSerialPort::init()` always throws on failure (never returns `false`), the interface permits `false` returns, and callers must handle both possibilities. This attribute makes it a compile warning/error to write `port->init();` without checking the result.

### Change 2b: Add defensive programming comment at `main.cpp:112`

**Before (lines 111-114):**
```cpp
    try {
        if (!port->init()) {
            std::cerr << "Failed to initialize serial port: " << device_path << std::endl;
            return 1;
```

**After:**
```cpp
    try {
        // Defensive check per SerialPort interface contract (psc-adr-0029-1):
        // ABSnifferSerialPort::init() throws on failure and never returns false,
        // but the interface permits false returns from alternative implementations.
        // This check ensures correctness regardless of the SerialPort implementation.
        if (!port->init()) {
            std::cerr << "Failed to initialize serial port: " << device_path << std::endl;
            return 1;
```

**Rationale:** Per psc-adr-0029-1, the `if (!port->init())` check appears to be dead code for `ABSnifferSerialPort` (which always throws), but it is correct per the `SerialPort` interface contract. The comment explains WHY this check exists and references the ADR, preventing future developers from removing it as "unnecessary."

### Build Verification Command

```bash
cmake --build --preset conan-debug
```

### Acceptance Criteria Mapping

- **AC1** (clear error on init failure): `[[nodiscard]]` prevents silent ignoring of `init()` return value. Defensive comment documents the intentional check.
- **AC3** (no silent exit): Both changes reinforce that `init()` must not be ignored and its result must be checked.

---

## Review of Already-Applied Code (main.cpp:102-119)

The following code was already applied to `main.cpp` before the pipeline was established. This section confirms it is correct per the A1/A2/A3 review.

### Lines 102-106: Action Flag Validation

```cpp
    // Require at least one action flag so the program doesn't exit silently.
    if (!info_flag && !scan_flag && !stop_scan_flag) {
        std::cerr << "Error: No action specified. Use -i (info), -s (scan), or --stop-scan." << std::endl;
        return 1;
    }
```

**Review:** ✅ Satisfies AC2. The check happens before any serial port or driver construction. The error message is clear and actionable. The exit code is 1 (non-zero). No silent exit possible.

### Lines 108-119: Init Call with Error Handling

```cpp
    // Initialize the serial port before passing it to the driver.
    // init() must be called explicitly — the driver does not call it.
    auto port = std::make_unique<serial::ABSnifferSerialPort>(device_path);
    try {
        if (!port->init()) {
            std::cerr << "Failed to initialize serial port: " << device_path << std::endl;
            return 1;
        }
    } catch (const serial::SerialPortException& e) {
        std::cerr << "Error opening serial port: " << e.what() << std::endl;
        return 1;
    }
    ble_sniffer::BluetoothATDriver driver(std::move(port));
```

**Review:** ✅ Satisfies AC1. `init()` is called before the port is passed to the driver. Both failure modes are handled:
- `init()` returning `false` → error message + exit 1
- `init()` throwing `SerialPortException` → caught, error message + exit 1

The comment at line 108-109 correctly documents the init() contract. Unit 2 will add a more detailed defensive comment at line 112.

### Line 120: Driver Construction After Successful Init

```cpp
    ble_sniffer::BluetoothATDriver driver(std::move(port));
```

**Review:** ✅ After Unit 1 (F4), this line will throw `std::invalid_argument` if `is_open()` returns false, providing a fail-fast guarantee even if a future caller forgets to call `init()`. With the current code, this line is guaranteed to receive an open port because `init()` either succeeds or the program exits with an error.

### Conclusion on Already-Applied Code

The already-applied code in `main.cpp:102-119` is correct and complete. It satisfies AC1 and AC2. No changes are needed to this code beyond the defensive comment addition in Unit 2 (Change 2b).

---

## Risks / Unknowns

| # | Risk | Mitigation |
|---|------|-----------|
| 1 | F4 (is_open() precondition) may break existing test code that constructs `BluetoothATDriver` with a mock port | No existing tests found (test infrastructure deferred to psc-0014). When tests are added, mock ports will return `true` from `is_open()`. |
| 2 | `[[nodiscard]]` on `init()` may produce warnings in existing code that calls `init()` without checking the result | No other callers of `init()` found in codebase. The only call site (`main.cpp:112`) already checks the result. |
| 3 | The `[[nodiscard]]` attribute requires C++17 or later | The project uses C++17 per AGENTS.md. No risk. |

## Approach

This is a focused bug-fix ticket with four precise changes across four files. The changes are small, well-defined, and directly traceable to the A3-A-GATE findings and ADR decisions. No new modules, no new types, no new dependencies. Each change is a surgical fix that closes a specific bug vector identified in psc-0029.

1. **Unit 1** (bluetooth_at_driver) corrects the false Doxygen claim and adds the fail-fast constructor precondition — the two most critical changes that directly prevent the psc-0029 bug class from recurring.
2. **Unit 2** (SerialPort.h + main.cpp) adds `[[nodiscard]]` to prevent future callers from silently ignoring `init()` and documents the intentional defensive check.

Both units are validated by a build check. No tests are added in this ticket (AC5 is deferred to psc-0014 when test infrastructure is introduced).