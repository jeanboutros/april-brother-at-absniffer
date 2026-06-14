# A1-SW: Software Engineer Architecture Review

| Field | Value |
|-------|-------|
| Agent | software-engineer |
| Timestamp | 2026-06-14T14:30:00Z |
| Step | A1-SW |
| Verdict | CONDITIONAL PASS |

## Summary

PSC-0005 removes `termios.h` from the public header `bluetooth_at_driver.h` by introducing:
1. A `SerialPort` abstract interface in `serial` namespace
2. An `ABSnifferSerialPort` concrete implementation using PIMPL to hide termios
3. An `AtBaudParam` enum class replacing the old `BaudRate` that conflated AT protocol codes with actual baud rates
4. A `sniffer_baud_rate_bps` constexpr constant replacing the old `speed_t sniffer_baud_rate`
5. Dependency injection of `SerialPort` into `BluetoothATDriver`

The primary goal (remove termios from public headers) is **fully achieved**. The PIMPL pattern correctly isolates all POSIX-specific code. The namespace boundary between `ble_sniffer` (protocol) and `serial` (transport) is clean. However, there are several findings that need attention before this can be fully approved.

## Detailed Findings

### F1: `close_connection()` returns void — errors silently swallowed [Confidence: 85, Dimension: Invariant Expression]

**File:** `include/ble_sniffer/SerialPort.h:134`

```cpp
virtual void close_connection() = 0;
```

The `close_connection()` method returns `void`. In the `ABSnifferSerialPort` implementation (`ab_sniffer_serial_port.cpp:140-151`), errors during `tcsetattr` (restoring original terminal settings) are silently ignored. The comment says "Best-effort cleanup" which is acceptable for destruction paths, but the interface itself provides no way for callers to know whether close succeeded.

The `init()` method returns `bool` — `close_connection()` should follow the same pattern for consistency and to allow callers to decide what to do on failure.

**Severity:** Medium. This affects the interface design and could lead to silent resource leaks on platforms where terminal settings restoration matters (e.g., leaving a TTY in raw mode after the program exits).

**Suggested Fix:** Change `close_connection()` to return `bool` indicating success/failure, mirroring `init()`. The destructor path can ignore the return value (already the case since the destructor calls `close_connection()` without checking).

### F2: `set_baud_rate()` is virtual but `close_connection()` has no error reporting — inconsistent interface design [Confidence: 80, Dimension: Usefulness]

**File:** `include/ble_sniffer/SerialPort.h:136`

```cpp
virtual bool set_baud_rate(BaudRate baud_rate) = 0;
```

`init()` returns `bool`. `set_baud_rate()` returns `bool`. But `close_connection()` returns `void`. The interface is inconsistent in how it reports errors. All mutating operations on a serial port should have a consistent error-reporting strategy.

**Severity:** Low-Medium. Inconsistency in interface design, but not a bug.

**Suggested Fix:** Make `close_connection()` return `bool` for consistency with `init()` and `set_baud_rate()`.

### F3: PIMPL `Impl` struct stores `std::optional<struct termios>` — termios type in private impl is acceptable [Confidence: 95, Dimension: Encapsulation]

**File:** `src/ab_sniffer_serial_port.cpp:37-57`

```cpp
struct serial::ABSnifferSerialPort::Impl {
    std::string device;
    std::optional<struct termios> original_tty;
    int file_descriptor = -1;
    const uint16_t timeout_ms;
    BaudRate current_baud_rate;
    // ...
};
```

The `Impl` struct is defined in the `.cpp` file, not the header. This means `termios` is only visible in the implementation file. The header `ab_sniffer_serial_port.h` only declares `struct Impl;` and uses `std::unique_ptr<Impl>`. This is the correct PIMPL pattern.

**Verdict:** ✅ Correct. No termios types leak through the header.

### F4: `BluetoothATDriver` destructor calls methods on potentially moved-from `m_serial_port` [Confidence: 75, Dimension: Encapsulation]

**File:** `src/bluetooth_at_driver.cpp:15-21`

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

The move constructor and move assignment are deleted (`BluetoothATDriver(BluetoothATDriver&&) = delete`), which prevents the most common case of use-after-move. However, the destructor still dereferences `m_serial_port` unconditionally. If `m_serial_port` were somehow null (e.g., if constructor threw after `std::move`), the `is_open()` call would crash.

