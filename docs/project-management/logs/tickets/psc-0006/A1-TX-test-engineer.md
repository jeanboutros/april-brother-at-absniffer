# A1-TX: Test Engineer Review — psc-0006

| Field | Value |
|-------|-------|
| Agent | test-engineer |
| Timestamp | 2026-06-14T20:30:00Z |
| Step | A1-TX |
| Verdict | CONDITIONAL PASS |
| Coverage | 0/4 acceptance criteria have executable test evidence (no test infrastructure exists) |

---

## 1. Existing Test Coverage Analysis

### 1.1 Test Infrastructure — NONE EXISTS

| Check | Result |
|-------|--------|
| `test/` directory | **Not found** — no test directory anywhere in the project |
| Test framework dependency | **None** — `conanfile.py` only has `cli11/2.4.2`; no Catch2, GoogleTest, or doctest |
| Test CMakeLists | **None** — `CMakeLists.txt` has no `add_subdirectory(test)` or `enable_testing()` |
| `MockSerialPort` | **Not implemented** — PSC-0005 identified this gap and deferred to PSC-0014 |
| Any test file | **Zero** — `glob("test/**/*")` and `glob("**/*test*")` return nothing |
| Build verification | **Pre-existing build error** — `bluetooth_at_driver.cpp:105` has `m_serial_port.` (incomplete member access). This is unrelated to psc-0006 but blocks any build-based verification. |

### 1.2 What PSC-0005 Already Identified

The previous ticket (psc-0005, "Remove termios from public headers") identified the same test infrastructure gap and deferred it to PSC-0014. The A1-TX review for psc-0005 listed 18 test cases (T1-T18) that require Catch2 infrastructure. **PSC-0014 has not been completed.** This means:

- No `MockSerialPort` exists
- No Catch2 is available
- No test compilation target exists
- **All testing for psc-0006 must be done via manual CLI verification or compile-time `static_assert`**

### 1.3 What CAN Be Tested Without Infrastructure

Despite the lack of test infrastructure, the following test types are possible:

| Test Type | Feasible? | How |
|-----------|-----------|-----|
| `static_assert` compile-time tests | **Yes** | Add `static_assert` in a `.cpp` file that verifies `le16()`/`le32()` correctness at compile time |
| Manual CLI testing | **Yes** | Run `bluetooth-at-driver -s -vv` with real Samsung/AirPods devices |
| Golden file regression | **Yes (manual)** | Capture known-good output, diff against new output |
| Unit tests with Catch2 | **No** | Requires PSC-0014 |
| Mock-based driver tests | **No** | Requires PSC-0014 + `MockSerialPort` |
| Fuzz testing | **No** | Requires libFuzzer + Clang (not in current build config) |

---

## 2. Test Strategy for the Fixes

### 2.1 Unit 1: `le16()` and `le32()` Helpers (types.h)

**Risk:** If these helpers are wrong, every call site that uses them will produce wrong values. This is the highest-leverage change — it must be tested first and most thoroughly.

#### Test Cases

| # | Test | Type | Input | Expected Output |
|---|------|------|-------|-----------------|
| U1-T1 | `le16()` basic | `static_assert` | `{0x34, 0x12}` | `0x1234` |
| U1-T2 | `le16()` zero | `static_assert` | `{0x00, 0x00}` | `0x0000` |
| U1-T3 | `le16()` max | `static_assert` | `{0xFF, 0xFF}` | `0xFFFF` |
| U1-T4 | `le16()` LSB-only | `static_assert` | `{0x01, 0x00}` | `0x0001` |
| U1-T5 | `le16()` MSB-only | `static_assert` | `{0x00, 0x80}` | `0x8000` |
| U1-T6 | `le32()` basic | `static_assert` | `{0x78, 0x56, 0x34, 0x12}` | `0x12345678` |
| U1-T7 | `le32()` zero | `static_assert` | `{0x00, 0x00, 0x00, 0x00}` | `0x00000000` |
| U1-T8 | `le32()` max | `static_assert` | `{0xFF, 0xFF, 0xFF, 0xFF}` | `0xFFFFFFFF` |
| U1-T9 | `le32()` LSB-only | `static_assert` | `{0x42, 0x00, 0x00, 0x00}` | `0x00000042` |
| U1-T10 | `le32()` MSB-only | `static_assert` | `{0x00, 0x00, 0x00, 0x80}` | `0x80000000` |

