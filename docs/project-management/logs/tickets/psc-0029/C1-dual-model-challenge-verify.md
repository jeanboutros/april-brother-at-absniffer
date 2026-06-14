# C1: Dual-Model Challenge — Verification

| Field | Value |
|-------|-------|
| Agent | software-engineer |
| Timestamp | 2026-06-14T22:30:00Z |
| Step | C1 |
| Verdict | APPROVED |

## Purpose

Stress-test the IMPLEMENTED changes (not proposed) against the ADRs. The primary position is that the implementation correctly addresses all findings. The challenger position looks for any issues.

## ADR Cross-Reference

| ADR | Decision | Implementation Status |
|-----|----------|----------------------|
| psc-adr-0029-1 | Keep `bool init()`, add `[[nodiscard]]`, document contract | ✅ `[[nodiscard]]` added at `SerialPort.h:197`. Doxygen updated. |
| psc-adr-0029-2 | Add precondition check in `BluetoothATDriver` constructor: throw `std::invalid_argument` if `!port->is_open()` | ✅ Implemented at `bluetooth_at_driver.cpp:18-20`. Exception type matches. Message matches ADR intent. |
| psc-adr-0029-3 | Caller is responsible for calling `init()`. Driver validates precondition. Follows Dependency Inversion. | ✅ `main.cpp:110-122` shows caller calling `init()` before constructing driver. Defensive comments document this pattern. |

## Primary Position: All Findings Addressed ✅

| Finding | ADR Reference | Implementation Evidence | Verified? |
|---------|---------------|------------------------|-----------|
| F1: Doxygen said "Opens" instead of "Takes ownership" | — (pre-existing fix) | `bluetooth_at_driver.h:50`: "Takes ownership of an already-initialized serial port on construction and closes it on destruction (RAII)." | ✅ |
| F4: No constructor precondition for is_open() | psc-adr-0029-2 | `bluetooth_at_driver.cpp:18-20`: throws `std::invalid_argument` if `!is_open()` | ✅ |
| [[nodiscard]] on init() | psc-adr-0029-1 | `SerialPort.h:197`: `[[nodiscard]] virtual bool init() = 0;` | ✅ |
| Defensive comment about init() pattern | psc-adr-0029-3 | `main.cpp:108-109` and `main.cpp:112-114` | ✅ |
| init() called explicitly in main.cpp | psc-adr-0029-3 | `main.cpp:115`: `if (!port->init())` | ✅ |
| Action flag validation in main.cpp | — (pre-existing fix) | `main.cpp:102-106` | ✅ |

## Challenger Position: Issue Analysis

### C1.1: Is the is_open() check in the right position?

**Analysis:** The check sequence in `bluetooth_at_driver.cpp:14-21`:
1. Line 14: Member initializer — `m_serial_port(std::move(serial_port))` — necessary to access the port
2. Line 15-17: Null check — `if (!m_serial_port)` → throw
3. Line 18-20: Open check — `if (!m_serial_port->is_open())` → throw

The `is_open()` check is immediately after the null check. No other operations (no reads, writes, or method calls on the port) occur between these two checks.

**Verdict:** ✅ CORRECT — The ordering is correct. Null check must come first to prevent UB from dereferencing null.

### C1.2: Does the error message match ADR psc-adr-0029-2?

**ADR states:** "throw std::invalid_argument" with precondition "is_open() returns true"
**Implementation:** `throw std::invalid_argument("serial_port must be open before constructing BluetoothATDriver (call init() first)")`

**Assessment:**
- Exception type: `std::invalid_argument` ✅ matches ADR
- Condition: `!is_open()` ✅ matches ADR
- Message: Provides both the precondition ("must be open") and the fix ("call init() first") ✅ exceeds ADR minimum
- Message references "call init() first" ✅ actionable guidance for the caller

**Verdict:** ✅ MATCHES — Implementation matches and exceeds ADR requirements with actionable error message.

### C1.3: Does [[nodiscard]] compile correctly on all target platforms?

**Analysis:** `[[nodiscard]]` is a standard C++17 attribute (introduced in C++17, §10.6.5). Per AGENTS.md, this project targets C++17. All major compilers (GCC ≥ 7, Clang ≥ 4, MSVC ≥ 2017 15.3) support `[[nodiscard]]` in their C++17 modes.

**Evidence:** Build succeeds on current platform (macOS/Clang). The CMake configuration specifies C++17 (`CMAKE_CXX_STANDARD`).

**Verdict:** ✅ NO ISSUE — `[[nodiscard]]` is standard C++17 and compiles correctly.

### C1.4: Is the defensive comment accurate?

**Comment at main.cpp:112-114:**
```cpp
// Defensive check: ABSnifferSerialPort::init() always throws on failure (never returns false),
// but the SerialPort interface contract permits implementations that return false.
// This check handles alternative SerialPort implementations correctly.
```

