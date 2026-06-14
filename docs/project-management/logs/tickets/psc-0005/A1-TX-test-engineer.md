# A1-TX: Test Engineer Review

| Field | Value |
|-------|-------|
| Agent | test-engineer |
| Timestamp | 2026-06-14T13:00:00Z |
| Step | A1-TX |
| Verdict | CONDITIONAL PASS |
| Coverage | 4/6 acceptance criteria have test evidence (via code review); 0/6 have executable test evidence |

## Findings

| # | Category | Finding | Confidence | Severity |
|---|----------|---------|------------|----------|
| F1 | Mockability — Interface | ✅ `serial::SerialPort` is a pure virtual struct with virtual destructor. A `MockSerialPort` can be created that implements all 6 virtual methods (`init`, `is_open`, `read`, `write`, `close_connection`, `set_baud_rate`). This was the PRIMARY GOAL of PSC-0005 and it is achieved. | 95 | — |
| F2 | Mockability — Constructor | ✅ `BluetoothATDriver` takes `std::unique_ptr<serial::SerialPort>` — dependency injection via move. No hard-wired creation. Mock can be injected. | 95 | — |
| F3 | Error contract — `read()` returns `size_t` | ⚠️ `SerialPort::read()` returns `size_t`. Returns 0 for BOTH timeout and error. `BluetoothATDriver::read_line()` distinguishes: `n == 0` → `RawMessage::no_data()` (timeout), and `catch (SerialReadException)` → `RawMessage::error()`. However, the ABSENCE of an exception on a genuine error (e.g., `::read()` returning 0 when the port is disconnected but not raising an error) means the driver cannot distinguish a timeout from a silent disconnection. This is a testability concern — tests need to verify both paths, and the interface makes it impossible to inject a "silent failure" that returns 0 without throwing. | 85 | Medium |
| F4 | Error contract — `write()` returns `size_t` | ⚠️ `SerialPort::write()` returns `size_t`. Returns 0 for "not open" and also could return 0 for partial write. `send_command()` logs partial writes to stderr but doesn't propagate the error up. A mock `write()` returning 0 would test "not open" (which is guarded by `is_open()`) but cannot easily test partial-write semantics since `send_command()` is void. | 75 | Low |
| F5 | PIMPL effectiveness | ✅ PIMPL in `ABSnifferSerialPort` successfully hides `struct termios`, `int file_descriptor`, `std::optional<struct termios> original_tty`, and `uint16_t timeout_ms` from the header. The header only contains `std::unique_ptr<Impl> m_impl`. This means: (a) `#include <termios.h>` only in `.cpp` — confirmed, (b) mock creators don't need POSIX headers, (c) compilation on non-POSIX platforms (for mock compilation) is possible. | 95 | — |
| F6 | Test infrastructure gap — BLOCKING | 🚫 No Catch2 dependency, no test directory, no test CMakeLists, no `MockSerialPort`, no test files. The ENTIRE PURPOSE of PSC-0005 was to enable mock-based testing. While the interface refactoring is correct, the absence of ANY test infrastructure means the goal is structurally unfulfilled. Without a test, we cannot verify that: (a) the `SerialPort` interface actually works with a mock, (b) `BluetoothATDriver` behaves correctly when the mock returns specific values, (c) the refactoring didn't introduce regressions. | 100 | High |
| F7 | `bluetooth_at_driver.h` no longer includes termios | ✅ Confirmed: `bluetooth_at_driver.h` includes `<ble_sniffer/messages.h>`, `<ble_sniffer/SerialPort.h>`, `<memory>`, `<string>`. No `<termios.h>`. | 100 | — |
| F8 | `ab_sniffer_serial_port.h` no longer includes termios | ✅ Confirmed: `ab_sniffer_serial_port.h` includes `<ble_sniffer/SerialPort.h>`, `<ble_sniffer/types.h>`, `<memory>`, `<string>`. No `<termios.h>`. All POSIX types are behind PIMPL. | 100 | — |
| F9 | `sniffer_baud_rate_bps` is now a portable constant | ✅ `types.h` line 80: `inline constexpr int sniffer_baud_rate_bps = 115200`. No `speed_t`, no `B115200` in any public header. | 100 | — |
| F10 | `B115200` only in implementation | ✅ Confirmed: `B115200` and all `B*` macros only appear in `ab_sniffer_serial_port.cpp` inside `baud_rate_to_speed_t()`. | 100 | — |
| F11 | `BaudRate` enum naming conflict resolved | ✅ `ble_sniffer::BaudRate` was renamed to `ble_sniffer::AtBaudParam`. `serial::BaudRate` is the transport-level enum. No naming conflict. | 95 | — |

## Testability Assessment

### Can a MockSerialPort be created?

**Yes.** The `serial::SerialPort` interface is fully mockable:

```cpp
struct MockSerialPort : public serial::SerialPort {
    bool init() override { return init_result; }
    bool is_open() const override { return is_open_result; }
    std::size_t read(char* buffer, std::size_t max_length) override { /* configurable */ }
    std::size_t write(const char* data, std::size_t length) override { /* configurable */ }
    void close_connection() override { close_called = true; }
    bool set_baud_rate(serial::BaudRate baud_rate) override { /* configurable */ }

    // Test state
    bool init_result = true;
    bool is_open_result = true;
    bool close_called = false;
    std::size_t read_result = 0;
    std::size_t write_result = 0;
    std::string last_write_data;
};
```

No POSIX headers required. This is the key achievement of PSC-0005.

### Error contract assessment

