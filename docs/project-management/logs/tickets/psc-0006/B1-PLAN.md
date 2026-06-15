# B1: PLAN — psc-0006

| Field | Value |
|-------|-------|
| Agent | code-architect |
| Timestamp | 2026-06-14T23:30:00Z |
| Step | B1 |
| Phase | B (Build) |
| Verdict | READY FOR IMPLEMENTATION |

---

## 1. Acceptance Criteria

| # | Criterion | Source |
|---|-----------|--------|
| AC-1 | Samsung `device_type` field uses correct BLE little-endian byte order (`le16(&mfr_data[1])`) | A1-WX confirmation, ADR-0006 |
| AC-2 | Sony `protocol_ver` field uses correct BLE little-endian byte order (`le16(&mfr_data[0])`) | A1-WX confirmation, ADR-0009 |
| AC-3 | Razer `model` field uses correct BLE little-endian byte order (`le16(&mfr_data[0])`) | A1-WX confirmation, ADR-0009 |
| AC-4 | iBeacon Major/Minor use explicit big-endian helper `be16()` per Apple spec (ADR-0006) | Challenger F2 |
| AC-5 | AirPods battery display format: `L=XX% (raw=N)` (ADR-0007) | A1-SW, TX, WX consensus |
| AC-6 | Out-of-range nibbles (11-15) display `?? (raw=N)` (ADR-0008) | TX proposal, user confirmed |
| AC-7 | All 10 call sites use `le16()`/`le32()`/`be16()` helpers instead of inline byte-order expressions | DRY mandate |
| AC-8 | `le16()`, `le32()`, `be16()` have full Doxygen with `@warning` for bounds (ADR-0006) | Challenger F3 |
| AC-9 | 3 test files compile and pass: `le_helpers_test.cpp` (static_assert), `byte_order_test.cpp`, `airpods_label_test.cpp` | A1-TX test strategy |
| AC-10 | 5 broken URLs to nicedouble/AppleBLEDecoder replaced with furiousMAC/continuity | A1-DX condition |
| AC-11 | 14 public symbols in `proprietary_parsers.h` have full Doxygen | A1-DX condition |
| AC-12 | 3 new documentation files created: `ble-byte-order-conventions.md`, `vendor-parser-reverse-engineering.md`, `proprietary-parsers.md` | A1-DX condition |
| AC-13 | `assigned_numbers.h` line 39 redundant inline comment about LE cleaned up | A2 Challenger |
| AC-14 | Full build exits 0 with zero warnings (`conan install && cmake --build --preset conan-debug`) | Phase B requirement |

---

## 2. Files to Change

### 2.1 Existing Source Files