Looking at the constructor:
```cpp
BluetoothATDriver::BluetoothATDriver(std::unique_ptr<serial::SerialPort> serial_port) 
    : m_serial_port(std::move(serial_port)) {}
```

Since the move is in the initializer list, if `serial_port` was a null unique_ptr, `m_serial_port` would be null and `m_serial_port->is_open()` would be UB. The constructor should validate the pointer, or the destructor should check for null.

**Severity:** Medium. Defensive programming issue. Not currently exploitable since the class deletes move, but it's a latent bug.

**Suggested Fix:** Add a null check in the destructor or an assertion in the constructor. Consider:
```cpp
BluetoothATDriver::BluetoothATDriver(std::unique_ptr<serial::SerialPort> serial_port) 
    : m_serial_port(std::move(serial_port)) {
    if (!m_serial_port) {
        throw std::invalid_argument("serial_port must not be null");
    }
}
```

### F5: Namespace boundary between `ble_sniffer` and `serial` is clean [Confidence: 95, Dimension: Usefulness]

The `SerialPort` interface and its supporting types (`BaudRate`, `Parity`, `StopBits`, `DataBits`, `FlowControl`, exception types) live in `namespace serial`. The protocol-level types (`AtBaudParam`, `ScanMode`, `AdvertisementType`, `MessageType`, etc.) live in `namespace ble_sniffer`. This is a correct separation of concerns:

- `serial` — generic serial I/O abstraction (reusable for any serial device)
- `ble_sniffer` — ABSniffer 528 protocol specifics

The `BluetoothATDriver` depends on `serial::SerialPort` (interface) not on `serial::ABSnifferSerialPort` (concrete). This is correct dependency inversion.

**Verdict:** ✅ Clean namespace boundary, correct dependency direction.

### F6: `AtBaudParam` vs `serial::BaudRate` naming distinction is clear [Confidence: 90, Dimension: Invariant Expression]

The rename from `BaudRate` to `AtBaudParam` is an improvement. The old name was misleading — `BaudRate::BAUD_115200` had value `4` (the AT command parameter code), not `115200` (the actual baud rate). The new naming clearly distinguishes:

- `ble_sniffer::AtBaudParam` — AT protocol parameter codes (0-5), lives in the protocol namespace
- `serial::BaudRate` — actual serial port baud rate values (300-921600), lives in the transport namespace

The conversion functions `at_baud_param_from_num()` and `at_baud_param_to_num()` bridge the two domains correctly.

**Verdict:** ✅ Well-designed type separation.

### F7: `baud_rate_from_num()` and `at_baud_param_from_num()` are template functions in headers with `throw` in constexpr context [Confidence: 70, Dimension: Enforcement]

**File:** `include/ble_sniffer/SerialPort.h:41-58`
**File:** `include/ble_sniffer/types.h:118-130`

Both `serial::baud_rate_from_num()` and `ble_sniffer::at_baud_param_from_num()` are `static constexpr` template functions that `throw std::invalid_argument` in their `default:` case. In C++17, `constexpr` functions can contain `throw` statements, but the `throw` makes them non-constexpr at runtime when the exception path is taken. This is technically valid but misleading — the `constexpr` hint suggests compile-time evaluation, but the default path prevents that.

More importantly, these are `static` free functions in headers. In C++17, `static` on a free function in a header means "internal linkage" — each translation unit gets its own copy. This is fine for small functions but the `inline` keyword would be more conventional and wouldn't give each TU its own copy. The `static constexpr` combination is unusual; `inline constexpr` or just `constexpr` (which implies `inline` for functions) would be more idiomatic.

**Severity:** Low. Not a bug, but a style issue that could confuse maintainers.

**Suggested Fix:** Change `static constexpr` to `constexpr` for both functions. In C++17, a `constexpr` function at namespace scope is implicitly `inline`.

### F8: `sniffer_baud_rate_bps` uses `int` instead of a typed constant [Confidence: 75, Dimension: Invariant Expression]

**File:** `include/ble_sniffer/types.h:80`

```cpp
inline constexpr int sniffer_baud_rate_bps = 115200;
```

Per the type-design-review skill, raw integer constants in public APIs should use typed vocabulary. This is a `constexpr int` representing a baud rate in bps — it's not a raw `uint8_t` parameter (which would be worse), but it could be more expressive. Since this is a default value for configuring a serial port, it could be a `serial::BaudRate` constant. However, since `types.h` is in `ble_sniffer` namespace and should not depend on `SerialPort.h`, an `int` is a reasonable compromise.

