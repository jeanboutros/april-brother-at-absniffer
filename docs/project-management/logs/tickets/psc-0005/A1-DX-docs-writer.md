# A1-DX: Docs Writer Review — psc-0005

| Field | Value |
|-------|-------|
| Agent | docs-writer |
| Timestamp | 2026-06-14T14:30:00Z |
| Step | A1-DX |
| Verdict | CONDITIONAL PASS |

## Findings

| ID | Confidence | Severity | File:Line | Description | Suggested Fix |
|----|-----------|----------|-----------|-------------|---------------|
| F1 | 90 | Critical | SerialPort.h:1 | Missing `@file`/`@brief`/`@example` file-level Doxygen block | Add `@file`, `@brief`, `@example` block to SerialPort.h |
| F2 | 90 | Critical | SerialPort.h:134 | `close_connection()` has no Doxygen comment — missing `@brief`, `@param`, `@return`, `@example` | Add `@brief Close the serial port and release resources.` and `@example` |
| F3 | 90 | Critical | SerialPort.h:136 | `set_baud_rate()` has no Doxygen comment — missing `@brief`, `@param`, `@return`, `@example` | Add `@brief`, `@param baud_rate`, `@return`, `@example` |
| F4 | 90 | Critical | SerialPort.h:41-59 | `baud_rate_from_num()` template function has no Doxygen comment at all | Add `@brief`, `@tparam`, `@param`, `@return`, `@example` |
| F5 | 90 | Critical | SerialPort.h:138 | `MAX_READ_BUFFER` constant has no Doxygen comment | Add `@brief Maximum read buffer size (64 KB).` |
| F6 | 85 | High | SerialPort.h:10-12 | `SerialPortException`, `SerialReadException`, `SerialWriteException` have no `@brief` doc comments | Add `@brief` doc comments for each exception class |
| F7 | 85 | High | SerialPort.h:26-39 | `BaudRate` enum has `@brief` and `@see` but no `@example` block (AGENTS.md requires `@example` on every module, struct, and public function) | Add `@example` block showing usage with `serial::baud_rate_from_num()` |
| F8 | 85 | High | SerialPort.h:67-71 | `Parity` enum has `@brief` and `@see` but no `@example` block | Add `@example` block |
| F9 | 85 | High | SerialPort.h:77-80 | `StopBits` enum has `@brief` and `@see` but no `@example` block | Add `@example` block |
| F10 | 85 | High | SerialPort.h:86-91 | `DataBits` enum has `@brief` and `@see` but no `@example` block | Add `@example` block |
| F11 | 85 | High | SerialPort.h:97-101 | `FlowControl` enum has `@brief` and `@see` but no `@example` block | Add `@example` block |
| F12 | 80 | High | bluetooth_at_driver.h:141-144 | `query_status()` missing `@example` block | Add `@example` showing `driver.query_status()` |
| F13 | 80 | High | bluetooth_at_driver.h:147-150 | `query_address()` missing `@example` block | Add `@example` showing `driver.query_address()` |
| F14 | 80 | High | bluetooth_at_driver.h:153-156 | `query_version()` missing `@example` block | Add `@example` showing `driver.query_version()` |
| F15 | 80 | High | bluetooth_at_driver.h:159-162 | `device_info()` missing `@example` block | Add `@example` showing `driver.device_info()` |
| F16 | 80 | High | bluetooth_at_driver.h:164-165 | `start_scan()` missing `@example` block | Add `@example` |
| F17 | 80 | High | bluetooth_at_driver.h:167-168 | `stop_scan()` missing `@example` block | Add `@example` |
| F18 | 80 | High | bluetooth_at_driver.h:170-174 | `set_baud_rate()` missing `@example` block | Add `@example` showing `driver.set_baud_rate(AtBaudParam::BAUD_115200)` |
| F19 | 80 | High | bluetooth_at_driver.h:176-180 | `set_scan_mode()` missing `@example` block | Add `@example` showing `driver.set_scan_mode(ScanMode::ACTIVE)` |
| F20 | 80 | High | bluetooth_at_driver.h:182-183 | `reset_device()` missing `@example` block | Add `@example` |
| F21 | 80 | High | ab_sniffer_serial_port.h:36 | `ABSnifferSerialPort` struct-level missing `@example` block (struct has `@brief` but no `@example`) | Add `@example` block to struct doc |
| F22 | 75 | Moderate | ab_sniffer_serial_port.h:59-63 | `init()`, `set_baud_rate()`, `is_open()`, `read()`, `write()`, `close_connection()` use single-line `///` comments but missing `@example` blocks | Expand to multi-line `/** */` blocks with `@example` per AGENTS.md standard |
| F23 | 75 | Moderate | types.h:80 | `sniffer_baud_rate_bps` constant has `@brief` but no `@see` reference to `serial::BaudRate` or `serial::baud_rate_from_num()` | Add `@see serial::BaudRate` and `@see serial::baud_rate_from_num()` |
| F24 | 75 | Moderate | SerialPort.h:26-39 | `BaudRate` enum has `@see` to termios but no `@see` back to `ble_sniffer::AtBaudParam` for cross-reference | Add `@see ble_sniffer::AtBaudParam` for bidirectional cross-reference |
| F25 | 75 | Moderate | bluetooth_at_driver.h:171-174 | `set_baud_rate()` doc doesn't cross-reference `serial::BaudRate` or the relationship between `AtBaudParam` and `serial::BaudRate` | Add `@see serial::BaudRate` and note about `at_baud_param_to_num()` conversion |
| F26 | 70 | Moderate | N/A | No `docs/modules/` directory exists — AGENTS.md requires module doc files for every module (14 required) | Create `docs/modules/` with required module docs (tracked separately, not PSC-0005 scope) |
| F27 | 70 | Moderate | ab_sniffer_serial_port.cpp:114 | VTIME calculation `m_impl->timeout_ms / 100` uses integer division which truncates (e.g. 150ms → 1 decisecond = 100ms, not 150ms). Not a doc issue but affects timeout accuracy documented as "100–25500 ms". | Document the truncation behavior in the constructor `@param` doc |
| F28 | 65 | Low | types.h:56 | `AT_BAUD` doc says "Append AtBaudParam enum value (0-5)" which is correct but could be clearer that the value is `static_cast<int>(enum)`, not the baud rate number | Consider clarifying: "Append the integer value of the AtBaudParam enum (0-5), not the actual baud rate" |
| F29 | 65 | Low | SerialPort.h:103-139 | `SerialPort` base struct lacks `@example` at the struct level | Add struct-level `@example` showing polymorphic usage pattern |
| F30 | 60 | Low | ab_sniffer_serial_port.h:46 | Constructor `@param baud_rate` says "Default: 115200" but the actual default is `serial::baud_rate_from_num(SNIFFER_DEFAULT_BAUD_RATE_BPS)` which resolves to `BaudRate::BAUD_115200`. The doc should reference the constant, not a raw number. | Change "Default: 115200" to "Default: `baud_rate_from_num(SNIFFER_DEFAULT_BAUD_RATE_BPS)`" |