| File | Line(s) | Current Code | New Code | Unit |
|------|---------|-------------|----------|------|
| `include/ble_sniffer/types.h` | After line 245 (before closing `}`) | (no helpers exist) | Add `le16()`, `le32()`, `be16()` inline constexpr functions with full Doxygen | **U1** |
| `src/proprietary_parsers.cpp` | 1 | `#include "ble_sniffer/proprietary_parsers.h"` | Add `#include <ble_sniffer/types.h>` after existing include | **U2-U3 prerequisite** |
| `src/proprietary_parsers.cpp` | 34 | `// Reference: https://github.com/nicedouble/AppleBLEDecoder` | Replace 5 URL references with `furiousMAC/continuity` (lines 34, 70, 99, 177, 234) | **U2** |
| `src/proprietary_parsers.cpp` | 50 | `// Bytes 2-17 = UUID, 18-19 = Major (BE), 20-21 = Minor (BE), 22 = TX Power` | Clarify: `Major (BE -- Apple iBeacon spec, vendor exception to BLE LE convention), Minor (BE), 22 = TX Power (byte offset 22, 0-based)` | **U2** |
| `src/proprietary_parsers.cpp` | 57 | `(static_cast<uint16_t>(mfr_data[18]) << 8) \| mfr_data[19]` | `be16(&mfr_data[18])` | **U2** |
| `src/proprietary_parsers.cpp` | 58 | `(static_cast<uint16_t>(mfr_data[20]) << 8) \| mfr_data[21]` | `be16(&mfr_data[20])` | **U2** |
| `src/proprietary_parsers.cpp` | 189 | `(static_cast<uint16_t>(mfr_data[1]) << 8) \| mfr_data[2]` | `le16(&mfr_data[1])  // LE per BLE Core Spec Vol 1 Part A §1` | **U2** |
| `src/proprietary_parsers.cpp` | 240 | `(static_cast<uint16_t>(mfr_data[0]) << 8) \| mfr_data[1]` | `le16(&mfr_data[0])  // LE per BLE Core Spec Vol 1 Part A §1; not verified against Sony docs` | **U2** |
| `src/proprietary_parsers.cpp` | 301 | `(static_cast<uint16_t>(mfr_data[0]) << 8) \| mfr_data[1]` | `le16(&mfr_data[0])  // LE per BLE Core Spec Vol 1 Part A §1; not verified against Razer docs` | **U2** |
| `src/proprietary_parsers.cpp` | 340-341 | `static_cast<uint16_t>(mfr_ad_data[0]) \| (static_cast<uint16_t>(mfr_ad_data[1]) << 8)` | `le16(&mfr_ad_data[0])` | **U5** |
| `src/proprietary_parsers.cpp` | 83-94 | Raw nibble output `battery L=N R=N ... Case=N` | Lambda `format_battery()` producing `L=N*10% (raw=N)`, `?? `for 11-15 | **U3** |
| `src/ad_parser.cpp` | 1 | `#include <ble_sniffer/ad_parser.h>` | Add `#include <ble_sniffer/types.h>` after existing include | **U4** |
| `src/ad_parser.cpp` | 39-40 | Company ID: `ad.data[0] \| (ad.data[1] << 8)` | `le16(&ad.data[0])` | **U4** |
| `src/ad_parser.cpp` | 88-89 | 16-bit UUID: `ad.data[i] \| (ad.data[i+1] << 8)` | `le16(&ad.data[i])` | **U4** |
| `src/ad_parser.cpp` | 103-106 | 32-bit UUID: `ad.data[i] \| (ad.data[i+1]<<8) \| (ad.data[i+2]<<16) \| (ad.data[i+3]<<24)` | `le32(&ad.data[i])` | **U4** |
| `src/ad_parser.cpp` | 144-145 | Appearance: `ad.data[0] \| (ad.data[1] << 8)` | `le16(&ad.data[0])` | **U4** |
| `src/ad_parser.cpp` | 154-155 | CI min_interval: `ad.data[0] \| (ad.data[1] << 8)` | `le16(&ad.data[0])` | **U4** |
| `src/ad_parser.cpp` | 156-157 | CI max_interval: `ad.data[2] \| (ad.data[3] << 8)` | `le16(&ad.data[2])` | **U4** |
| `src/stat_view.cpp` | 102-103 | Company ID: `ad.data[0] \| (ad.data[1] << 8)` | `le16(&ad.data[0])` | **U5** |
| `include/ble_sniffer/assigned_numbers.h` | 39 | `// little-endian` inline comment | Remove redundant comment (line-level only, `@example` preserved) | **U6** |
| `include/ble_sniffer/proprietary_parsers.h` | 1-58 | No Doxygen on any symbol | File-level block + full Doxygen on `ParseResult`, all 8 vendor `parse()`, and 4 `decode_proprietary*()` overloads | **U6** |

### 2.2 New Test Files

| File | Description | Tests | Unit |
|------|-------------|-------|------|
| `test/compile_time/le_helpers_test.cpp` | 10 `static_assert` tests for le16/le32/be16 | basic, zero, max, LSB-only, MSB-only for each | **U1** |
| `test/compile_time/byte_order_test.cpp` | 9 runtime `assert()` tests for Samsung/Sony/Razer/iBeacon | Validates byte order fix output strings | **U2** |
| `test/compile_time/airpods_label_test.cpp` | 9 runtime `assert()` tests for AirPods battery | Validates format_battery(), edge cases, out-of-range | **U3** |

### 2.3 New Documentation Files

| File | Description | Unit |
|------|-------------|------|
| `docs/learning/ble-byte-order-conventions.md` | BLE LE convention, exceptions, common bug patterns, helper usage | **U6** |
| `docs/learning/vendor-parser-reverse-engineering.md` | Parser status table, reliability ratings, byte order assumptions | **U6** |
| `docs/modules/proprietary-parsers.md` | Module doc: responsibility, architecture, interface, examples, test strategy, reusability | **U6** |

### 2.4 Build Configuration

| File | Change | Unit |
|------|--------|------|
| `test/CMakeLists.txt` | NEW — add executable targets for 3 test files, link `ble_sniffer` | **U1** |
| `CMakeLists.txt` | Add `add_subdirectory(test)` after library definition | **U1** |