**Severity:** Low. The naming `sniffer_baud_rate_bps` makes the unit explicit (bps), and the value `115200` is self-documenting for this domain.

**Verdict:** ✅ Acceptable. The unit is explicit in the name, and cross-namespace dependency would be worse.

### F9: `ABSnifferSerialPort` constructor has `uint16_t` timeout range documented but not enforced as `static_assert` [Confidence: 65, Dimension: Invariant Expression]

**File:** `include/ble_sniffer/ab_sniffer_serial_port.h:46`

```cpp
/// @param timeout_ms Read timeout in milliseconds (100–25500). Default: 1000.
```

**File:** `src/ab_sniffer_serial_port.cpp:65-66`

```cpp
if (m_impl->timeout_ms < 100 || m_impl->timeout_ms > 25500) {
    throw std::invalid_argument("Timeout must be between 100ms and 25500ms");
}
```

The range is validated at runtime but could be documented more strongly. The old `bluetooth_at_driver.h` had `static_assert(sniffer_timeout >= 100, ...)` and `static_assert(sniffer_timeout / 100 <= 255, ...)` — compile-time enforcement of the constraint. The new code validates only at runtime. Since the default value (1000) is within range, this is fine, but for custom values the old static_assert was more defensive.

**Severity:** Low. Runtime validation is adequate for a constructor parameter.

### F10: SerialPort interface methods lack error documentation contracts [Confidence: 70, Dimension: Usefulness]

**File:** `include/ble_sniffer/SerialPort.h:103-139`

The `SerialPort` interface documents the happy path but doesn't specify error contracts clearly:

- `read()` says "return 0 on error/timeout" but implementations throw `SerialReadException`. Which is it?
- `write()` says "return 0 on error" but implementations throw `SerialWriteException`.
- The exception hierarchy (`SerialPortException` → `SerialReadException`, `SerialWriteException`) is defined in the interface header, which is good, but the methods' Doxygen doesn't mention they can throw.

**Severity:** Medium. The interface contract is ambiguous. A mock implementation (for testing) wouldn't know whether to return 0 or throw.

**Suggested Fix:** Update Doxygen on `read()` and `write()` to explicitly state that they throw `SerialReadException`/`SerialWriteException` on errors, and return 0 only for timeout/no-data conditions.

### F11: SOLID Analysis

| Principle | Assessment | Notes |
|-----------|-----------|-------|
| **S — Single Responsibility** | ✅ PASS | `SerialPort` handles transport. `BluetoothATDriver` handles AT protocol. `ABSnifferSerialPort` handles POSIX serial I/O. Clean separation. |
| **O — Open/Closed** | ✅ PASS | New serial port implementations (e.g., `MockSerialPort`, `TCPSerialPort`) can be added without modifying `BluetoothATDriver` or `SerialPort`. |
| **L — Liskov Substitution** | ⚠️ CONDITIONAL | `close_connection()` void return makes it hard for callers to know if close failed. But any `SerialPort` impl can substitute. Pass with caveat from F1. |
| **I — Interface Segregation** | ✅ PASS | `SerialPort` has 6 methods: `init`, `is_open`, `read`, `write`, `close_connection`, `set_baud_rate`. Each is necessary for serial I/O. No fat interface. |
| **D — Dependency Inversion** | ✅ PASS | `BluetoothATDriver` depends on `serial::SerialPort` (abstract interface), not `ABSnifferSerialPort` (concrete). Constructor takes `unique_ptr<SerialPort>` — injectable. |

### F12: DRY Analysis

| Concern | Assessment |
|---------|------------|
| Baud rate mapping | `at_baud_param_to_num()` in `ble_sniffer` namespace and `baud_rate_from_num()` in `serial` namespace both map numeric baud rates to enums. These serve different domains (AT protocol vs. serial port config) so the duplication is intentional and correct. |
| `set_baud_rate()` in `BluetoothATDriver` | Correctly converts `AtBaudParam` → actual baud rate → `serial::BaudRate` → calls `m_serial_port->set_baud_rate()`. No duplication. |
| PIMPL pattern | The `Impl` struct in `.cpp` correctly eliminates duplication — termios config code is in one place. |

### F13: Clean Architecture Analysis