**Implementation approach:** Add a `test/compile_time/le_helpers_test.cpp` file that contains these `static_assert`s. Include it in the build (add to `ble_sniffer` library sources temporarily, or create a separate compile-only target). The file compiles → all asserts pass → helpers are correct.

**Why `static_assert` and not runtime:** These are pure constexpr functions operating on known compile-time arrays. `static_assert` catches errors at compile time with zero runtime overhead and zero test framework dependency.

### 2.2 Unit 2: Samsung/Sony/Razer Byte Order Fixes (proprietary_parsers.cpp)

**Risk:** Byte-swapping a field changes the displayed value. If the fix is wrong (vendor actually uses big-endian), the output becomes incorrect. Need both synthetic and real-device verification.

#### Test Cases — Synthetic (static_assert / compile-time)

| # | Test | Input (mfr_data bytes) | Expected `device_type` (Samsung) | Expected `protocol_ver` (Sony) | Expected `model` (Razer) |
|---|------|------------------------|----------------------------------|-------------------------------|---------------------------|
| U2-T1 | Samsung LE basic | `{0x42, 0x04, 0x00}` | `0x0004` | — | — |
| U2-T2 | Samsung LE max | `{0x42, 0xFF, 0xFF}` | `0xFFFF` | — | — |
| U2-T3 | Samsung LE LSB-only | `{0x42, 0x01, 0x00}` | `0x0001` | — | — |
| U2-T4 | Samsung LE MSB-only | `{0x42, 0x00, 0x80}` | `0x8000` | — | — |
| U2-T5 | Samsung truncated | `{0x42, 0x04}` | `0x0400` (only 1 byte available — should still parse as LE: `mfr_data[1] \| 0`) | — | — |
| U2-T6 | Sony LE basic | `{0x01, 0x00}` | — | `0x0001` | — |
| U2-T7 | Sony LE max | `{0xFF, 0xFF}` | — | `0xFFFF` | — |
| U2-T8 | Razer LE basic | `{0x34, 0x12}` | — | — | `0x1234` |
| U2-T9 | Razer LE max | `{0xFF, 0xFF}` | — | — | `0xFFFF` |

**Note on U2-T5:** The current code at line 188-191 checks `mfr_data.size() >= 3` before parsing `device_type`. If only 2 bytes are available (type byte + 1 data byte), the `device_type` line is skipped entirely. The fix should handle this consistently — either skip (current behavior) or parse the single available byte as the LSB with MSB=0. I recommend keeping the current skip behavior (requires ≥3 bytes) for safety.

#### Test Cases — Real Device Verification (manual CLI)

| # | Test | Device | How to Verify |
|---|------|--------|---------------|
| U2-R1 | Samsung SmartTag | Samsung Galaxy SmartTag (or any SmartThings device) | Run `bluetooth-at-driver -s -vv`, capture output, verify `device_type` matches known value from reference implementation (e.g., nRF Connect app) |
| U2-R2 | Sony device | Sony BLE device (headphones, speaker, TV remote) | Same — cross-reference with nRF Connect or LightBlue |
| U2-R3 | Razer device | Razer BLE peripheral (mouse, keyboard) | Same — cross-reference |

**Cross-validation method:** Use nRF Connect for Mobile (Android/iOS) or LightBlue to scan the same device. Both apps display raw manufacturer data bytes. Compare the hex bytes with the parsed `device_type`/`protocol_ver`/`model` output from `bluetooth-at-driver`.

