# C2: Specialist Approval (Software Engineer + Test Engineer + Docs Writer)

| Field | Value |
|-------|-------|
| Agent | software-engineer |
| Timestamp | 2026-06-14T22:15:00Z |
| Step | C2-SW |
| Ticket | psc-0029 |

---

## Self-Audit Checklist

| Category | Checked? | Finding or PASS |
|----------|----------|-----------------|
| Build passes (exit 0, no warnings) | yes | PASS — `cmake --build --preset conan-debug` completes with exit 0, both targets built |
| Typed enums / vocabulary types (no raw integers in API) | yes | PASS — `AtBaudParam`, `ScanMode`, `AdvertisementType`, `BaudRate`, `Parity`, `StopBits`, `DataBits`, `FlowControl` all `enum class`; `baud_rate_from_num` template validates; `MessageType` is `enum class`. No raw integer params in public API. |
| Documentation on new public symbols | yes | PASS — `BluetoothATDriver` constructor (line 89) has `@brief`, `@param`, `@example`. `is_connected()` (line 109) has `@brief`, `@return`, `@example`. `SerialPort::init()` has `@brief`, `@return`, `@example`. `[[nodiscard]]` documented. |
| Spec/datasheet fidelity | yes | N/A — no hardware register or protocol changes in this ticket. Changes are C++ API contract enforcement only. |
| Module boundary (no platform headers in shared modules) | yes | PASS — `bluetooth_at_driver.h` includes only `ble_sniffer/messages.h`, `ble_sniffer/SerialPort.h`, `<memory>`, `<string>`. No platform-specific headers. `SerialPort.h` includes only `<cstddef>`, `<stdexcept>`, `<string>`, `<type_traits>`. Clean. |
| Reserved/padding fields handled | yes | N/A — no serialisation struct changes. |
| No magic numbers in doc examples | yes | PASS — all examples use named constants (`AtBaudParam::BAUD_230400`, `ScanMode::ACTIVE`). |
| Buffer safety (bounded copies) | yes | PASS — `read_line()` checks `MAX_READ_BUFFER` (65536) overflow at line 57-61 of bluetooth_at_driver.cpp. `char buf[512]` is stack-allocated with fixed bound. |
| AGENTS.md compliance | yes | PASS — `namespace ble_sniffer`; `m_` prefix for private members; `PascalCase` for classes; `snake_case` for methods; `inline constexpr std::string_view` for AT commands; `\r\n` terminator handled in `send_command`; Doxygen comments present. |
| Conventional commit ready | yes | PASS — changes are ready for a commit message like `fix: enforce precondition in BluetoothATDriver constructor, add [[nodiscard]] to SerialPort::init()`. |

---

## SW (Software Engineer) Review

### SW-1: Does the implementation match ADR psc-adr-0029-2? (Constructor throws `std::invalid_argument` if `!is_open()`)

**PASS — Verified at `bluetooth_at_driver.cpp:14-21`.**

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

The implementation matches ADR psc-adr-0029-2 exactly: `std::invalid_argument` is thrown when `is_open()` returns false. The error message is clear and actionable — it tells the caller exactly what to do ("call init() first").

**Confidence: 95 (Critical)**

### SW-2: Does the implementation match ADR psc-adr-0029-1? (`[[nodiscard]]` on `init()`)

**PASS — Verified at `SerialPort.h:197`.**

```cpp
[[nodiscard]] virtual bool init() = 0;
```

The `[[nodiscard]]` attribute is present. This matches ADR psc-adr-0029-1 which specified: "Keep `bool init()` interface as-is, add `[[nodiscard]]`, and document that ABSnifferSerialPort always throws on failure."

The Doxygen comment on `init()` (lines 194-196) documents the return value: `@return true if the port was opened and configured successfully, false otherwise.`

**Confidence: 95 (Critical)**

### SW-3: Is the error message clear and actionable?

**PASS — Verified at `bluetooth_at_driver.cpp:19`.**

The error message `"serial_port must be open before constructing BluetoothATDriver (call init() first)"` is:
- **Clear**: States exactly what precondition was violated.
- **Actionable**: Tells the caller what to do ("call init() first").
- **Consistent**: Matches the pattern of the null-pointer message at line 16.

**Confidence: 95 (Critical)**

### SW-4: Are there any remaining silent failure paths?

**FINDING F1 — Moderate (70).** Several `void` methods in `BluetoothATDriver` silently no-op when `!is_connected()`:

| Method | Line | Behavior when disconnected |
|--------|------|----------------------------|
| `send_command()` | 33 | Silently returns (no error, no log) |
| `start_scan()` | 109 | Silently returns |
| `stop_scan()` | 113 | Silently returns |
| `set_baud_rate()` | 118 | Silently returns |
| `set_scan_mode()` | 131 | Silently returns |
| `reset_device()` | 136 | Silently returns |

However, **these are pre-existing** and not part of psc-0029. The psc-0029 fix specifically addressed the constructor precondition. The runtime guard `if (!is_connected()) return;` pattern is a separate design concern that should be addressed in a future ticket (potentially converting these to `bool` return values or logging). This is noted as an advisory finding, not a blocker.

Additionally, `send_command()` at line 33 returns `void` and silently no-ops on disconnected port. It also catches exceptions at lines 44-46 but only logs to `stderr` — it doesn't propagate the error. This is another pre-existing pattern.

**Confidence: 70 (Moderate) — advisory, not blocking.**

### SW-5: Is the code consistent with the project's error handling patterns?

**PASS.** The project uses a mix of exceptions and sentinel values:
- `SerialPort::init()` returns `bool` (with `[[nodiscard]]`) or throws `SerialPortException`
- `BluetoothATDriver` constructor throws `std::invalid_argument` for precondition violations
- `read_line()` returns `RawMessage::error()` and `RawMessage::no_data()` sentinel values
- Runtime methods guard with `if (!is_connected()) return;`

The psc-0029 changes follow the established patterns. The constructor throwing for a precondition violation is consistent with the existing null-check at line 16. The `[[nodiscard]]` on `init()` is a new pattern for this project but is a standard C++17 practice.

**Confidence: 90 (High)**

### SW-6: Defensive comment analysis

At `main.cpp:112-114`:

```cpp
// Defensive check: ABSnifferSerialPort::init() always throws on failure (never returns false),
// but the SerialPort interface contract permits implementations that return false.
// This check handles alternative SerialPort implementations correctly.
if (!port->init()) {
```

This comment is **clear and accurate**. It explains why a check that appears to be dead code for the current implementation is actually correct per the interface contract. This aligns with ADR psc-adr-0029-1's rationale.

**Confidence: 90 (High)**

### SW-7: Destructor safety

At `bluetooth_at_driver.cpp:23-30`:

```cpp
BluetoothATDriver::~BluetoothATDriver() {
    if (m_serial_port->is_open()) {
        stop_scan();
        reset_device();
        m_serial_port->close_connection();
    }
    std::cout << "Bluetooth AT Driver cleaned up." << std::endl;
}
```

**FINDING F2 — Low (55).** The destructor calls `stop_scan()` and `reset_device()` which both guard with `if (!is_connected()) return;`, so they're safe even if the port closes mid-destruction. However, `m_serial_port->is_open()` could throw if the serial port is in an invalid state. This is a pre-existing design choice, not introduced by psc-0029. No action required for this ticket.

**Confidence: 55 (Low) — advisory only.**

### SW-8: main.cpp error path coverage (lines 102-123)

The error paths are:

| Line | Error Condition | Handling |
|------|----------------|----------|
| 103-106 | No action flag specified | `std::cerr` message, `return 1` |
| 110-118 | `port->init()` returns false | `std::cerr` message, `return 1` |
| 119-122 | `port->init()` throws `SerialPortException` | `catch` block, `std::cerr` message, `return 1` |
| 123 | `BluetoothATDriver` constructor throws | Not explicitly caught! |

**FINDING F3 — Moderate (75).** If `BluetoothATDriver` constructor throws `std::invalid_argument` (e.g., port became closed between `init()` and construction), it will propagate uncaught and terminate with a non-helpful error. However, this scenario is practically impossible in the current code because `init()` just succeeded and no intervening close call exists. The risk is theoretical. This should be wrapped in a try-catch for robustness, but is not a psc-0029 regression.

**Confidence: 75 (Moderate) — advisory, not blocking.**

---

## TX (Test Engineer) Review

### TX-1: How would you test the `is_open()` precondition?

**Test description:** With a `MockSerialPort` (psc-0014), create two test cases:

1. **Precondition passes:** `MockSerialPort` where `init()` returns `true` and `is_open()` returns `true`. Construct `BluetoothATDriver` with this port. Verify no exception is thrown and the driver is usable.

2. **Precondition fails (not open):** `MockSerialPort` where `is_open()` returns `false`. Attempt to construct `BluetoothATDriver`. Verify `std::invalid_argument` is thrown with the expected message.