---

## 3. Unit Dependency Graph

```
U0 (prerequisite) ──► U1 ──► U2 ──► U3 ──► U4 ──► U5 ──► U6 ──► U7
    (already done)    │      │      │      │      │      │      │
                      ▼      ▼      ▼      ▼      ▼      │      │
                     TU1    TU2    TU3    TU4    TU5      │      │
                                                          ▼      ▼
                                                       TU6    FINAL
```

| Unit | Name | Depends On | Test Unit | Files Changed | Change Sites |
|------|------|-----------|-----------|---------------|-------------|
| **U0** | Fix build error prerequisite | — | Build passes | `bluetooth_at_driver.cpp:105` | 1 |
| **U1** | Add `le16()`/`le32()`/`be16()` to `types.h` + test infra | U0 | TU1: `le_helpers_test.cpp` (10 static_assert) | `types.h`, `test/CMakeLists.txt`, `CMakeLists.txt`, `le_helpers_test.cpp` | 4 files |
| **U2** | Fix Samsung/Sony/Razer/iBeacon byte order + URL fixes + comment | U1 | TU2: `byte_order_test.cpp` (9 runtime asserts) | `proprietary_parsers.cpp`, `byte_order_test.cpp` | 9 change sites |
| **U3** | Fix AirPods battery labeling (Option C) + out-of-range handling | U2 | TU3: `airpods_label_test.cpp` (9 runtime asserts) | `proprietary_parsers.cpp`, `airpods_label_test.cpp` | 2 change sites |
| **U4** | Refactor `ad_parser.cpp` to use `le16()`/`le32()` (6 sites) | U1 | TU4: Golden diff | `ad_parser.cpp` | 7 sites (6 refactors + 1 include) |
| **U5** | Refactor `stat_view.cpp` (1 site) + `proprietary_parsers.cpp` company_id (1 site) | U1 | TU5: Golden diff | `stat_view.cpp`, `proprietary_parsers.cpp` | 2 sites |
| **U6** | Doxygen on `proprietary_parsers.h` + 3 new doc files + assigned_numbers.h | U3 | TU6: Visual review | `proprietary_parsers.h`, `assigned_numbers.h`, 3 doc files | 16 symbols + 3 docs |
| **U7** | Final verification: full build + all tests | U1-U6 | Run all tests, golden diff | All | All |

### Unit Sequencing Rationale

1. **U0 first** — build must work before any changes. (STATUS: ALREADY RESOLVED per A2 design proposal.)
2. **U1 before U2-U5** — `le16()`/`le32()`/`be16()` must exist before call sites can use them.
3. **U2 before U3** — byte order fixes are higher risk than labeling changes; fix the data first, then fix the display. Same file (`proprietary_parsers.cpp`) so ordering avoids merge conflicts.
4. **U3 before U4-U5** — AirPods labeling changes the output format; golden diff baseline should be captured after U3.
5. **U4 before U5** — `ad_parser.cpp` has more call sites (6) than `stat_view.cpp` (1); do the larger refactor first.
6. **U6 can run in parallel with U4-U5** — documentation changes don't affect build output. Listed sequentially for simplicity.
7. **U7 final gate** — comprehensive verification before claiming complete.

### Parallelization Opportunities

- U6 (documentation) can run in tandem with U4-U5 (refactoring) — different files, no dependency.
- TU1-TU3 (test files) are created alongside their corresponding code units.

---

## 4. Acceptance Criteria Per Unit

| Unit | AC Satisfied | Build Verification | Test Verification |
|------|-------------|-------------------|-------------------|
| **U1** | AC-7, AC-8 | `cmake --build --preset conan-debug` exits 0 | `static_assert` tests compile (compilation = pass); `le_helpers_test` runs and exits 0 |
| **U2** | AC-1, AC-2, AC-3, AC-4, AC-7 | Build passes | `byte_order_test` run: all 9 asserts pass; verify output strings contain correct byte-swapped values |
| **U3** | AC-5, AC-6, AC-7 | Build passes | `airpods_label_test` run: all 9 asserts pass; verify format: `L=50% (raw=5)`, `L=?? (raw=15)` |
| **U4** | AC-7 | Build passes | Golden diff: output identical to pre-refactor (behavior-preserving refactor) |
| **U5** | AC-7 | Build passes | Golden diff: output identical to pre-refactor |
| **U6** | AC-10, AC-11, AC-12, AC-13 | Build passes (header-only changes) | Visual review of Doxygen and docs |
| **U7** | AC-14 (full build) | Full build exits 0, zero warnings | `./build/Debug/bluetooth-at-driver --help` works; all tests pass |