## Blocking Findings (confidence ≥80)

- F1: SerialPort.h missing file-level Doxygen block (`@file`, `@brief`, `@example`)
- F2: `close_connection()` missing Doxygen entirely
- F3: `set_baud_rate()` missing Doxygen entirely
- F4: `baud_rate_from_num()` missing Doxygen entirely
- F5: `MAX_READ_BUFFER` missing Doxygen
- F6: Exception classes missing `@brief`
- F7–F11: Enums in SerialPort.h missing `@example` blocks
- F12–F20: Methods in `bluetooth_at_driver.h` missing `@example` blocks
- F21–F22: `ABSnifferSerialPort` methods and struct missing `@example` blocks

## Advisory Findings (confidence <80)

- F23: `sniffer_baud_rate_bps` missing `@see` cross-reference to `serial::BaudRate`
- F24: `BaudRate` enum missing `@see` cross-reference back to `AtBaudParam`
- F25: `set_baud_rate()` in `bluetooth_at_driver.h` missing `@see` to `serial::BaudRate`
- F26: Module docs don't exist yet (pre-existing, not PSC-0005 scope)
- F27: VTIME truncation not documented in `@param timeout_ms`
- F28: `AT_BAUD` doc could clarify "append integer value of enum, not baud rate"
- F29: `SerialPort` base struct missing struct-level `@example`
- F30: Constructor default parameter doc uses raw number instead of constant reference

## Self-Audit Checklist

| Category | Checked? | Finding or PASS |
|----------|----------|-----------------|
| Build passes (exit 0, no warnings) | NOT VERIFIED — requires build | N/A (not in scope for docs review) |
| Typed enums / vocabulary types (no raw integers in API) | yes | PASS — `AtBaudParam` and `BaudRate` are `enum class`; `baud_rate_from_num()` uses typed returns |
| Documentation on new public symbols | yes | FAIL — SerialPort.h missing doc on `close_connection()`, `set_baud_rate()`, `baud_rate_from_num()`, `MAX_READ_BUFFER`, exception classes; multiple methods missing `@example` blocks |
| Spec/datasheet fidelity (fields match spec) | yes | PASS — AtBaudParam values (0-5) match ABSniffer 528 wiki; AdvertisementType values (0-4) match Table 2 of wiki; serial settings match termios man page |
| Module boundary (no platform headers in shared modules) | yes | PASS — `termios.h` only in `ab_sniffer_serial_port.cpp`, NOT in any public header |
| Reserved/padding fields handled | N/A | N/A — no register structs |
| No magic numbers in doc examples | yes | PASS — examples use typed enums (`AtBaudParam::BAUD_115200`, `ScanMode::ACTIVE`), not raw integers |
| Buffer safety (bounded copies) | yes | PASS — `read()` takes `max_length`; `MAX_READ_BUFFER` limits `m_read_buffer` |
| AGENTS.md compliance | yes | FAIL — missing `@example` blocks on many public symbols; missing module doc files |
| Conventional commit ready | N/A | N/A — not in scope for docs review |

