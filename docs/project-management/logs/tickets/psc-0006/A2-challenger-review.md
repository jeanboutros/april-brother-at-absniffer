# A2-Challenger: Software Engineer Challenger — Dual-Model Challenge Review

| Field | Value |
|-------|-------|
| Model | ollama-cloud/glm-5.1 |
| Phase | A2 |
| Primary Output | Comprehensive design proposal for psc-0006: byte order fixes (Samsung/Sony/Razer), le16()/le32() helpers, AirPods battery labeling Option C, out-of-range ?? handling, DRY refactoring of 10 call sites, full Doxygen for 14+2 symbols, 3 new doc files, broken URL fixes, 7 logical units, 4 ADR candidates |
| Challenger | software-engineer-challenger |
| Timestamp | 2026-06-14T23:00:00Z |

---

## Overall Assessment

The primary agent (deepseek-v4-pro) has produced a thorough, well-structured design proposal that synthesizes all four A1 specialist reviews into a coherent implementation plan. The architecture is sound, the byte order analysis is correct, and the test strategy is pragmatic given the lack of Catch2 infrastructure. However, I identify **several significant concerns** — primarily around the pointer-based `le16()`/`le32()` API safety, an inflated type design score, a missing `be16()` helper for iBeacon, and an under-examined `format_battery` lambda design. These concerns do not invalidate the proposal but require resolution before A-GATE.

---

## Agreements

### A1: Samsung/Sony/Razer byte order bugs — Confirmed (Confidence: 95)

I independently verified all three buggy patterns in the source code at `proprietary_parsers.cpp:189, 240, 301`. The `(byte[0] << 8) | byte[1]` pattern is definitively big-endian and violates BLE Core Spec Vol 6 Part B §1.3.1. The fix to `le16()` is correct.

### A2: iBeacon Major/Minor — Correctly big-endian, no fix needed (Confidence: 90)

The iBeacon parser at lines 57-58 uses `(mfr_data[18] << 8) | mfr_data[19]` which is big-endian — but this is correct per Apple's iBeacon specification. The primary correctly identifies this as a vendor-specific exception. No change needed.

### A3: AirPods battery Option C (percentage + raw parenthetical) (Confidence: 85)

Option C is the right choice. It gives users actionable percentages while preserving the raw nibble value for debugging. This is consistent with the existing `charging=0x7` format. The `??` display for out-of-range values (11-15) is honest and preserves data.

### A4: Broken URL fixes (Confidence: 90)

All 5 references to `nicedouble/AppleBLEDecoder` are confirmed 404 per A1-DX. Replacing with a note pointing to `furiousMAC/continuity` is the correct action.

### A5: Full Doxygen for proprietary_parsers.h (Confidence: 95)

Zero Doxygen on all 14 public symbols is a T1.2 failure. The primary's plan to add full Doxygen blocks with `@note` tags is necessary and correct.

### A6: Logical unit sequencing (U0→U1→U2→U3→U4→U5→U6→U7) (Confidence: 80)

The dependency ordering is correct: helpers must exist before call sites can use them (U1 before U2-U5), byte order fixes before labeling (U2 before U3), and documentation last (U6) since it should reflect the final state.

### A7: 3 new documentation files (Confidence: 80)

The learning docs (`ble-byte-order-conventions.md`, `vendor-parser-reverse-engineering.md`) and module doc (`proprietary-parsers.md`) fill a pre-existing gap. The content is well-structured and accurate.

### A8: ADR candidates identified (Confidence: 75)

The 4 ADR candidates are appropriate. The le16/le32 helper decision, AirPods labeling format, out-of-range handling, and Sony/Razer byte order assumption are all architectural decisions worth recording.

---

## Disagreements

### D1: Type Design Review Score for `le16()`/`le32()` — Inflated (Confidence: 85)

**Primary claims:** Overall score 10.0/10 — "Excellent — meets all typed vocabulary standards." All four dimensions scored 10.

**My assessment:** This score is inflated. The `le16()`/`le32()` functions are **untyped pointer-based APIs with no bounds checking**. While this is a deliberate design choice (the primary argues callers are responsible for bounds checking), it does NOT merit a perfect score across all four dimensions.