3. **Precondition fails (null port):** Construct `BluetoothATDriver` with `nullptr`. Verify `std::invalid_argument` is thrown.

4. **Precondition fails after init failure:** `MockSerialPort` where `init()` returns `false` (or throws). Verify caller error path in main.cpp handles it correctly.

Without `MockSerialPort` infrastructure, these tests cannot be written yet. AC5 (new test cases) is correctly deferred.

**Confidence: 85 (High)**

### TX-2: How would you test the `[[nodiscard]]` attribute?

**Compiler warning test:** Write a test file that calls `port->init()` without capturing the return value:

```cpp
port->init();  // Should produce [[nodiscard]] warning
```

Compile with `-Werror=unused-result` (or equivalent). Verify the compilation fails with a warning about ignoring the nodiscard return value.

This is a compile-time enforcement test, not a runtime test. The `[[nodiscard]]` attribute on a pure virtual function propagates to all implementations.

**Confidence: 90 (High)**

### TX-3: Are all error paths in main.cpp:102-119 covered?

Analysis of main.cpp error paths:

| Path | Line(s) | Covered? | Assessment |
|------|---------|----------|------------|
| No action flag | 103-106 | Yes | Returns 1 with error message |
| `init()` returns false | 115-117 | Yes | Returns 1 with error message |
| `init()` throws SerialPortException | 119-122 | Yes | Caught, returns 1 |
| `BluetoothATDriver` constructor throws | 123 | **No** | Uncaught, will `std::terminate` |
| `ABSnifferSerialPort` constructor throws | 110 | No explicit catch | `SerialPortException` propagates |

The `BluetoothATDriver` constructor throw path is unhandled at line 123. As noted in F3, this is practically impossible in the current flow but should be wrapped for defense in depth.

**Confidence: 80 (High)**

### TX-4: Is AC5 (new test cases) deferred with proper justification?

**PASS.** The test infrastructure (`MockSerialPort`) does not exist yet and is tracked in psc-0014. The psc-0029 fix (constructor precondition) cannot be unit-tested without a mock. The ADR psc-adr-0029-2 notes: "Test code constructing `BluetoothATDriver` must ensure the mock port returns `true` from `is_open()`." This is a clear deferral with a tracked dependency.

**Confidence: 90 (High)**

---

## DX (Docs Writer) Review

### DX-1: Does the Doxygen at line 50 accurately describe the constructor's behavior?

**PASS.** At `bluetooth_at_driver.h:47-75`:

```
 * @brief Serial driver for the ABSniffer 528 BLE sniffer.
 *
 * Takes ownership of an already-initialized serial port on construction and closes it on destruction (RAII).
 * The port must be open (is_open() returns true) before construction; otherwise std::invalid_argument is thrown.
 * All communication is synchronous and blocking up to the configured timeout.
```

And the constructor-specific Doxygen at lines 78-88:

```
 * @brief Construct a driver with the given serial port.
 * @param serial_port Ownership of a SerialPort implementation (moved in).
 *
 * @example
 * @code
 * auto port = std::make_unique<serial::ABSnifferSerialPort>("/dev/ttyUSB0");
 * port->init();
 * ble_sniffer::BluetoothATDriver driver(std::move(port));
 * @endcode
```

This accurately describes:
1. Ownership transfer (moved in)
2. The precondition (port must be open)
3. The exception thrown (`std::invalid_argument`)
4. The correct usage pattern (`port->init()` before construction)

**Confidence: 95 (Critical)**

### DX-2: Does the `@example` block show the correct usage pattern (port->init() before driver)?

**PASS.** The class-level example (lines 55-58) and constructor example (lines 83-86) both show:

```cpp
auto port = std::make_unique<serial::ABSnifferSerialPort>("/dev/ttyUSB0");
port->init();
ble_sniffer::BluetoothATDriver driver(std::move(port));
```

This matches ADR psc-adr-0029-3's decision that the caller is responsible for calling `init()`. The example correctly demonstrates the caller-managed initialization pattern.

**Confidence: 95 (Critical)**

### DX-3: Is the defensive comment clear and accurate?

**PASS.** At `main.cpp:112-114`:

```cpp
// Defensive check: ABSnifferSerialPort::init() always throws on failure (never returns false),
// but the SerialPort interface contract permits implementations that return false.
// This check handles alternative SerialPort implementations correctly.
```