### 2.3 Unit 3: AirPods Battery Labeling (proprietary_parsers.cpp)

**Risk:** The mapping from raw nibble (0-10) to percentage (0%-100%) is reverse-engineered. If Apple changes the encoding or uses a different scale for certain models, the percentage will be wrong. The raw value must always be visible for debugging.

#### Test Cases — Synthetic

| # | Test | Input (battery_byte, case_byte) | Expected Output |
|---|------|----------------------------------|-----------------|
| U3-T1 | Both full | `battery_byte=0xAA` (L=10, R=10), `case_byte=0x0A` (case=10) | `battery L=100% (raw=10) R=100% (raw=10) Case=100% (raw=10)` |
| U3-T2 | Both empty | `battery_byte=0x00` (L=0, R=0), `case_byte=0x00` (case=0) | `battery L=0% (raw=0) R=0% (raw=0) Case=0% (raw=0)` |
| U3-T3 | Asymmetric | `battery_byte=0x73` (L=7, R=3) | `battery L=70% (raw=7) R=30% (raw=3)` |
| U3-T4 | Mid-range | `battery_byte=0x55` (L=5, R=5), `case_byte=0x05` (case=5) | `battery L=50% (raw=5) R=50% (raw=5) Case=50% (raw=5)` |
| U3-T5 | Out-of-range nibble (>10) | `battery_byte=0xFB` (L=15, R=11) | `battery L=?? (raw=15) R=?? (raw=11)` — must handle gracefully, not assert/UB |
| U3-T6 | Charging flag set | `case_byte=0x7A` (charging=0x7, case=10, lid=open) | `Case=100% (raw=10) charging=0x7 lid=open` |
| U3-T7 | Lid closed | `case_byte=0x05` (charging=0, case=5, lid=closed) | `Case=50% (raw=5) charging=0x0 lid=closed` |
| U3-T8 | Truncated data (<7 bytes) | `mfr_data` = `{0x07, 0x04, 0x00, 0x02}` (4 bytes) | `len=4 model=0x02` — no battery output (current behavior, correct) |
| U3-T9 | Exactly 7 bytes | `mfr_data` = `{0x07, 0x06, 0x00, 0x02, 0x00, 0x00, 0x55}` | Battery L/R shown, no case data (current behavior, correct) |

**Implementation note for U3-T5:** The raw nibble can be 0-15 (4 bits). Apple's protocol uses 0-10 for battery level. Values 11-15 are either reserved or indicate special states (e.g., charging, error). The code must handle these gracefully — I recommend displaying `??` for the percentage and always showing the raw value. Do NOT clamp to 10 silently — that hides real data.

#### Test Cases — Real Device Verification

| # | Test | Device | How to Verify |
|---|------|--------|---------------|
| U3-R1 | AirPods (any gen) | AirPods with case open | Run `bluetooth-at-driver -s -vv`, verify battery percentages match iOS Bluetooth battery widget |
| U3-R2 | AirPods in case | AirPods in closed case | Verify case battery percentage matches iOS |
| U3-R3 | AirPods charging | AirPods in charging case | Verify charging flag is set, percentages are reasonable |
| U3-R4 | Single AirPod | One AirPod out, one in case | Verify asymmetric L/R values |

### 2.4 Unit 4: Refactor ad_parser.cpp to Use `le16()`/`le32()`

**Risk:** Refactoring existing correct code to use new helpers. If the helpers are wrong (caught by U1 tests), all refactored sites become wrong. If the refactoring introduces a typo (wrong offset, wrong helper), the output changes.

#### Test Strategy: Golden File Regression

Since these are refactoring changes (behavior-preserving), the best test is a **golden file regression test**:

1. **Before refactoring:** Run `bluetooth-at-driver -s -vv` for 30 seconds in a BLE-rich environment. Capture stdout to `golden_before.txt`.
2. **After refactoring:** Run the same command. Capture stdout to `golden_after.txt`.
3. **Diff:** `diff golden_before.txt golden_after.txt` — should be **identical** (modulo timestamps/RSSI variations).

**What to diff:** The `--stat` view and `-vv` scan output. Timestamps and RSSI values will vary between runs, so the diff should focus on:
- Manufacturer names (should be identical)
- AD type decoding (should be identical)
- UUID formatting (should be identical)
- Appearance values (should be identical)
- Connection interval values (should be identical)

**If golden diff is impractical** (live BLE environment is non-deterministic), use **synthetic test vectors**:

| # | Test | AD Data Input | Expected Output (pre and post refactor must match) |
|---|------|---------------|-----------------------------------------------------|
| U4-T1 | Company ID LE16 | `{0xFF, 0x4C, 0x00, ...}` (Apple) | Manufacturer: "Apple, Inc." |
| U4-T2 | 16-bit UUID LE16 | `{0x03, 0x0D, 0x18, 0x0A, 0x18}` | "0x180D, 0x180A" |
| U4-T3 | 32-bit UUID LE32 | `{0x05, 0x01, 0x02, 0x03, 0x04}` | "0x04030201" |
| U4-T4 | Appearance LE16 | `{0x19, 0x40, 0x02}` | "0x0240 (category=9)" |
| U4-T5 | Conn Interval LE16×2 | `{0x12, 0x50, 0x00, 0xA0, 0x00}` | "100.00ms - 200.00ms" |

These can be tested as `static_assert` or runtime tests by calling `decode_ad_data()` directly with synthetic `AdStructure` objects.

### 2.5 Unit 5: Refactor stat_view.cpp to Use `le16()`

**Risk:** Same as U4 — behavior-preserving refactoring.

**Test:** The `stat_view.cpp` company_id parse at line 102-103 is used in the `--stat` live view. Verify with golden diff or manual observation that manufacturer names are unchanged after refactoring.

### 2.6 Unit 6: Comment Fixes

**Risk:** Zero — comment-only changes. No tests needed beyond visual review.

---

## 3. Edge Cases Requiring Specific Tests

### 3.1 Byte Order Edge Cases

| Edge Case | Why It Matters | Test |
|-----------|---------------|------|
| Single-byte field (no endianness) | Ensure `le16()` is not accidentally applied to 1-byte fields | Code review — all call sites pass 2+ bytes |
| Odd-length data | `le16()` called on last byte of odd-length buffer | Callers must bounds-check before calling `le16()`. This is a **caller responsibility**, not a `le16()` responsibility. |
| Empty vector | `mfr_data.empty()` → `std::nullopt` | Already handled by all parsers (first line checks `empty()`) |
| Truncated data (<2 bytes for LE16) | `le16()` would read out of bounds | Callers must check `size() >= offset + 2` before calling. This is the existing pattern — all current call sites check size before accessing bytes. |

### 3.2 AirPods Battery Edge Cases

| Edge Case | Why It Matters | Test |
|-----------|---------------|------|
| Nibble value 11-15 | Reserved/special states in Apple protocol | U3-T5 — must not crash, must not silently clamp |
| Missing battery byte (data <7 bytes) | Older AirPods firmware or truncated advertisement | U3-T8 — gracefully omit battery |
| Missing case byte (data <8 bytes) | AirPods without case, or truncated | U3-T9 — show L/R only |
| All zeros | Device just powered on, or error state | U3-T2 — display 0% |
| Model byte = 0 | Unknown AirPods model | Should still parse battery if available |

### 3.3 Cross-Vendor Edge Cases

| Edge Case | Why It Matters | Test |
|-----------|---------------|------|
| Unknown company ID | `decode_proprietary_parts()` returns `std::nullopt` | Already handled — returns nullopt, caller handles |
| Company ID at boundary (0x0000, 0xFFFF) | Edge of 16-bit range | `le16()` handles all 16-bit values correctly |
| Multiple manufacturer data AD structures | Device sends two 0xFF AD types | Current code in `stat_view.cpp:99-113` breaks on first match. This is existing behavior, not a psc-0006 issue. |