| Method | Return type | Ambiguity | Impact on tests |
|--------|------------|-----------|-----------------|
| `read()` | `size_t` | 0 = timeout OR error (distinguished only by exception) | Mock can throw `SerialReadException` for error, return 0 for timeout. Adequate but fragile — any new error path that doesn't throw will be misinterpreted as timeout. |
| `write()` | `size_t` | 0 = not-open OR partial write | `send_command()` guards with `is_open()`, so mock tests can control this via `is_open_result`. Partial write logging is not testable (void function, logs to stderr). |
| `init()` | `bool` | `false` = any init failure | Clean — mock can return `false`. |
| `set_baud_rate()` | `bool` | `false` = not-open or failure | Clean — mock can return `false`. |

### Minimum test cases needed (PSC-0014 dependency)

These tests cannot be written until PSC-0014 (Catch2 infrastructure) is complete. Listing them as requirements for that ticket:

| # | Test Case | Module | Mock Needed |
|---|-----------|--------|-------------|
| T1 | `MockSerialPort` compiles without `<termios.h>` | Test infrastructure | N/A — this IS the mock |
| T2 | `BluetoothATDriver` constructor accepts `MockSerialPort` | `BluetoothATDriver` | `MockSerialPort` |
| T3 | `read_line()` returns `RawMessage::no_data()` when `read()` returns 0 | `BluetoothATDriver` | `MockSerialPort` |
| T4 | `read_line()` returns `RawMessage::error()` when `read()` throws `SerialReadException` | `BluetoothATDriver` | `MockSerialPort` |
| T5 | `read_line()` parses "OK+SCAN:..." into `MessageType::SCAN_RESULT` | `BluetoothATDriver` | `MockSerialPort` |
| T6 | `send_command()` writes correct string with `\r\n` | `BluetoothATDriver` | `MockSerialPort` |
| T7 | `send_command()` does nothing when `is_open()` returns false | `BluetoothATDriver` | `MockSerialPort` |
| T8 | `read()` buffer overflow protection (> MAX_READ_BUFFER) | `BluetoothATDriver` | `MockSerialPort` |
| T9 | `is_connected()` delegates to `SerialPort::is_open()` | `BluetoothATDriver` | `MockSerialPort` |
| T10 | `set_baud_rate()` calls `send_command()` AND `set_baud_rate()` on serial port | `BluetoothATDriver` | `MockSerialPort` |
| T11 | `close_connection()` called in destructor | `BluetoothATDriver` | `MockSerialPort` |
| T12 | `BaudRate` enum round-trip: `baud_rate_from_num(115200)` → `BaudRate::BAUD_115200` | `SerialPort` | None (pure function) |
| T13 | `BaudRate` enum throws on invalid value | `SerialPort` | None (pure function) |
| T14 | `AtBaudParam` round-trip: `at_baud_param_to_num(AtBaudParam::BAUD_115200)` → 115200 | `types` | None (pure function) |
| T15 | `at_baud_param_from_num` throws on invalid value | `types` | None (pure function) |
| T16 | `RawMessage::parse()` — known vectors | `messages` | None (pure function) |
| T17 | `ScanResultMessage::from()` — known vectors | `messages` | None (pure function) |
| T18 | `address_to_mac_address()` — format and edge cases | `types` | None (pure function) |

### Is the PIMPL change testable?

**Yes.** The PIMPL change is testable indirectly:
- A `MockSerialPort` that compiles without `<termios.h>` IS the test that the PIMPL refactoring achieved its goal.
- If we can compile and link a test file that includes only `<ble_sniffer/SerialPort.h>` and `<ble_sniffer/bluetooth_at_driver.h>` (no `<termios.h>`) and creates a `MockSerialPort`, then PSC-0005 is verified.
- This is a **compile-time test** as much as a runtime test.

## Verdict

**CONDITIONAL PASS**

### Rationale

PSC-0005's primary goal — removing `<termios.h>` from public headers to enable mock-based testing — is **architecturally achieved**. The `SerialPort` interface is clean, pure virtual, and mockable. The PIMPL pattern in `ABSnifferSerialPort` correctly hides all POSIX types. The naming conflicts are resolved. The build passes.

However, the goal remains **unfulfilled in practice** because:

1. **No test infrastructure exists** (P0 gap noted in the ticket itself as T1/T2). No Catch2, no test directory, no test CMakeLists, no `MockSerialPort`. The refactoring's value cannot be validated without at least one test proving a `MockSerialPort` compiles and works.
2. **No compile-time verification** that the refactoring actually removed the dependency. A simple `static_assert` or compilation test that includes only `SerialPort.h` and creates a mock would validate the core claim.

### Conditions for FULL APPROVAL

1. **PSC-0014 (Catch2 infrastructure) must be completed** or at minimum a smoke-test `test/` directory with one `MockSerialPort` test that compiles and links without `<termios.h>`.
2. **A compilation test** that includes only `SerialPort.h` and creates a mock struct — this proves the POSIX dependency is fully removed from the public API surface.

### What does NOT block this ticket

- The `read()` error contract (0 = timeout vs. error) is a design concern, not a PSC-0005 regression. It was like this before and after. It should be tracked as a separate advisory.
- The test cases listed above (T1-T18) are requirements for PSC-0014, not PSC-0005. PSC-0005's scope was the refactoring, not the tests.

### Routing

- **PSC-0005**: CONDITIONAL PASS — the refactoring is correct, but needs at least a smoke test to confirm the goal.
- **PSC-0014**: Must be prioritized — it is the ticket that will deliver the test infrastructure and `MockSerialPort`.
- Error contract advisory (F3, F4): Route to SW Engineer for a future enhancement ticket (consider `std::variant<size_t, ErrorCode>` or separate `last_error()` method).