**Verification:** Reading `ab_sniffer_serial_port.cpp:75-122`:
- Line 80: throws `SerialPortException` if `open()` fails
- Line 85: throws `SerialPortException` if `tcgetattr` fails
- Line 117: throws `SerialPortException` if `tcsetattr` fails
- Line 122: only return statement is `return true`

**Conclusion:** `ABSnifferSerialPort::init()` indeed never returns `false` — it always throws on failure. The comment is accurate.

**Verdict:** ✅ ACCURATE — Comment correctly describes the implementation behavior and the interface contract.

### C1.5: Edge Cases

| Edge Case | Analysis | Risk |
|-----------|----------|------|
| Null port passed to constructor | Caught by null check (line 15-17). Safe. | None |
| Port where is_open() returns false | Caught by is_open() check (line 18-20). Throws clear exception. | None |
| Port where init() throws | Caught by try/catch in main.cpp:119-122. Error printed, exit code 1. | None |
| No action flags specified | Caught by validation (main.cpp:102-106). Error printed, exit code 1. | None |
| Already-open port passed to constructor | is_open() returns true, construction proceeds normally. | None |
| Double-init on same port | Pre-existing issue, not introduced by psc-0029. The old file descriptor would leak. Out of scope. | Advisory (confidence: 65) |
| Destructor when port was never opened | Destructor checks `is_open()` before cleanup (line 24). Safe. | None |

### C1.6: Header Doxygen matches implementation contract?

**Header (bluetooth_at_driver.h:50-51):**
```
Takes ownership of an already-initialized serial port on construction and closes it on destruction (RAII).
The port must be open (is_open() returns true) before construction; otherwise std::invalid_argument is thrown.
```

**Implementation (bluetooth_at_driver.cpp:14-21):**
- Takes ownership: `m_serial_port(std::move(serial_port))` ✅
- Requires is_open(): `if (!m_serial_port->is_open()) throw std::invalid_argument(...)` ✅
- RAII: destructor closes port ✅

**Verdict:** ✅ HEADER MATCHES IMPLEMENTATION

## Self-Audit Checklist

| Category | Checked? | Finding or PASS |
|----------|----------|-----------------|
| Build passes (exit 0, no warnings) | yes | PASS — Build completed with no errors or warnings |
| Typed enums / vocabulary types (no raw integers in API) | yes | PASS — All public API uses typed enums (AtBaudParam, ScanMode, BaudRate, etc.) |
| Documentation on new public symbols | yes | PASS — `[[nodiscard]]` on init() has Doxygen. Constructor precondition documented in header. |
| Spec/datasheet fidelity | yes | N/A — No datasheet register changes in this ticket |
| Module boundary (no platform headers in shared modules) | yes | PASS — No platform headers in shared modules |
| Reserved/padding fields handled | yes | N/A — No serialisation changes |
| No magic numbers in doc examples | yes | PASS — Doc examples use typed constants |
| Buffer safety (bounded copies) | yes | PASS — No buffer changes in this ticket |
| AGENTS.md compliance | yes | PASS — All naming, documentation, and architecture rules followed |
| Conventional commit ready | yes | PASS — Changes are cohesive and ready for commit |

## Review Findings

**Reviewer:** software-engineer
**Phase:** C
**Artifact:** bluetooth_at_driver.h, bluetooth_at_driver.cpp, SerialPort.h, main.cpp
**Date:** 2026-06-14

### Findings

| ID | Confidence | Severity | File:Line | Description | Suggested Fix |
|----|-----------|----------|-----------|-------------|---------------|
| F1 | 95 | Critical | bluetooth_at_driver.cpp:18-20 | Constructor is_open() precondition correctly implemented ✅ (verified) | No fix needed |
| F2 | 90 | Critical | SerialPort.h:197 | [[nodiscard]] attribute correctly added to init() ✅ (verified) | No fix needed |
| F3 | 90 | Critical | main.cpp:102-106 | Action flag validation correctly implemented ✅ (verified) | No fix needed |
| F4 | 90 | Critical | main.cpp:108-122 | Defensive init() pattern correctly implemented ✅ (verified) | No fix needed |
| F5 | 65 | Low | ab_sniffer_serial_port.cpp:75 | Double-init edge case (calling init() twice leaks first fd) — pre-existing, out of scope for psc-0029 | Track as future ticket if needed |

### Blocking Findings (confidence ≥80)

None. All findings F1-F4 are verified as correctly implemented. F5 is advisory (confidence 65, out of scope).

### Advisory Findings (confidence <80)

- F5: Double-init on ABSnifferSerialPort could leak a file descriptor. This is a pre-existing issue not introduced by psc-0029. Track separately if needed.

## Verdict

**APPROVED**

All four psc-0029 changes are correctly implemented, verified against the ADRs, and tested against edge cases. The build passes. No blocking findings.