---

## 4. Test Data Requirements

### 4.1 Synthetic Test Vectors (Priority: HIGH)

Synthetic test vectors are **essential** because they are deterministic and can be embedded as `static_assert` or compile-time tests. They don't require real hardware.

**Sources for synthetic vectors:**
1. **Apple iBeacon:** Well-documented format. UUID `E2C56DB5-DFFB-48D2-B060-D0F5A71096E0`, Major 0x0001, Minor 0x0002 → `mfr_data = {0x02, 0x15, 0xE2, 0xC5, 0x6D, 0xB5, 0xDF, 0xFB, 0x48, 0xD2, 0xB0, 0x60, 0xD0, 0xF5, 0xA7, 0x10, 0x96, 0xE0, 0x00, 0x01, 0x00, 0x02, 0xC5}`
2. **BLE Core Spec examples:** Service UUIDs, Appearance values, Connection Intervals — all have known encodings from the spec
3. **Reference implementations:** https://github.com/nicedouble/AppleBLEDecoder has test vectors for Apple/Samsung formats

### 4.2 Real BLE Captures (Priority: MEDIUM)

Real captures are needed for cross-validation but are harder to obtain deterministically.

**How to obtain:**
1. Run `bluetooth-at-driver -s -vv` near target devices, capture output
2. Use nRF Connect for Mobile to capture raw advertisement bytes for the same devices
3. Use Wireshark + nRF52840 dongle for packet-level capture (if available)

**Devices needed for validation:**
- Samsung SmartThings device (SmartTag, Galaxy Buds, etc.)
- Apple AirPods (any generation)
- Sony BLE device (headphones, speaker)
- Razer BLE peripheral (mouse, keyboard)

### 4.3 Golden Files (Priority: MEDIUM)

Golden files for regression testing of the refactoring (U4, U5). Capture before and after output from a 30-second scan in a BLE-rich environment.

---

## 5. Test Implementation Plan (Phase B)

### 5.1 What Can Be Implemented NOW (Without PSC-0014)

| Test File | Content | Build Integration |
|-----------|---------|-------------------|
| `test/compile_time/le_helpers_test.cpp` | U1-T1 through U1-T10 `static_assert` tests for `le16()`/`le32()` | Add as a source file to `ble_sniffer` library (temporary) or create a separate `add_executable(le_helpers_test ...)` that links `ble_sniffer` |
| `test/compile_time/byte_order_test.cpp` | U2-T1 through U2-T9 `static_assert` tests for Samsung/Sony/Razer byte order | Same — add to build, verify compilation passes |
| `test/compile_time/airpods_label_test.cpp` | U3-T1 through U3-T9 tests for AirPods battery labeling | These require calling `apple::parse()` and checking the string output — needs runtime, not `static_assert`. Could be a simple `main()` that asserts and returns 0 or 1. |

**Build integration approach:** Add a `test/` directory with its own `CMakeLists.txt`:

```cmake
# test/CMakeLists.txt — minimal, no Catch2 dependency
add_executable(le_helpers_test
    compile_time/le_helpers_test.cpp
)
target_link_libraries(le_helpers_test PRIVATE ble_sniffer)
# static_assert tests — compilation = test pass

add_executable(byte_order_test
    compile_time/byte_order_test.cpp
)
target_link_libraries(byte_order_test PRIVATE ble_sniffer)

add_executable(airpods_label_test
    compile_time/airpods_label_test.cpp
)
target_link_libraries(airpods_label_test PRIVATE ble_sniffer)
# This one has a main() that returns 0 on success, 1 on failure
```

Then in root `CMakeLists.txt`:
```cmake
add_subdirectory(test)
```