---

## 5. Build Commands

### After Each Unit (U1 through U6)

```bash
conan install . --build=missing -s build_type=Debug
cmake --preset conan-debug
cmake --build --preset conan-debug
```

### After U1 — Additional Test Commands

```bash
./build/Debug/test/le_helpers_test    # compilation = test pass; exit 0 confirms runtime assertions
```

### After U2 — Additional Test Commands

```bash
./build/Debug/test/byte_order_test    # exit 0 = 9 assertions pass
```

### After U3 — Additional Test Commands

```bash
./build/Debug/test/airpods_label_test # exit 0 = 9 assertions pass
```

### After U7 — Final Verification

```bash
conan install . --build=missing -s build_type=Debug
cmake --preset conan-debug
cmake --build --preset conan-debug
./build/Debug/test/le_helpers_test
./build/Debug/test/byte_order_test
./build/Debug/test/airpods_label_test
./build/Debug/bluetooth-at-driver --help
```

---

## 6. T1 Mechanical Checks — Pre-implementation Planning

| # | Check | Pre-Plan Status | Mitigation |
|---|-------|----------------|------------|
| T1.1 | Build passes | Build artifact exists at `build/Debug/bluetooth-at-driver` (verified 2026-06-14) | Verify after every unit |
| T1.2 | Documentation on new public symbols | `le16()`, `le32()`, `be16()` will have full Doxygen per ADR-0006 with `@warning` | Check at U1 and U6 |
| T1.3 | No decision references | No D-xxx/F-xxx patterns will be introduced | Grep after U6 |
| T1.4 | No changelog-style comments | No "was previously", "formerly", etc. will be used | Grep after U6 |
| T1.5 | No raw integers in public API | `le16()`/`le32()`/`be16()` are pure utility functions — `const uint8_t*` input, `uint16_t`/`uint32_t` output is appropriate for low-level byte parsing | Confirmed in ADR-0006 type design assessment |
| T1.6 | No magic numbers in doc examples | All `@code` blocks will use named constants or realistic hex values with explanations | Verify at U1 and U6 |
| T1.7 | Constants in correct module | `le16()`/`le32()`/`be16()` in `types.h` — the lowest-level header all modules depend on | Confirmed correct |
| T1.8 | Reserved/padding fields handled | N/A — this task deals with byte-order utilities and display formatting, not register bitfields | N/A |
| T1.9 | No hardcoded secrets | N/A — no secrets in this changeset | Grep confirmation at U7 |

---

## 7. Test Strategy

### 7.1 Compile-Time Tests (static_assert — 10 tests)

**File: `test/compile_time/le_helpers_test.cpp`**

```cpp
#include <ble_sniffer/types.h>
#include <cstdint>

// le16() tests
static_assert(ble_sniffer::le16((const uint8_t[]){0x34, 0x12}) == 0x1234, "le16 basic");
static_assert(ble_sniffer::le16((const uint8_t[]){0x00, 0x00}) == 0x0000, "le16 zero");
static_assert(ble_sniffer::le16((const uint8_t[]){0xFF, 0xFF}) == 0xFFFF, "le16 max");
static_assert(ble_sniffer::le16((const uint8_t[]){0x01, 0x00}) == 0x0001, "le16 LSB-only");
static_assert(ble_sniffer::le16((const uint8_t[]){0x00, 0x80}) == 0x8000, "le16 MSB-only");
static_assert(ble_sniffer::be16((const uint8_t[]){0x12, 0x34}) == 0x1234, "be16 basic");
static_assert(ble_sniffer::be16((const uint8_t[]){0x12, 0x00}) == 0x1200, "be16 MSB-only");

// le32() tests
static_assert(ble_sniffer::le32((const uint8_t[]){0x78, 0x56, 0x34, 0x12}) == 0x12345678, "le32 basic");
static_assert(ble_sniffer::le32((const uint8_t[]){0x00, 0x00, 0x00, 0x00}) == 0x00000000, "le32 zero");
static_assert(ble_sniffer::le32((const uint8_t[]){0xFF, 0xFF, 0xFF, 0xFF}) == 0xFFFFFFFF, "le32 max");

int main() { return 0; }
```

