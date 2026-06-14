# B2a-1: B-UNIT-GATE Unit 1

| Field | Value |
|-------|-------|
| Agent | software-engineer |
| Timestamp | 2026-06-14T13:00:00Z |
| Step | B2a-1 |
| Verdict | PASS |

## Scope

Unit 1 of psc-0029: F1 (Doxygen fix in `bluetooth_at_driver.h`) + F4 (constructor precondition in `bluetooth_at_driver.cpp`).

---

## T1 (Mechanical) Checks

| # | Check | Result | Evidence |
|---|-------|--------|----------|
| T1.1 | F1 fix: line 50 now says "Takes ownership of an already-initialized serial port" instead of "Opens the serial port on construction" | ✅ PASS | `bluetooth_at_driver.h:50` reads "Takes ownership of an already-initialized serial port on construction and closes it on destruction (RAII)." Line 51 adds precondition: "The port must be open (is_open() returns true) before construction; otherwise std::invalid_argument is thrown." Both the ownership description and the precondition are documented. |
| T1.2 | F1 fix: mentions the `is_open()` precondition | ✅ PASS | Line 51 explicitly states "The port must be open (is_open() returns true) before construction; otherwise std::invalid_argument is thrown." |
| T1.3 | F4 fix: constructor checks `!m_serial_port->is_open()` after the null check | ✅ PASS | `bluetooth_at_driver.cpp:18-20` adds `if (!m_serial_port->is_open()) { throw std::invalid_argument("serial_port must be open before constructing BluetoothATDriver (call init() first)"); }` immediately after the null check at lines 15-17. |
| T1.4 | F4 fix: throws `std::invalid_argument` with clear message mentioning `init()` | ✅ PASS | The message reads "serial_port must be open before constructing BluetoothATDriver (call init() first)" — explicitly mentions `init()`. |
| T1.5 | Build verification | ✅ PASS | `cmake --build --preset conan-debug` exits 0 with no warnings. All targets (`libble_sniffer.a`, `bluetooth-at-driver`) rebuilt successfully. |
| T1.6 | No unintended changes to target files | ✅ PASS | `git diff HEAD` shows only 3 added lines in `bluetooth_at_driver.cpp` (the is_open check) and 1 replaced line + 1 added line in `bluetooth_at_driver.h` (Doxygen update). No other modifications to these two files. |
| T1.7 | No decision references in source | ✅ PASS | No `D-`, `F-`, or `(decision` patterns in changed lines. |
| T1.8 | No changelog-style comments | ✅ PASS | No "replaces the", "was previously", "formerly", "old", "refactored from" patterns in changed lines. |
| T1.9 | No hardcoded secrets | ✅ PASS | No password, api_key, secret, token, credential, or Bearer patterns in changed lines. |

---

## T-ARCH (Architecture + Principles) Checks

| # | Check | Result | Evidence |
|---|-------|--------|----------|
| T-ARCH.1 | Constructor precondition aligns with ADR psc-adr-0029-2 | ✅ PASS | ADR psc-adr-0029-2 states: "Add a precondition check in BluetoothATDriver constructor: throw `std::invalid_argument` if the port's `is_open()` returns false." The implementation at `bluetooth_at_driver.cpp:18-20` does exactly this. |
| T-ARCH.2 | Doxygen change aligns with ADR psc-adr-0029-3 | ✅ PASS | ADR psc-adr-0029-3 states: "The caller is responsible for calling `init()`. The driver validates the precondition (`is_open() == true`) in its constructor." The Doxygen at lines 50-51 documents both: (a) the driver takes ownership of an already-initialized port (caller calls init), and (b) the constructor throws if is_open() returns false. |
| T-ARCH.3 | Error message consistent with existing null check pattern | ✅ PASS | The null check at line 15-16 throws `std::invalid_argument("serial_port must not be null")`. The is_open check at line 18-20 throws `std::invalid_argument("serial_port must be open before constructing BluetoothATDriver (call init() first)")`. Both use `std::invalid_argument` for precondition violations. Same exception type, same descriptive pattern. |
| T-ARCH.4 | Constructor follows RAII (takes ownership, validates, no resource leak on throw) | ✅ PASS | The constructor: (1) moves the unique_ptr into m_serial_port (takes ownership), (2) validates null, (3) validates is_open(). If the is_open check throws, the unique_ptr has already been moved to m_serial_port, so the destructor will be called when the exception unwinds — the moved-from unique_ptr in the caller is empty. No resource leak. The port's close_connection() will be called by the destructor if the port was open (which it is, since is_open() returned true — but wait, we threw because is_open() returned false, so the port was never usable, and the destructor checks is_open() before closing). |
| T-ARCH.5 | Logical consistency: throw on !is_open() means the port is NOT open, so destructor is safe | ✅ PASS | When the constructor throws `std::invalid_argument` because `!is_open()` is true, the `BluetoothATDriver` object is not fully constructed, so its destructor is NOT called (C++ rule: destructors are only called for fully-constructed objects). The `m_serial_port` member's destructor WILL be called because it was already initialized by the move. The SerialPort object will be destroyed, and since it was never open, its destructor should handle that cleanly. |