| Dimension | Primary Score | My Score | Justification |
|-----------|--------------|----------|---------------|
| Encapsulation | 10 | 8 | The `const uint8_t*` parameter is raw — callers can pass any pointer, including dangling or insufficient-length. The function cannot verify preconditions. This is not "users cannot create invalid states" (a 10/10 criterion). Passing a pointer to 1 byte where 2 are needed is a real invalid state. |
| Invariant Expression | 10 | 7 | `constexpr` does NOT mean "all values verified at compile time." `constexpr` means it CAN be evaluated at compile time IF the inputs are compile-time constants. When called with runtime data (which is the primary use case — parsing BLE advertisement buffers), there is zero compile-time verification. Invalid states (OOB pointer) are not rejected at compile time. |
| Usefulness | 10 | 9 | Self-documenting name is good, but the pointer API is less useful than a bounded API for debugging — you can't inspect the source buffer from the function signature. |
| Enforcement | 10 | 6 | `const uint8_t*` is a raw type. There is no enforcement that the caller passes a valid pointer with sufficient bytes. The type-design-review skill defines Enforcement 10 as: "All public API surface uses typed enums and struct wrappers. Raw `uint8_t` overloads exist but are `private`." The `le16()` API IS a raw `const uint8_t*` — no typed wrapper at all. |

**My Overall Score:** 7.5/10 — Acceptable, but NOT excellent. The API is appropriate for a low-level byte-parsing utility, but scoring it 10/10 on every dimension misrepresents the design tradeoffs.

**Suggested Fix:** Acknowledge the pointer-based API's limitations honestly in the type design review. This does NOT mean changing the API (the pointer design is correct for this use case), but the self-assessment should be accurate.

### D2: Missing `be16()` Helper for iBeacon — Should Be In Scope (Confidence: 80)

**Primary position:** `read_be16()` is "a nice-to-have but not in scope for psc-0006." Deferred to a separate ticket.

**My position:** The primary is creating helper functions specifically to prevent byte-order bugs and make intent self-documenting. The iBeacon parser is the **only place in the codebase that uses big-endian parsing**, and it relies on a comment (`// (BE)`) to document this intent. The primary's own rationale for `le16()` applies equally to `be16()`:

1. **Bug prevention:** If `be16()` existed, a developer seeing `be16(&mfr_data[18])` immediately knows this is big-endian — no comment needed. Without it, a future developer could "fix" the iBeacon code to use `le16()`, introducing a regression.
2. **Self-documenting:** `be16()` makes the big-endian intent explicit in code, not just in comments. This is the primary's own stated goal for `le16()`.
3. **Minimal effort:** Adding `be16()` is 3 lines of code + 5 `static_assert` tests. It follows the exact same pattern as `le16()`.

**Evidence:** The WX specialist explicitly recommended `read_be16()` at Finding 10: "A `read_be16()` helper could also be useful for the iBeacon case, making the big-endian intent explicit rather than relying on a comment." The primary acknowledged this in the proposal but rejected it as out of scope.

**Recommendation:** Add `be16()` to U1 alongside `le16()` and `le32()`. This is a 10-minute addition that completes the byte-order helper suite. Deferring it creates an inconsistency: LE parsing has a self-documenting helper, but BE parsing (the exception case!) does not. The iBeacon comment update in U2 should use `be16()` instead of just improving the comment.

### D3: `format_battery` Lambda Is Not Reusable and Should Be a Free Function (Confidence: 70)

**Primary position:** "Making it a lambda keeps it scoped to the single call site. If other parsers need battery formatting in the future, it can be promoted to a free function."

**My position:** This is a reasonable short-term choice, but it has two problems:

1. **DRY violation risk:** If Apple adds a new AirPods subtype (e.g., AirPods Max with different battery encoding), the lambda would need to be duplicated or the code restructured. A free function in an anonymous namespace in `proprietary_parsers.cpp` would serve the same scoping purpose without being a local lambda.
2. **Testability:** The lambda is defined inside the `case 0x07:` block. It cannot be tested in isolation. The `airpods_label_test.cpp` test file tests the full `apple::parse()` function, which is fine for integration testing, but if the battery formatting logic needs to change (e.g., Apple changes the encoding for a new model), you can't unit-test the formatter independently.