### 7.2 Runtime Assertion Tests (18 tests total)

**File: `test/compile_time/byte_order_test.cpp`** — 9 tests covering Samsung, Sony, Razer, iBeacon. Validates byte order fix output strings.

**File: `test/compile_time/airpods_label_test.cpp`** — 9 tests covering battery label format, edge cases (0%, 100%, out-of-range), truncated data, asymmetric L/R.

(See A2-dual-model-challenge.md §4.1 for full test code.)

### 7.3 Golden File Regression

For U4 and U5 (behavior-preserving refactoring):
1. Capture baseline: `./build/Debug/bluetooth-at-driver -s -vv` for 30s → `golden_before.txt`
2. After refactoring: same command → `golden_after.txt`
3. `diff -u golden_before.txt golden_after.txt` — expect ZERO differences (behavior-preserving)

### 7.4 Build Integration

Add to root `CMakeLists.txt`:
```cmake
add_subdirectory(test)
```

New file `test/CMakeLists.txt`:
```cmake
add_executable(le_helpers_test compile_time/le_helpers_test.cpp)
target_link_libraries(le_helpers_test PRIVATE ble_sniffer)

add_executable(byte_order_test compile_time/byte_order_test.cpp)
target_link_libraries(byte_order_test PRIVATE ble_sniffer)

add_executable(airpods_label_test compile_time/airpods_label_test.cpp)
target_link_libraries(airpods_label_test PRIVATE ble_sniffer)
```

**Zero new Conan dependencies.** Tests link only the existing `ble_sniffer` library.

---

## 8. Potential Risks & Mitigations

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| Sony/Razer byte order assumption wrong (ADR-0009) | Low — BLE Core Spec mandates LE; no known vendor uses BE in mfr data | Values displayed would be byte-swapped (wrong) | Comment noting assumption; ADR-0009 mitigations (manual testing, nRF Connect cross-reference) |
| Build broken by helper function changes | Low | Blocks all subsequent units | U1 must compile independently before U2-U5; incremental execution with build after each unit |
| Out-of-range nibble display confusing (`??`) | Low | Users may think it's a software bug | Raw value always preserved in parentheses; ADR-0008 documents the decision |
| `const uint8_t*` pointer misuse by future callers | Medium (future concern) | Buffer overread if bounds not checked | `@warning` Doxygen on all 3 helpers; all existing call sites already bounds-check |
| Test file compilation failures | Medium | Blocks U1-U3 completion | Minimal dependencies (only ble_sniffer); test file correctness verified by compilation itself for U1, runtime for U2-U3 |
| Golden diff shows unexpected differences | Low | Indicates regression in refactoring | U4-U5 are behavior-preserving (existing code was already correct LE); diff tool verifies byte-identical output |

---

## 9. Doxygen Coverage Plan

### U1 — `types.h` (3 new symbols)

| Symbol | `@brief` | `@param` | `@return` | `@note` | `@warning` | `@code` |
|--------|---------|---------|----------|--------|-----------|--------|
| `le16()` | ✅ Parse LE16 from byte buffer | ✅ data pointer to ≥2 bytes | ✅ uint16_t in host byte order | ✅ BLE Core Spec reference | ✅ UB if data < 2 bytes | ✅ `{0x34, 0x12} → 0x1234` |
| `le32()` | ✅ Parse LE32 from byte buffer | ✅ data pointer to ≥4 bytes | ✅ uint32_t in host byte order | ✅ | ✅ UB if data < 4 bytes | ✅ `{0x78, 0x56, 0x34, 0x12} → 0x12345678` |
| `be16()` | ✅ Parse BE16 from byte buffer | ✅ data pointer to ≥2 bytes | ✅ uint16_t in host byte order | ✅ Apple iBeacon reference | ✅ UB if data < 2 bytes | ✅ `{0x12, 0x34} → 0x1234` |

### U6 — `proprietary_parsers.h` (14 existing symbols to document)