## Cross-Document Consistency Report

**DC-1 (ADR Cross-Reference):**
- ADRs found: 4 (psc-adr-0001 through psc-adr-0004)
- None are specific to PSC-0005 (POSIX leak bugfix)
- No ADR needed for this bugfix per AGENTS.md ("every resolved decision from A2 has a corresponding ADR file") — PSC-0005 is a bugfix, not an A2 design decision
- Verdict: PASS

**DC-2 (Schema Consistency):**
- No SQL schemas in this project
- Type definitions across headers are consistent: `AtBaudParam` (0-5) ↔ `serial::BaudRate` (actual bps values) ↔ `baud_rate_from_num()` conversion
- No contradictory field names, types, or constraints found
- Verdict: PASS

**DC-3 (Decision-to-Document Trace):**
- No ADRs created for PSC-0005 (bugfix ticket, not a design decision)
- Existing ADRs (psc-adr-0001 through 0004) are for different features
- Verdict: PASS (no orphaned decisions for this ticket)

**DC-4 (SQL-vs-Decision Validation):**
- No SQL files in this project
- Verdict: N/A

## Verified External References

| Reference | URL | Status |
|-----------|-----|--------|
| ABSniffer 528 AT Commands | https://wiki.aprbrother.com/en/AT_Commands_For_ABSniffer_528.html | ✅ VERIFIED — accessible, content matches code |
| termios man page | https://man7.org/linux/man-pages/man3/termios.3.html | ✅ VERIFIED — accessible, termios constants match code |

## Scope Note

Many of the findings in `SerialPort.h` and `bluetooth_at_driver.h` are **pre-existing documentation gaps** that existed before PSC-0005. PSC-0005 specifically addresses removing `termios.h` from public headers (which is now DONE — verified: `termios.h` only appears in `ab_sniffer_serial_port.cpp`, not in any `.h` file). The findings listed above represent documentation debt that should be addressed, but the PSC-0005-specific changes (PIMPL pattern in `ab_sniffer_serial_port.h`, `SerialPort.h` as interface, `sniffer_baud_rate_bps` constant) are documented correctly.

## Verdict

**CONDITIONAL PASS**

**Rationale:** The PSC-0005 core objective (removing `termios.h` from public headers) is met and well-documented. The `ab_sniffer_serial_port.h` file has proper `@file`/`@brief`/`@example` and PIMPL documentation. However, there are significant pre-existing documentation gaps in `SerialPort.h` (missing file-level doc, missing docs on `close_connection()`, `set_baud_rate()`, `baud_rate_from_num()`, `MAX_READ_BUFFER`, exception classes, and missing `@example` blocks on all enums and methods) and `bluetooth_at_driver.h` (missing `@example` blocks on 9 of 12 public methods). The cross-reference gaps (`sniffer_baud_rate_bps` → `serial::BaudRate`, `BaudRate` → `AtBaudParam`) are moderate-severity advisory findings.

**Conditions for APPROVED:**
1. Add file-level `@file`/`@brief`/`@example` block to `SerialPort.h`
2. Add `@brief` doc comments to `SerialPortException`, `SerialReadException`, `SerialWriteException` in `SerialPort.h`
3. Add full Doxygen (`@brief`, `@tparam`, `@param`, `@return`, `@example`) to `baud_rate_from_num()` in `SerialPort.h`
4. Add `@brief`, `@param`, `@return` to `close_connection()` and `set_baud_rate()` in `SerialPort.h`
5. Add `@brief` to `MAX_READ_BUFFER` in `SerialPort.h`
6. Add `@example` blocks to all public enums and methods in `SerialPort.h` (`BaudRate`, `Parity`, `StopBits`, `DataBits`, `FlowControl`, `SerialPort` struct and all methods)
7. Add `@example` blocks to the 9 public methods in `bluetooth_at_driver.h` that lack them (`query_status`, `query_address`, `query_version`, `device_info`, `start_scan`, `stop_scan`, `set_baud_rate`, `set_scan_mode`, `reset_device`)
8. Add `@example` block to `ABSnifferSerialPort` struct doc and to its override methods

**Advisory (should fix but not blocking):**
- Add bidirectional `@see` cross-references between `AtBaudParam` and `serial::BaudRate`
- Add `@see` to `sniffer_baud_rate_bps` referencing `serial::BaudRate`
- Document VTIME truncation in constructor `@param timeout_ms`
- Use constant references in doc instead of raw numbers (e.g., "Default: `baud_rate_from_num(SNIFFER_DEFAULT_BAUD_RATE_BPS)`" instead of "Default: 115200")