**Recommendation:** Keep the lambda for now (it's pragmatic), but add a comment noting that it should be promoted to a free function (anonymous namespace) if it needs to be reused. This is advisory, not blocking.

---

## Missing Considerations

### M1: `le16()`/`le32()` Pointer Safety — No Defensive Documentation (Confidence: 80)

The `le16()`/`le32()` functions take a `const uint8_t*` with no bounds checking. The Doxygen says "Caller is responsible for bounds checking" — but this is an **assumption that must be verified**, not just documented.

**Risk:** There are currently 10 call sites. I verified that all 10 check `size() >= offset + N` before accessing bytes. But the helper is now a PUBLIC API in `types.h`. Any future caller can use `le16()` without bounds checking, and the compiler won't catch it.

**Primary's position:** "This is the existing pattern in all 10 call sites" and "adding bounds checking in the helper would be redundant and introduce inconsistency."

**My assessment:** The primary's argument is valid for the CURRENT call sites, but it doesn't address FUTURE call sites. The `@note` in the Doxygen should be stronger:

```cpp
@warning Undefined behavior if data points to fewer than 2 consecutive bytes.
         Callers MUST ensure the buffer has at least 2 bytes available
         starting from the data pointer before calling this function.
```

`@warning` is a standard Doxygen tag that renders differently from `@note` — it signals danger. The current `@note` is too soft for an API that can cause buffer overread.

**Recommendation:** Upgrade the bounds-checking documentation from `@note` to `@warning`. This is a zero-cost change that makes the contract unambiguous.

### M2: Alignment and Strict Aliasing Concerns for `le16()`/`le32()` (Confidence: 60)

The `le16()` implementation does byte-by-byte access via `data[0]` and `data[1]`, which is correct and avoids strict aliasing violations. However, the primary should explicitly note in the Doxygen that this function is safe from alignment/punning issues because it uses byte-level access, not `reinterpret_cast<uint16_t*>`. This is a common C++ pitfall and worth documenting briefly.

**Recommendation:** Add a one-line `@note` to `le16()` and `le32()`: "Safe for unaligned access — uses byte-level reads, not pointer cast."

### M3: Samsung `device_type` Verification Data Missing (Confidence: 75)

The primary's byte order fix for Samsung `device_type` is based on BLE Core Spec convention. But there's a stronger form of verification available: **known Samsung SmartTag device type values**. The SmartThings community and Samsung developer documentation list specific `device_type` values for known devices (e.g., SmartTag = 0x0004, SmartTag+ = 0x0008, Galaxy Buds = 0x0001).

If the current (buggy) code displays `device_type=0x0400` for a SmartTag, and the fix changes it to `device_type=0x0004`, this can be cross-validated against known values — not just against BLE convention. The primary mentions real-device verification in the test strategy, but doesn't provide the specific known values.

**Recommendation:** Add a comment in `proprietary_parsers.cpp` near the Samsung fix listing 1-2 known device_type values for cross-validation. Example:
```cpp
// Known values: SmartTag=0x0004, Galaxy Buds=0x0001
uint16_t device_type = le16(&mfr_data[1]);
```

### M4: AirPods Battery Nibble 11-15 Semantics — Incomplete Documentation (Confidence: 70)

The primary documents nibble values 11-15 as "reserved/special states" and displays them as `??`. But the WX specialist and community references suggest specific meanings for some of these values:

- Nibble value 15 (0xF) is commonly reported as "charging in case" for some AirPods models
- Nibble value 7 (0x7) for the case byte may indicate a different charging state

The primary's `??` approach is safe (it doesn't misrepresent unknown data), but the `@note` documentation should mention that some community sources attribute specific meanings to values 11-15, and that the parser intentionally does not interpret these.

**Recommendation:** Add to the apple::parse() `@note`: "Nibble values 11-15 are not interpreted by this parser. Some community sources attribute specific meanings (e.g., 0xF = 'charging in case'), but these are not well-documented and may vary by model."

### M5: `stat_view.cpp` Company ID Parsing in Loop — Missed Refactoring (Confidence: 65)

The primary identifies 1 refactoring site in `stat_view.cpp` (line 102-103). But looking at the code, this company ID parsing is inside a `for` loop over AD structures (lines 99-113). The loop breaks after the FIRST 0xFF match:

```cpp
for (const auto& ad : structures) {
    if (ad.type == 0xFF) {
        if (ad.data.size() >= 2) {
            uint16_t cid = static_cast<uint16_t>(ad.data[0]) |
                           (static_cast<uint16_t>(ad.data[1]) << 8);
            e.manufacturer = company_name_from_id(cid);
        }
        // ...
        break;
    }
}
```

This is correct for the current use case (most devices send one manufacturer data AD structure), but it's worth noting as a limitation. The TX specialist flagged this at §3.3: "Current code in `stat_view.cpp:99-113` breaks on first match. This is existing behavior, not a psc-0006 issue."

**Recommendation:** Not blocking, but document this limitation in the `proprietary-parsers.md` module doc under the "Known Limitations" section.