This comment:
- Explains **why** the check exists (defensive programming for interface contracts)
- Explains **what** the current implementation does (always throws)
- Explains **who** the check protects against (alternative implementations)
- Matches ADR psc-adr-0029-1's rationale

**Confidence: 95 (Critical)**

### DX-4: Are the ADRs well-structured and cross-referenced?

**PASS.** All three ADRs follow the project's ADR format:

| ADR | Title | Cross-references |
|-----|-------|-----------------|
| psc-adr-0029-1 | SerialPort::init() Error Reporting Contract | psc-0029, psc-0014, psc-adr-0005 |
| psc-adr-0029-2 | BluetoothATDriver Constructor Precondition | psc-0029, psc-adr-0029-3 |
| psc-adr-0029-3 | SerialPort Initialization Pattern | psc-0029, psc-adr-0029-2 |

Each ADR has: Context, Decision, Options Considered (with clear option labels), Rationale, Consequences (Positive/Negative), and Related entries.

The ADRs form a coherent chain: 0029-1 defines the interface contract → 0029-2 adds the runtime enforcement → 0029-3 explains the architectural rationale for caller-managed initialization.

**Confidence: 95 (Critical)**

---

## Review Findings Summary

| ID | Confidence | Severity | File:Line | Description | Suggested Fix |
|----|-----------|----------|-----------|-------------|---------------|
| F1 | 70 | Moderate | bluetooth_at_driver.cpp:33,109,113,131,136 | Several `void` methods silently no-op when `!is_connected()` — pre-existing pattern, not introduced by psc-0029 | Future ticket: consider `bool` return values or logging |
| F2 | 55 | Low | bluetooth_at_driver.cpp:23-30 | Destructor calls methods on potentially-invalid port — pre-existing, not psc-0029 | No action for this ticket |
| F3 | 75 | Moderate | main.cpp:123 | `BluetoothATDriver` constructor throw path uncaught — theoretically impossible in current flow | Wrap line 123 in try-catch for defense in depth |

### Blocking Findings (confidence ≥ 80)

**None.** All findings are advisory (< 80 confidence).

### Advisory Findings (confidence < 80)

- **F1 (70):** The `void` methods that silently no-op on disconnected port are a pre-existing pattern. Not a psc-0029 concern. Should be tracked in a future ticket for converting to `bool` return values or adding logging.
- **F2 (55):** Destructor calling `is_open()` could theoretically throw on invalid state. Pre-existing, not psc-0029 concern. Low risk.
- **F3 (75):** The `BluetoothATDriver` constructor's `std::invalid_argument` throw path is not explicitly caught in main.cpp. Since `init()` just succeeded, this path is practically unreachable, but wrapping in a try-catch would improve defense in depth.

---

## Specialist Verdicts

### Software Engineer (SW): **APPROVED**

The psc-0029 implementation correctly addresses the silent failure bug by:
1. Adding `[[nodiscard]]` to `SerialPort::init()` (ADR psc-adr-0029-1) ✅
2. Adding constructor precondition check throwing `std::invalid_argument` if `!is_open()` (ADR psc-adr-0029-2) ✅
3. Documenting the caller-managed initialization pattern (ADR psc-adr-0029-3) ✅
4. Adding a clear, actionable error message ✅
5. Updating Doxygen to document the precondition ✅
6. Defensive comment in main.cpp explaining the `if (!port->init())` check ✅

The advisory findings (F1, F2, F3) are all pre-existing patterns or defense-in-depth suggestions, not regressions from psc-0029.

### Test Engineer (TX): **CONDITIONAL PASS**

The implementation is correct but cannot be unit-tested without `MockSerialPort` (psc-0014). AC5 (new test cases) is properly deferred with clear dependency tracking. The `[[nodiscard]]` attribute can be verified with a compiler warning test. The uncaught constructor throw path (F3) is advisory and does not block. **Condition:** When psc-0014 introduces `MockSerialPort`, the following tests must be written: precondition-pass, precondition-fail (not open), precondition-fail (null port).

### Docs Writer (DX): **APPROVED**

All Doxygen documentation is accurate, complete, and consistent with the ADRs. The `@example` blocks show the correct `port->init()` → `driver` construction pattern. The defensive comment in main.cpp clearly explains the rationale. ADRs are well-structured with proper cross-references.

### Overall Verdict: **APPROVED**

No blocking findings (all findings are < 80 confidence). The psc-0029 implementation correctly addresses the root cause (silent failure when init() is not called) through ADR-backed changes: constructor precondition enforcement, `[[nodiscard]]` on `init()`, and documented caller-managed initialization pattern.