**This requires zero new Conan dependencies.** It uses only the existing `ble_sniffer` library.

### 5.2 What Requires PSC-0014 (Deferred)

| Test | Dependency |
|------|------------|
| Catch2 unit tests for all parsers | Catch2 via Conan |
| `MockSerialPort` for driver tests | PSC-0014 + Catch2 |
| Fuzz targets for `parse()` functions | libFuzzer + Clang |
| Golden file regression automation | Test runner script |

### 5.3 Logical Units for Test Implementation (Parallel to Code Units)

| Test Unit | Depends On Code Unit | Test File | Test Type |
|-----------|---------------------|-----------|-----------|
| **TU1** | U1 (`le16`/`le32` in types.h) | `test/compile_time/le_helpers_test.cpp` | `static_assert` |
| **TU2** | U2 (byte order fixes) | `test/compile_time/byte_order_test.cpp` | `static_assert` |
| **TU3** | U3 (AirPods labeling) | `test/compile_time/airpods_label_test.cpp` | Runtime assert (simple `main()`) |
| **TU4** | U4 (ad_parser refactor) | Golden diff OR synthetic test vectors | Manual diff or `static_assert` |
| **TU5** | U5 (stat_view refactor) | Golden diff | Manual diff |
| **TU6** | U6 (comment fixes) | Visual review | No test needed |

---

## 6. Test Strategy for DRY Refactoring (`le16()`/`le32()`)

### 6.1 The Refactoring Risk

The `le16()`/`le32()` refactoring touches **10 call sites across 4 files**. Every site currently has a manually-written LE parse that is known to be correct (except Samsung/Sony/Razer which are being fixed). The refactoring replaces known-correct code with a new helper.

**Risk mitigation:**
1. **Test the helper exhaustively first** (TU1 — 10 `static_assert` cases)
2. **Refactor one site at a time** (incremental execution per PAU loop)
3. **Build after each site** — if a refactored site produces a different value, the build won't catch it (both produce `uint16_t`), but a golden diff will
4. **Golden diff as final gate** — before claiming U4/U5 complete, diff output against pre-refactoring baseline

### 6.2 Regression Test Strategy

```
Phase B Step:
1. Build and run BEFORE any refactoring → capture golden_before.txt
2. Apply U1 (le16/le32 helpers) → build, run TU1 tests
3. Apply U2 (byte order fixes) → build, run TU2 tests
4. Apply U3 (AirPods labeling) → build, run TU3 tests
5. Apply U4 (ad_parser refactor) → build, run golden diff
6. Apply U5 (stat_view refactor) → build, run golden diff
7. Apply U6 (comments) → build
8. Final golden diff: golden_before.txt vs golden_after.txt
   - Expected differences: Samsung/Sony/Razer device_type/protocol_ver/model values (byte order fix), AirPods battery format
   - Expected NO differences: All other output (manufacturer names, UUIDs, appearance, connection intervals, iBeacon, etc.)
```

---

## 7. Blocking Questions and Flags

### 7.1 Flag: Test Infrastructure Gap — BLOCKING for Full Verification

**Type:** `task`
**Priority:** critical
**Raised by:** Test Engineer
**Blocking:** yes (blocks comprehensive testing, but does NOT block psc-0006 implementation — see rationale below)

**Description:** PSC-0014 (Catch2 test infrastructure) has not been completed. Without it, we cannot write:
- Unit tests with assertions for parser output strings
- Mock-based driver tests
- Fuzz targets for parser robustness
- Automated regression test suites

**Impact on psc-0006:** The byte order fixes and AirPods labeling CAN be verified with `static_assert` and manual CLI testing. The refactoring CAN be verified with golden diff. **psc-0006 does not need to wait for PSC-0014.** However, the test coverage will be thinner than ideal — we're relying on compile-time assertions and manual verification rather than a comprehensive unit test suite.