### M6: Test File Naming — `compile_time` Directory Misnomer (Confidence: 60)

The primary proposes `test/compile_time/byte_order_test.cpp` and `test/compile_time/airpods_label_test.cpp`, but these are NOT compile-time tests — they contain `main()` functions with runtime `assert()` calls. Only `le_helpers_test.cpp` is truly compile-time (using `static_assert`).

Putting runtime assertion tests in a directory called `compile_time` is misleading. A developer looking at the test directory structure would expect `compile_time/` to contain only `static_assert` tests.

**Recommendation:** Split into two directories:
- `test/compile_time/` — only `static_assert` tests (le_helpers_test.cpp)
- `test/runtime/` — runtime assertion tests (byte_order_test.cpp, airpods_label_test.cpp)

Or rename to `test/static_assert/` and `test/assertion/` for clarity.

### M7: CMake Test Integration — Missing `ctest` Support (Confidence: 55)

The primary proposes adding test executables to the build but does not include `enable_testing()` or `add_test()` calls. This means `ctest` won't discover or run the tests automatically.

**Recommendation:** Add to `test/CMakeLists.txt`:
```cmake
enable_testing()
add_test(NAME le_helpers_test COMMAND le_helpers_test)
add_test(NAME byte_order_test COMMAND byte_order_test)
add_test(NAME airpods_label_test COMMAND airpods_label_test)
```

This costs nothing and enables `ctest` integration for CI.

---

## Cross-Validation Against A1 Specialist Reviews

### SW (Software Engineer) — 6 findings

| SW Finding | Primary Addressed? | My Assessment |
|------------|-------------------|---------------|
| F1: Samsung device_type byte order | ✅ Fixed with le16() | Agree |
| F2: AirPods battery nibbles | ✅ Option C adopted | Agree |
| F3: Sony/Razer byte order | ✅ Fixed with le16() | Agree, but note assumption risk |
| F4: iBeacon TX power comment | ✅ Advisory only | Agree, trivial |
| F5: No helper function | ✅ le16()/le32() added | Agree, but be16() should also be added |
| F6: LE32 at ad_parser:103-105 | ✅ le32() covers this | Agree |

### TX (Test Engineer) — 5 flags

| TX Flag | Primary Addressed? | My Assessment |
|---------|-------------------|---------------|
| F1: Build error | ✅ Already resolved | Verified — build passes |
| F2: No Catch2 | ✅ Workaround with static_assert + runtime assert | Agree, pragmatic |
| F3: AirPods Option C | ✅ Confirmed | Agree |
| F4: Out-of-range ?? | ✅ Option 1 confirmed | Agree |
| F5: Sony/Razer byte order | ✅ Apply with comment | Agree |

### DX (Docs Writer) — 14 findings

| DX Finding | Primary Addressed? | My Assessment |
|------------|-------------------|---------------|
| F1: Zero Doxygen on 14 symbols | ✅ Full Doxygen planned | Agree |
| F2: No @note on reverse-engineered status | ✅ @note planned per vendor | Agree |
| F3: Broken URL (5 instances) | ✅ Replace with note | Agree |
| F4: No @note on BLE LE convention | ✅ File-level + per-vendor | Agree |
| F5: No @note on AirPods battery encoding | ✅ @note planned | Agree, but should mention 11-15 ambiguity (see M4) |
| F6: le16/le32 need Doxygen | ✅ Full Doxygen planned | Agree, but upgrade to @warning (see M1) |
| F7: Out-of-range nibble @note | ✅ @note planned | Agree |
| F8: Microsoft @note distinction | ✅ "Officially documented" | Agree |
| F9: Sony/Razer @note assumption | ✅ "Not verified" note | Agree |
| F10: Learning docs | ✅ 2 new files planned | Agree |
| F11: Module doc | ✅ 1 new file planned | Agree |
| F12: iBeacon comment clarity | ✅ Minor fix | Agree |
| F13: PETS paper URL | ❌ Not addressed | Advisory, not blocking |
| F14: Apple iBeacon URL | ❌ Not addressed | Advisory, not blocking |

**Note:** DX findings F13 and F14 were not addressed by the primary. These are advisory (confidence <80) but should at least be acknowledged in the proposal.

### WX (Wireless Expert) — 4 blocking findings

| WX Finding | Primary Addressed? | My Assessment |
|------------|-------------------|---------------|
| WX-F1: Samsung device_type | ✅ Fixed | Agree |
| WX-F2: Sony protocol_ver | ✅ Fixed | Agree |
| WX-F3: Razer model | ✅ Fixed | Agree |
| WX-F4: AirPods battery misleading | ✅ Option C fixes | Agree |