**Note on T-ARCH.4 RAII analysis:** There is a subtle point worth documenting. When `!m_serial_port->is_open()` throws, `m_serial_port` has already been initialized (it holds the moved unique_ptr). The member destructors run during stack unwinding, so `m_serial_port` (unique_ptr) is destroyed, which destroys the SerialPort. The SerialPort was never opened, so its destructor should handle this gracefully. The `BluetoothATDriver` destructor does NOT run because the object was never fully constructed. This is correct C++ RAII behavior — no resource leak.

---

## Self-Audit Checklist

| Category | Checked? | Finding or PASS |
|----------|----------|-----------------|
| Build passes (exit 0, no warnings) | yes | PASS — `cmake --build --preset conan-debug` exits 0, 0 warnings |
| Typed enums / vocabulary types (no raw integers in API) | yes | PASS — No new public API symbols. The change adds a precondition check using existing typed interfaces. |
| Documentation on new public symbols | yes | PASS — The constructor's Doxygen was updated (line 50-51). The constructor's explicit Doxygen at lines 79-88 already exists and is unchanged. |
| Spec/datasheet fidelity | yes | N/A — No hardware register changes. |
| Module boundary (no platform headers in shared modules) | yes | PASS — No new includes added. |
| Reserved/padding fields handled | yes | N/A — No serialization changes. |
| No magic numbers in doc examples | yes | PASS — No magic numbers in examples. |
| Buffer safety (bounded copies) | yes | N/A — No buffer operations changed. |
| AGENTS.md compliance | yes | PASS — `enum class` not applicable (no new enums). Namespace correct (`ble_sniffer`). Naming follows conventions. |
| Conventional commit ready | yes | PASS — Changes are atomic and focused on psc-0029 bug fix. |

---

## Additional Observation (Non-Blocking)

The B1-PLAN specified slightly different wording for the F1 Doxygen fix:

**PLAN specified:**
```
 * Takes ownership of an already-initialized serial port. The port must be
 * open (is_open() returns true) before construction; otherwise
 * std::invalid_argument is thrown. The port is closed on destruction (RAII).
```

**Actual implementation:**
```
 * Takes ownership of an already-initialized serial port on construction and closes it on destruction (RAII).
 * The port must be open (is_open() returns true) before construction; otherwise std::invalid_argument is thrown.
```

The actual wording is semantically equivalent but more concise — it combines ownership/RAII on one line and the precondition on the next. This is an acceptable deviation that preserves all required information: (a) takes ownership, (b) already-initialized, (c) is_open() precondition, (d) std::invalid_argument thrown, (e) RAII semantics. No information is lost.

Confidence: 65 (Low) — this is an advisory observation, not a blocking finding. The actual text is arguably better (more concise while preserving all semantics).

---

## Review Findings

**Reviewer:** software-engineer
**Phase:** B (B-UNIT-GATE, Unit 1)
**Artifact:** `include/ble_sniffer/bluetooth_at_driver.h` (lines 50-51), `src/bluetooth_at_driver.cpp` (lines 18-20)
**Date:** 2026-06-14

### Findings

| ID | Confidence | Severity | File:Line | Description | Suggested Fix |
|----|-----------|----------|-----------|-------------|---------------|
| F1 | 95 | Critical | bluetooth_at_driver.cpp:18-20 | ✅ VERIFIED: Constructor now checks `!m_serial_port->is_open()` and throws `std::invalid_argument` per ADR psc-adr-0029-2 | N/A |
| F2 | 95 | Critical | bluetooth_at_driver.h:50-51 | ✅ VERIFIED: Doxygen now says "Takes ownership of an already-initialized serial port" and documents the `is_open()` precondition per ADR psc-adr-0029-3 | N/A |
| F3 | 90 | High | bluetooth_at_driver.cpp:18-20 | ✅ VERIFIED: Error message "call init() first" explicitly mentions `init()`, providing actionable guidance | N/A |
| F4 | 65 | Low | bluetooth_at_driver.h:50-51 | Minor wording difference from PLAN (combined ownership/RAII into one line vs. two) — semantically equivalent, arguably more concise | No change needed |

### Blocking Findings (confidence ≥80)

None. All critical/high findings are verified as correct.

### Advisory Findings (confidence <80)

- F4: Minor wording variation from PLAN — acceptable, no action needed.

---

## Overall Verdict: PASS

All T1 mechanical checks pass. All T-ARCH architecture checks pass. The F1 Doxygen fix and F4 constructor precondition are correctly implemented per the ADRs. The build succeeds. No unintended changes to the target files. The main.cpp changes (lines 102-119) are pre-existing and documented in the B1-PLAN as "already applied" — they are not part of unit 1 scope.