**Suggested action:** Proceed with psc-0006 using the `static_assert` + golden diff strategy. Create a follow-up ticket (or add to PSC-0014 scope) to backfill Catch2 unit tests for all proprietary parsers once infrastructure exists.

### 7.2 Flag: Pre-existing Build Error in bluetooth_at_driver.cpp

**Type:** `task`
**Priority:** high
**Raised by:** Test Engineer
**Blocking:** yes (blocks ALL build verification)

**Description:** `src/bluetooth_at_driver.cpp:105` has `m_serial_port.` (incomplete member access — missing method name after the dot). This causes the build to fail with:
```
error: expected unqualified-id
  105 |     info += "Baud Rate: " + std::to_string(at_baud_param_to_num<int>(m_serial_port.)) + "\n";
```

**Impact:** Cannot run any build verification (golden diff, static_assert compilation, manual CLI testing) until this is fixed.

**Suggested action:** Fix this before any psc-0006 implementation begins. This is a one-character fix (add the missing method name). Create a quick `bugfix` ticket or fix as a pre-requisite step in psc-0006 Phase B.

### 7.3 Question: AirPods Battery Mapping — Which Option?

The SW Engineer proposed three options for AirPods battery labeling:
- **Option A:** `battery L=7 (raw level)` — just label as raw
- **Option B:** `battery L=70%` — percentage only
- **Option C:** `battery L=70% (raw=7)` — percentage + raw parenthetical (SW Engineer's recommendation)

**Test Engineer recommendation:** **Option C** — it gives users actionable information (percentage) while preserving the raw data for debugging. This is consistent with how the `charging` field is already displayed (`charging=0x7` — raw hex, not interpreted). The raw parenthetical also makes it obvious when a value is out of the 0-10 range (e.g., `battery L=?? (raw=15)`).

**Question for user:** Confirm Option C is the desired approach.

### 7.4 Question: Out-of-Range Battery Nibble Handling

What should happen when a battery nibble is 11-15 (outside the 0-10 range)?

**Options:**
1. Display `??` for percentage, show raw value: `battery L=?? (raw=15)`
2. Clamp to 10 (100%): `battery L=100% (raw=15)` — **NOT recommended** (hides data)
3. Display as hex: `battery L=0xF (raw=15)` — confusing (mixes hex and decimal)
4. Display special label: `battery L=UNKNOWN (raw=15)`

**Test Engineer recommendation:** Option 1 (`??` for percentage, raw value shown). This is honest about uncertainty and preserves all data for debugging.

### 7.5 Question: Sony/Razer Byte Order — Apply Fix or Defer?

The SW Engineer flagged that Sony and Razer byte order cannot be 100% verified without vendor protocol documentation. The BLE convention says little-endian, so the fix is likely correct.

**Test Engineer recommendation:** Apply the fix to Sony and Razer alongside Samsung. The risk of being wrong is low (BLE convention is clear), and if wrong, it will be immediately visible in manual testing (the displayed value won't match nRF Connect). Add a comment noting the BLE convention assumption.

---

## 8. Self-Audit Checklist

| Category | Checked? | Finding or PASS |
|----------|----------|-----------------|
| Build passes (exit 0, no warnings) | no | **FAIL** — pre-existing build error in `bluetooth_at_driver.cpp:105` (`m_serial_port.` incomplete). Must be fixed before any psc-0006 implementation. |
| Typed enums / vocabulary types (no raw integers in API) | yes | PASS — `le16()`/`le32()` return `uint16_t`/`uint32_t` which is appropriate for byte-level parsing utilities. No new public API types needed. |
| Documentation on new public symbols | n/a | PASS — `le16()`/`le32()` will be in `types.h` with Doxygen. Test files are internal, no public symbols. |
| Spec/datasheet fidelity (fields match spec) | yes | PASS — BLE Core Spec Vol 1 Part A §1 mandates little-endian. iBeacon spec (Apple) mandates big-endian for Major/Minor — correctly identified as exception. |
| Module boundary (no platform headers in shared modules) | yes | PASS — test files will be in `test/` directory, not in `include/`. No platform headers introduced. |
| Reserved/padding fields handled | n/a | PASS — not applicable to byte parsing utilities or labeling changes. |
| No magic numbers in doc examples | n/a | n/a — no new doc examples in this review. |
| Buffer safety (bounded copies) | yes | PASS — `le16()`/`le32()` take `const uint8_t*` and read exactly 2/4 bytes. Callers are responsible for bounds checking (existing pattern). |
| AGENTS.md compliance | yes | PASS — test files follow project conventions. `static_assert` tests are pure, no state, no I/O. |

---

## 9. Verdict

**VERDICT: CONDITIONAL PASS**

**Rationale:**

The test strategy for psc-0006 is sound and implementable **without** waiting for PSC-0014 (Catch2 infrastructure). The key insight is that `le16()`/`le32()` helpers and byte-order fixes can be verified at **compile time** via `static_assert`, and the refactoring can be verified via **golden file diff**. This avoids the test infrastructure bottleneck.

However, **three conditions** must be resolved:

1. **Pre-existing build error** (`bluetooth_at_driver.cpp:105`) — must be fixed before any implementation or verification can proceed. This is a hard blocker for Phase B.
2. **AirPods labeling option** — Option C (percentage + raw parenthetical) is recommended but needs user confirmation.
3. **Out-of-range nibble handling** — Option 1 (`??` for percentage) is recommended but needs user confirmation.

**Coverage:** 0/4 acceptance criteria currently have executable test evidence. After Phase B implementation with the proposed test strategy, coverage target is 4/4:
- AC1 (Samsung LE byte order): TU2 `static_assert` + U2-R1 real device
- AC2 (AirPods battery labeling): TU3 runtime test + U3-R1 real device
- AC3 (Doxygen @note): Visual review (no executable test needed)
- AC4 (Build passes): Build verification after all units

**Blocking findings:** None at design level. The pre-existing build error is an implementation blocker, not a design blocker.

**ROUTING:** Proceed to Wireless Expert (A1-WX) for protocol-level verification of byte order analysis, then to A2 Dual-Model Challenge.

---

## 10. Summary of Test Cases

| Test Unit | Test Count | Test Type | Infrastructure Needed |
|-----------|------------|-----------|----------------------|
| TU1 — `le16()`/`le32()` helpers | 10 | `static_assert` | None (compile-only) |
| TU2 — Byte order fixes | 9 synthetic + 3 real device | `static_assert` + manual CLI | None (compile-only + existing CLI) |
| TU3 — AirPods labeling | 9 synthetic + 4 real device | Runtime assert + manual CLI | Simple `main()` test executable |
| TU4 — ad_parser refactor | 5 synthetic + golden diff | `static_assert` + manual diff | None + existing CLI |
| TU5 — stat_view refactor | Golden diff | Manual diff | Existing CLI |
| TU6 — Comment fixes | Visual review | None | None |
| **Total** | **33 synthetic + 7 real device + 2 golden diffs** | | |

---

## Flag Summary

| # | Type | Priority | Blocking | Description |
|---|------|----------|----------|-------------|
| F1 | `task` | critical | yes | Pre-existing build error in `bluetooth_at_driver.cpp:105` — must fix before Phase B |
| F2 | `task` | critical | no (for psc-0006) | PSC-0014 (Catch2 infrastructure) not completed — limits test coverage depth but does not block psc-0006 |
| F3 | `clarification` | high | no | Confirm AirPods battery labeling Option C (percentage + raw parenthetical) |
| F4 | `clarification` | medium | no | Confirm out-of-range nibble handling (Option 1: `??` for percentage) |
| F5 | `clarification` | medium | no | Confirm Sony/Razer byte order fix should be applied (BLE convention assumption) |