**WX Advisory:** `read_be16()` for iBeacon — **Rejected by primary as out of scope.** I disagree with this rejection (see D2).

---

## Findings

| ID | Confidence | Severity | Category | Description | Suggested Fix |
|----|-----------|----------|----------|-------------|---------------|
| F1 | 85 | High | Type Design | `le16()`/`le32()` type design score inflated to 10.0/10 — pointer-based raw API cannot score perfect on Encapsulation, Invariant Expression, or Enforcement | Correct the self-assessment to ≤8.0; acknowledge pointer API tradeoffs honestly |
| F2 | 80 | High | API Completeness | Missing `be16()` helper for iBeacon big-endian parsing — WX recommended it, primary rejected as "out of scope" but the rationale for `le16()` applies equally | Add `be16()` to U1; use it for iBeacon Major/Minor instead of comment-only documentation |
| F3 | 80 | High | API Safety | `le16()`/`le32()` bounds-checking documentation uses `@note` instead of `@warning` — the consequence of misuse is UB (buffer overread), which warrants stronger documentation | Change `@note` to `@warning` with explicit "Undefined behavior" language |
| F4 | 70 | Moderate | Documentation | AirPods nibble values 11-15 documented only as "reserved/special states" — some community sources attribute specific meanings that should be acknowledged | Add `@note` acknowledging community attributions but stating parser intentionally does not interpret them |
| F5 | 65 | Moderate | Test Organization | Runtime assertion tests in `test/compile_time/` directory is misleading — only `static_assert` tests are truly compile-time | Split into `test/compile_time/` and `test/runtime/` (or rename) |
| F6 | 60 | Moderate | Documentation | `le16()`/`le32()` Doxygen should note alignment safety — byte-level reads avoid strict aliasing/alignment issues, which is worth documenting | Add `@note`: "Safe for unaligned access — uses byte-level reads, not pointer cast." |
| F7 | 60 | Moderate | Build | Missing `enable_testing()` and `add_test()` calls — tests won't be discovered by `ctest` | Add `enable_testing()` and `add_test()` to `test/CMakeLists.txt` |
| F8 | 55 | Low | Test Data | Samsung `device_type` fix lacks known-value cross-validation data | Add 1-2 known SmartTag device_type values as comments for cross-validation |
| F9 | 55 | Low | DX Acknowledgment | DX F13 (PETS paper URL) and F14 (Apple iBeacon URL) not acknowledged in primary proposal | Add advisory acknowledgment in proposal or as comment in code |
| F10 | 50 | Low | Lambda Design | `format_battery` lambda is untestable in isolation and cannot be reused without promotion | Keep for now, but add comment noting future promotion to free function if reused |

---

## Blocking Findings (confidence ≥80)

- **F1:** Type design score inflation (85) — while this doesn't change the code, an inaccurate self-assessment masks real design tradeoffs. The pointer-based API is appropriate but not perfect. Score should be corrected before A-GATE.
- **F2:** Missing `be16()` helper (80) — the primary's own rationale for `le16()` (self-documenting intent, preventing bugs) applies equally to `be16()` for iBeacon. Deferring creates an inconsistency where LE intent is code-enforced but BE intent is comment-only.
- **F3:** `@warning` vs `@note` for bounds checking (80) — UB is a serious consequence. The documentation should use the strongest Doxygen tag available.

---

## Recommendations

### R1: Add `be16()` Helper (Confidence: 80)

Add to `types.h` alongside `le16()` and `le32()`:

```cpp
/**
 * @brief Parse a big-endian 16-bit unsigned integer from a byte buffer.
 *
 * For vendor-specific formats that use big-endian (e.g., Apple iBeacon
 * Major/Minor). This is a vendor exception to the BLE little-endian convention.
 *
 * @warning Undefined behavior if data points to fewer than 2 consecutive bytes.
 *
 * @example
 * @code
 * const uint8_t buf[] = {0x12, 0x34};
 * uint16_t val = ble_sniffer::be16(buf);
 * // val == 0x1234
 * @endcode
 */
inline constexpr uint16_t be16(const uint8_t* data) noexcept {
    return (static_cast<uint16_t>(data[0]) << 8) | static_cast<uint16_t>(data[1]);
}
```

Use in iBeacon parser:
```cpp
uint16_t major = be16(&mfr_data[18]);  // BE per Apple iBeacon spec
uint16_t minor = be16(&mfr_data[20]);
```