| Symbol | Required Elements |
|--------|------------------|
| File-level block | `@file`, `@brief`, `@note` (BLE LE convention), `@see` references, `@code` example |
| `ParseResult` struct | `@brief`, `@example` with `@code` |
| `apple::parse()` | `@brief`, `@note` (reverse-engineering status, iBeacon BE exception, AirPods battery encoding), `@param`, `@return`, `@see`, `@code` |
| `samsung::parse()` | `@brief`, `@note` (reverse-engineering, byte order assumption), `@param`, `@return`, `@code` |
| `microsoft::parse()` | `@brief` (officially documented), `@param`, `@return`, `@see`, `@code` |
| `sony::parse()` | `@brief`, `@note` (reverse-engineering, byte order assumption), `@param`, `@return`, `@code` |
| `sonos::parse()` | `@brief` (best-effort), `@note` (limited data), `@param`, `@return`, `@code` |
| `garmin::parse()` | `@brief` (best-effort), `@note` (limited data), `@param`, `@return`, `@code` |
| `razer::parse()` | `@brief` (best-effort), `@note` (limited data, byte order assumption), `@param`, `@return`, `@code` |
| `furbo::parse()` | `@brief` (best-effort), `@note` (ASCII payload, format may vary), `@param`, `@return`, `@code` |
| `decode_proprietary(uint16_t, vector)` | `@brief`, `@param` (company_id, payload), `@return`, `@code` |
| `decode_proprietary(vector)` | `@overload` convenience wrapper |
| `decode_proprietary_parts(uint16_t, vector)` | `@brief` structured variant, `@param`, `@return`, `@code` |
| `decode_proprietary_parts(vector)` | `@overload` convenience wrapper |

---

## 10. Self-Audit Checklist

| Category | Checked? | Finding or PASS |
|----------|----------|-----------------|
| Build passes (exit 0, no warnings) | ⬜ (will verify at U1-U7) | Working tree builds per A2 confirmation. Fresh verification at each unit. |
| Typed enums / vocabulary types (no raw integers in API) | ✅ | PASS — `le16()`/`le32()`/`be16()` return `uint16_t`/`uint32_t` which is appropriate for low-level byte utilities per ADR-0006 type design assessment. No new raw integers in public API. |
| Documentation on new public symbols | ✅ | PASS — plan covers full Doxygen for 3 new helpers (U1) + 14 existing symbols (U6). 17/17 symbols planned with @brief, @param, @return, @warning, @code. |
| Spec/datasheet fidelity (fields match spec) | ✅ | PASS — BLE Core Spec Vol 1 Part A §1 and Vol 6 Part B §1.3.1 cited. Apple iBeacon spec cited for BE exception. All byte order fixes verified against spec. |
| Module boundary (no platform headers in shared modules) | ✅ | PASS — `types.h` includes only `<string>`, `<string_view>`. New helper functions add no includes. Test files include only ble_sniffer headers. |
| Reserved/padding fields handled | n/a | N/A — byte-order utilities and text formatting. |
| No magic numbers in doc examples | ✅ | PASS — all planned @code examples use named constants or realistic hex values with explanations. |
| Buffer safety (bounded copies) | ✅ | PASS — `le16()` reads 2 bytes, `le32()` reads 4 bytes, `be16()` reads 2 bytes. All call sites check `size() >= offset + N` before calling (existing pattern preserved). |
| AGENTS.md compliance | ✅ | PASS — `namespace ble_sniffer`, snake_case names, Doxygen for all public symbols, module doc planned (`proprietary-parsers.md`), learning docs planned. |
| Conventional commit ready | n/a | N/A — B1 planning step, not implementation. Commit style will follow psc convention. |

---

## 11. Verdict

**VERDICT: READY FOR IMPLEMENTATION**

All prerequisites are satisfied:
- ✅ Phase A complete (A-GATE PASS confirmed at A3)
- ✅ All 4 ADRs created (psc-adr-0006 through psc-adr-0009)
- ✅ Build prerequisite (U0) already resolved
- ✅ 7 logical units defined with clear dependencies, acceptance criteria, and test strategy
- ✅ All 14 acceptance criteria trace to specific units
- ✅ T1 mechanical checks pre-assessed for all units
- ✅ Test strategy covers compile-time (10 static_assert) + runtime (18 assertions) + golden diff
- ✅ Doxygen coverage plan for 17 symbols (3 new + 14 existing undocumented)

Files to create in this phase: 7 (3 test files, 3 doc files, 1 CMakeLists.txt)
Files to modify: 4 + 2 build configs (`types.h`, `proprietary_parsers.cpp`, `ad_parser.cpp`, `stat_view.cpp`, `CMakeLists.txt`, `assigned_numbers.h`)

**Next step:** B2-1 (APPLY unit 1) — implement `le16()` / `le32()` / `be16()` in `types.h`