| Layer | Module | Depends on | Assessment |
|-------|--------|-----------|------------|
| Entry Point | `main.cpp` | `BluetoothATDriver`, `ABSnifferSerialPort` | ✅ Correct — wiring layer |
| Adapter | `ABSnifferSerialPort` | `SerialPort` (interface), termios | ✅ Correct — adapter implements interface |
| Protocol | `BluetoothATDriver` | `SerialPort` (interface), `messages`, `types` | ✅ Correct — depends on abstraction |
| Interface | `SerialPort` | Nothing (pure abstract) | ✅ Correct — innermost layer |
| Types | `types.h` | Standard library only | ✅ Correct — no device dependency |

The dependency arrows point inward. `BluetoothATDriver` never includes `<termios.h>` or any POSIX header. ✅

### F14: `ABSnifferSerialPort` default baud rate references `ble_sniffer` namespace [Confidence: 70, Dimension: Clean Architecture]

**File:** `include/ble_sniffer/ab_sniffer_serial_port.h:40`

```cpp
static constexpr int SNIFFER_DEFAULT_BAUD_RATE_BPS = ble_sniffer::sniffer_baud_rate_bps;
```

This creates a cross-namespace dependency: `serial::ABSnifferSerialPort` references `ble_sniffer::sniffer_baud_rate_bps`. While not a hard dependency (it's just a `constexpr int` value), it's a semantic coupling. The `serial` namespace should be generic/reusable, but this constant ties it to the ABSniffer domain.

**Severity:** Low. The value (115200) is a common serial default, and the reference is to a `constexpr int` (compile-time constant). But if `serial::ABSnifferSerialPort` is meant to be reusable for other serial devices, this coupling is undesirable.

**Suggested Fix:** Consider either:
1. Moving the default to the `serial` namespace as `serial::DEFAULT_BAUD_RATE_BPS` and having `ble_sniffer::sniffer_baud_rate_bps` reference it (dependency arrow reversal), or
2. Hardcoding `115200` as the default and documenting that it matches the ABSniffer factory default.

Option 2 is simpler and avoids the cross-namespace dependency entirely.

## Type Design Review

### `AtBaudParam` (types.h:97-104)

| Dimension | Score | Justification |
|-----------|-------|---------------|
| Encapsulation | 8/10 | `enum class` with explicit underlying type `int`. Values are finite (0-5). No invalid values can be constructed. Slight deduction: values don't directly map to human-readable baud rates (value 4 = 115200 requires lookup). |
| Invariant Expression | 8/10 | `enum class` prevents invalid values at compile time. `at_baud_param_from_num()` validates input at runtime and throws on invalid values. Well-designed. |
| Usefulness | 9/10 | `AtBaudParam::BAUD_115200` is self-documenting. The name makes clear it's an AT command parameter, not an actual baud rate. Conversion functions are clearly named. |
| Enforcement | 7/10 | The public API of `set_baud_rate(AtBaudParam)` uses the typed enum. However, `static_cast<int>(baud_rate)` in `bluetooth_at_driver.cpp:112` reveals the underlying int. This is internal code, so acceptable. The `at_baud_param_from_num` template is `static` (internal linkage) which limits testability from other TUs. |

**Overall:** 8.0/10 — Meets the minimum A-GATE threshold of 7.0.

### `serial::BaudRate` (SerialPort.h:26-39)

| Dimension | Score | Justification |
|-----------|-------|---------------|
| Encapsulation | 7/10 | `enum class` with explicit underlying type `int`. Clean. But values are raw integers (300, 115200, etc.) which happen to be the actual baud rates — not protocol codes. This is correct for a transport-layer enum. |
| Invariant Expression | 7/10 | `enum class` prevents arbitrary values. `baud_rate_from_num()` validates runtime input. But the `default:` case throws rather than providing a `std::optional` or error type. |
| Usefulness | 8/10 | `BaudRate::BAUD_115200` is immediately clear. The `baud_rate_from_num()` helper is well-named. |
| Enforcement | 7/10 | Public API uses the typed enum. `baud_rate_from_num()` is `static constexpr` (internal linkage) which limits testability. |

**Overall:** 7.25/10 — Meets minimum threshold.

### `ScanMode` (types.h:166-169)

| Dimension | Score | Justification |
|-----------|-------|---------------|
| Encapsulation | 9/10 | Simple two-value `enum class`. Impossible to create invalid values. |
| Invariant Expression | 9/10 | Only PASSIVE=0 and ACTIVE=1. Compile-time safe. |
| Usefulness | 9/10 | `ScanMode::ACTIVE` is self-documenting. |
| Enforcement | 9/10 | Public API uses the typed enum exclusively. |

**Overall:** 9.0/10 — Excellent.

### `AdvertisementType` (types.h:185-191)

| Dimension | Score | Justification |
|-----------|-------|---------------|
| Encapsulation | 9/10 | Five finite values, `enum class` with underlying `int`. |
| Invariant Expression | 8/10 | Compile-time safe. `static_cast<int>` used internally in message parsing, which is acceptable. |
| Usefulness | 10/10 | Names are directly from BLE spec. `advertisement_type_to_string()` provides human-readable output. |
| Enforcement | 8/10 | Used via typed enum in `ScanResultMessage`. Internal parsing uses `static_cast`. |

**Overall:** 8.75/10 — Excellent.

## PIMPL Correctness Verification

| Check | Result |
|-------|--------|
| `termios.h` not included in any public header | ✅ PASS — only in `ab_sniffer_serial_port.cpp` |
| No `speed_t`, `cc_t`, `tcgetattr`, `tcsetattr` in headers | ✅ PASS — only in `.cpp` |
| `struct Impl` forward-declared in header | ✅ PASS — `ab_sniffer_serial_port.h:79` |
| `Impl` defined in `.cpp` | ✅ PASS — `ab_sniffer_serial_port.cpp:37-57` |
| `std::unique_ptr<Impl>` used correctly | ✅ PASS — move-only, destructor in `.cpp` |
| No POSIX types in `Impl` forward declaration | ✅ PASS — `struct Impl;` only |
| `#include <termios.h>` only in `.cpp` | ✅ PASS — `ab_sniffer_serial_port.cpp:11` |
| `bluetooth_at_driver.h` has no termios/POSIX includes | ✅ PASS — only includes `<memory>`, `<string>`, project headers |

## Self-Audit Checklist

| # | Check | Result |
|---|-------|--------|
| 1 | Did I read ALL specified files? | ✅ Yes — all 6 files reviewed |
| 2 | Did I verify the primary goal (remove termios from public headers)? | ✅ Yes — `#include <termios.h>` absent from all `.h` files |
| 3 | Did I check namespace hygiene? | ✅ Yes — `ble_sniffer` and `serial` are clean |
| 4 | Did I check SOLID compliance? | ✅ Yes — all 5 principles assessed |
| 5 | Did I check type design for all enums? | ✅ Yes — 4 enums scored |
| 6 | Did I check for raw integers in public API? | ✅ Yes — `sniffer_baud_rate_bps` is `constexpr int` (F8) |
| 7 | Did I verify build compiles? | ✅ Yes — cmake build succeeds |
| 8 | Did I identify any missing test coverage concerns? | ✅ Yes — `SerialPort` interface enables mocking, but no mock exists yet |
| 9 | Are my findings backed by file:line evidence? | ✅ Yes — all findings reference specific lines |
| 10 | Did I check dependency direction? | ✅ Yes — arrows point inward per Clean Architecture |

## Verdict

**CONDITIONAL PASS**

The primary goal of PSC-0005 (remove `termios.h` from public header) is fully achieved. The PIMPL pattern correctly hides all POSIX details. The `SerialPort` interface enables testability. The `AtBaudParam` rename correctly separates AT protocol codes from serial port baud rates. The namespace boundary is clean.

**Conditions for full approval:**

1. **[F1/F2] `close_connection()` return type:** Change from `void` to `bool` for consistency with `init()` and `set_baud_rate()`. This is an interface design fix that should be done before the interface is consumed by other code. **Must fix.**

2. **[F4] Null pointer safety in destructor:** Add either a null check in the `BluetoothATDriver` destructor or a validation/assertion in the constructor that `serial_port` is not null. **Must fix.**

3. **[F10] Exception contract documentation:** Update `read()` and `write()` Doxygen in `SerialPort.h` to document that they throw `SerialReadException`/`SerialWriteException`. The interface contract is currently ambiguous. **Should fix.**

4. **[F7] `static constexpr` → `constexpr`:** Change `static constexpr` functions to `constexpr` (which implies `inline` in C++17) in both `SerialPort.h` and `types.h`. Minor style issue. **Nice to have.**

5. **[F14] Cross-namespace dependency:** Consider decoupling `serial::ABSnifferSerialPort` from `ble_sniffer::sniffer_baud_rate_bps`. **Nice to have.**

Findings F3, F5, F6, F8, F9, F11, F12, F13 are all PASS or ACCEPTABLE. No REJECTED items.