### R2: Correct Type Design Score (Confidence: 85)

| Dimension | Corrected Score | Justification |
|-----------|----------------|---------------|
| Encapsulation | 8 | Pointer parameter allows invalid states (dangling, OOB); function cannot verify preconditions |
| Invariant Expression | 7 | `constexpr` enables compile-time evaluation but runtime use is unchecked |
| Usefulness | 9 | Self-documenting name, but pointer API is less debuggable than bounded API |
| Enforcement | 6 | Raw `const uint8_t*` — no type enforcement on buffer validity |
| **Overall** | **7.5** | Acceptable for low-level utility, but not excellent |

### R3: Upgrade Bounds-Check Documentation to `@warning` (Confidence: 80)

```cpp
@warning Undefined behavior if data points to fewer than 2 consecutive bytes.
         Callers MUST ensure sufficient buffer length before calling.
```

### R4: Add `enable_testing()` to CMake (Confidence: 55)

```cmake
enable_testing()
add_test(NAME le_helpers_test COMMAND le_helpers_test)
add_test(NAME byte_order_test COMMAND byte_order_test)
add_test(NAME airpods_label_test COMMAND airpods_label_test)
```

### R5: Add Alignment Safety Note (Confidence: 60)

```cpp
@note Safe for unaligned access — uses byte-level reads, not pointer cast.
```

---

## Verdict

**CONDITIONAL PASS**

### Rationale

The primary's proposal is comprehensive and addresses all four A1 specialist reviews effectively. The core bug fixes (Samsung/Sony/Razer byte order, AirPods battery labeling) are correct and well-planned. The architecture is sound — `le16()`/`le32()` in `types.h` follows the inward-dependency rule and eliminates a DRY violation. The test strategy is pragmatic given the lack of Catch2 infrastructure.

However, **three blocking findings** must be resolved:

1. **F1 (85):** Type design self-assessment is inflated. A pointer-based raw API scoring 10/10 on every dimension misrepresents design tradeoffs. Correct the score to reflect the actual strengths and limitations.
2. **F2 (80):** Missing `be16()` helper creates an inconsistency — LE intent is code-enforced, but BE intent (the exception case!) is comment-only. Adding `be16()` is minimal effort and completes the byte-order helper suite.
3. **F3 (80):** Bounds-checking documentation should use `@warning`, not `@note`. Buffer overread is UB, which warrants the strongest Doxygen tag.

### Conditions for APPROVED

1. Correct the type design score for `le16()`/`le32()` to ≤8.0 overall (F1)
2. Add `be16()` helper to U1 alongside `le16()`/`le32()`, and use it for iBeacon Major/Minor (F2)
3. Upgrade bounds-checking documentation from `@note` to `@warning` for `le16()`, `le32()`, and `be16()` (F3)

### Advisory (should fix but not blocking)

- F4: Acknowledge community attributions for AirPods nibble values 11-15
- F5: Rename `test/compile_time/` for runtime assertion tests
- F6: Add alignment safety `@note`
- F7: Add `enable_testing()` to CMake
- F8: Add known Samsung device_type values as cross-validation data
- F9: Acknowledge DX F13/F14 (advisory findings)
- F10: Add comment about future `format_battery` promotion

### Routing

Proceed to A2a (ADR creation) after resolving the 3 blocking findings. Then to A-GATE.

---

## Self-Reflection

1. **Why might the primary have inflated the type design score?** The primary was evaluating a low-level utility function against a rubric designed for high-level API types (register structs, driver classes). The rubric's criteria for "10/10 Encapsulation — users cannot create invalid states" is unrealistic for a byte-parsing helper that takes a raw pointer. The primary should have contextualized the score based on the function's role rather than applying the rubric literally.

2. **Why did the primary reject `be16()`?** The primary likely viewed scope strictly — "psc-0006 is about fixing Samsung byte order, not refactoring iBeacon." But this is a false scope boundary: the `le16()` helper is being added specifically to prevent byte-order bugs and make intent self-documenting. `be16()` serves the identical purpose for the opposite byte order. Excluding it creates a gap where the "bug prevention" rationale only applies to LE, not BE.

3. **What procedural safeguard would catch the `@warning` vs `@note` issue?** A T1.9-style check that flags `@note` in Doxygen when the described condition involves UB. Any `@note` containing "undefined behavior", "buffer overread", or "caller must ensure" should be upgraded to `@warning`.
