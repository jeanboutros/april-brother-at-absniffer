# C2-WX: Wireless Expert Specialist Review — psc-0006

| Field | Value |
|-------|-------|
| Agent | wireless-expert |
| Timestamp | 2026-06-15T13:00:00Z |
| Step | C2-WX |
| Verdict | APPROVED |
| Spec ref | Bluetooth Core Specification Vol 1 Part A §1, Vol 6 Part B §1.3.1; Apple iBeacon Proximity Specification; furiousMAC/continuity |

## Reference Validation

| Primary Claim | Reference Provided | Authority Level | Verified? | Correctly Applied? |
|--------------|-------------------|-----------------|-----------|-------------------|
| BLE multi-byte fields are little-endian | [Spec: Bluetooth Core Specification, Vol 1, Part A, Section 1] | 1 — Definitive | ✓ | ✓ — `le16()`/`le32()` correctly implement LSByte-first |
| iBeacon Major/Minor are big-endian | [Spec: Apple iBeacon Proximity Specification] | 2 — Authoritative | ✓ | ✓ — `be16()` correctly used at `proprietary_parsers.cpp:57-58` |
| AirPods battery nibble 0-10 → 0%-100% | [Source: furiousMAC/continuity, community reference] | 8 — Moderate | ✓ | ✓ — `format_battery` lambda correctly maps nibble×10 |
| Company ID in AD type 0xFF is 2 bytes LE | [Spec: Bluetooth Core Specification, Vol 1, Part A, Section 1] | 1 — Definitive | ✓ | ✓ — `le16(&ad.data[0])` at all 3 call sites |
| AD structure 16/32-bit UUIDs are LE | [Spec: Bluetooth Core Specification, Vol 1, Part A, Section 1] | 1 — Definitive | ✓ | ✓ — `le16()`/`le32()` at `ad_parser.cpp:88,102` |
| AD structure Appearance is LE | [Spec: Bluetooth Core Specification, Vol 1, Part A, Section 1] | 1 — Definitive | ✓ | ✓ — `le16()` at `ad_parser.cpp:140` |
| AD structure Connection Interval is LE | [Spec: Bluetooth Core Specification, Vol 1, Part A, Section 1] | 1 — Definitive | ✓ | ✓ — `le16()` at `ad_parser.cpp:149-150` |

## Key Findings

| # | Area | Finding | Severity | Confidence | Recommendation |
|---|------|---------|----------|------------|----------------|
| 1 | Samsung `device_type` | `le16(&mfr_data[1])` at `proprietary_parsers.cpp:205` — correct LE per BLE Core Spec Vol 1 Part A §1. Verified by `byte_order_test.cpp` Tests 1-2. | PASS | 95 | None — correct |
| 2 | Sony `protocol_ver` | `le16(&mfr_data[0])` at `proprietary_parsers.cpp:256` — correct LE per BLE Core Spec. Verified by `byte_order_test.cpp` Test 3. | PASS | 95 | None — correct |
| 3 | Razer `model` | `le16(&mfr_data[0])` at `proprietary_parsers.cpp:317` — correct LE per BLE Core Spec. Verified by `byte_order_test.cpp` Test 4. | PASS | 95 | None — correct |
| 4 | iBeacon Major/Minor | `be16(&mfr_data[18])` / `be16(&mfr_data[20])` at `proprietary_parsers.cpp:57-58` — correct BE per Apple iBeacon spec. Verified by `byte_order_test.cpp` Tests 5, 6, 9 (discriminator). | PASS | 90 | None — correct |
| 5 | AirPods battery nibble | `format_battery` lambda at `proprietary_parsers.cpp:90-95` — correct 0-10→0%-100% mapping, 11-15→"??" per ADR-0008. Verified by `airpods_label_test.cpp` Tests 1-9. | PASS | 85 | None — correct |
| 6 | Company ID extraction | `le16(&ad.data[0])` at `ad_parser.cpp:40`, `proprietary_parsers.cpp:356`, `stat_view.cpp:102` — all correct LE. Verified by `byte_order_test.cpp` Tests 7, 8, 10. | PASS | 95 | None — correct |
| 7 | AD 16-bit UUID | `le16(&ad.data[i])` at `ad_parser.cpp:88` — correct LE. Verified by `byte_order_test.cpp` Test 11 (discriminator). | PASS | 95 | None — correct |
| 8 | AD 32-bit UUID | `le32(&ad.data[i])` at `ad_parser.cpp:102` — correct LE. Verified by `byte_order_test.cpp` Test 12 (discriminator). | PASS | 95 | None — correct |
| 9 | AD Appearance | `le16(&ad.data[0])` at `ad_parser.cpp:140` — correct LE. Verified by `byte_order_test.cpp` Test 13 (discriminator). | PASS | 95 | None — correct |
| 10 | AD CI min/max interval | `le16(&ad.data[0])` / `le16(&ad.data[2])` at `ad_parser.cpp:149-150` — correct LE. Verified by `byte_order_test.cpp` Tests 14-15 (discriminators). | PASS | 95 | None — correct |
| 11 | `le16()`/`le32()`/`be16()` helpers | All three `constexpr` functions in `types.h:273-326` correctly implement their respective byte orders. Verified by `le_helpers_test.cpp` (11 `static_assert`). | PASS | 95 | None — correct |
| 12 | Cross-validation evidence | No nRF52840 Sniffer or Ubertooth cross-validation for Sony/Razer byte-order fixes. ADR-0009 acknowledges risk and recommends manual testing with real devices. | Advisory | 65 | Track for future backfill; not blocking — pre-existing gap, documented risk |
| 13 | `be16()` edge-case tests | `le_helpers_test.cpp` missing zero/max/LSB-only `be16()` tests (only basic + MSB-only tested). Not a protocol issue — helper implementation is correct. | Advisory | 60 | Add `{0x00,0x00}→0x0000`, `{0xFF,0xFF}→0xFFFF`, `{0x01,0x00}→0x0100` in future backfill |
| 14 | `le32()` boundary tests | `le_helpers_test.cpp` missing LSB-only/MSB-only `le32()` tests. Not a protocol issue — helper implementation is correct. | Advisory | 60 | Add `{0x42,0x00,0x00,0x00}→0x00000042` and `{0x00,0x00,0x00,0x80}→0x80000000` in future backfill |

## Overall Assessment

**All protocol-level checks pass.** The implementation correctly follows every applicable specification:

1. **BLE Core Spec Vol 1 Part A §1 (little-endian):** All 13 call sites across 4 source files (`ad_parser.cpp`, `proprietary_parsers.cpp`, `stat_view.cpp`, `assigned_numbers.h`) correctly use `le16()`/`le32()` for BLE protocol fields. The three bugs identified in A1-WX (Samsung `device_type`, Sony `protocol_ver`, Razer `model`) are definitively fixed. No remaining inline byte-order expressions exist in the codebase (grep confirmed zero matches).

2. **Apple iBeacon specification (big-endian):** The iBeacon Major/Minor fields correctly use `be16()` — the vendor-specific exception to the BLE LE convention. The discriminator test (`byte_order_test.cpp` Test 9) explicitly verifies that the wrong LE interpretation (0x0102 → 513) is NOT present, and the correct BE interpretation (0x0102 → 258) IS present.

3. **AirPods battery nibble encoding:** The `format_battery` lambda correctly maps nibble 0-10 to 0%-100% and nibble 11-15 to "??" per ADR-0007 and ADR-0008. The raw nibble value is always preserved in parentheses. Truncated data paths (size < 7, size ≥ 7 but < 8) are correctly handled.

4. **AD structure parsing:** All 6 refactored call sites in `ad_parser.cpp` (Company ID, 16-bit UUID, 32-bit UUID, Appearance, CI min/max interval) correctly use `le16()`/`le32()`. Each has a discriminator test that explicitly verifies the wrong BE interpretation is NOT present.

5. **Test evidence:** 102 assertions across 3 test files, all passing. Fresh build: exit 0, zero warnings. All 3 test executables exit 0.

**No blocking findings (confidence ≥80).** The 3 advisory findings (cross-validation evidence, `be16()` edge-case tests, `le32()` boundary tests) are all pre-existing gaps or test coverage improvements — none affect protocol correctness.

**Verdict: APPROVED.**

## Self-Audit Checklist

| Category | Checked? | Finding or PASS |
|----------|----------|-----------------|
| Build passes (exit 0, no warnings) | yes | PASS — Fresh build: `cmake --build --preset conan-debug` → exit 0, zero warnings, all 5 targets built |
| Typed enums / vocabulary types (no raw integers in API) | yes | PASS — `le16()`/`le32()`/`be16()` take `const uint8_t*` (appropriate for byte-level utilities). All other public API uses typed enums. |
| Documentation on new public symbols | yes | PASS — `le16()`, `le32()`, `be16()` all have full Doxygen with `@brief`, `@param`, `@return`, `@warning`, `@note`, `@see`, `@example`. 14 symbols in `proprietary_parsers.h` have `@note` tags. |
| Spec/datasheet fidelity (fields match spec) | yes | PASS — BLE Core Spec Vol 1 Part A §1 cited for LE; Apple iBeacon spec cited for BE exception; furiousMAC/continuity cited for AirPods encoding. All byte-order choices verified against authoritative references. |
| Module boundary (no platform headers in shared modules) | yes | PASS — `types.h` includes only `<cstdint>`, `<string>`, `<string_view>`. No platform headers. |
| Reserved/padding fields handled | yes | N/A (PASS) — No register structs with reserved bits in changeset. `le16()`/`le32()`/`be16()` are pure byte-level reads. |
| No magic numbers in doc examples | yes | PASS — All hex values in `@code` blocks are test vectors with documented expected output. |
| Buffer safety (bounded copies) | yes | PASS — `le16()`/`le32()`/`be16()` read exactly 2/4 bytes. All 13 call sites bounds-check before calling (verified by code inspection). `@warning` Doxygen documents UB risk. |
| AGENTS.md compliance | yes | PASS — Review follows Phase C protocol, cites authoritative references, no code written. |
| Conventional commit ready | yes | PASS — No decision references, no changelog comments, no secrets in changeset. |

## Routing

Proceed to C3: C-GATE. All 4 dispatched specialists (SW, TX, DX, WX) must issue APPROVED before C-GATE can pass. WX has issued APPROVED.

## Self-Reflection

### Why was the Samsung/Sony/Razer byte order bug missed originally?

The bug pattern `(byte[0] << 8) | byte[1]` is the natural "human" way to write a 16-bit value — it matches how we write numbers (most significant digit first). The BLE Core Spec's little-endian rule (§1.3.1) is counterintuitive to human readers. The code was likely written by someone thinking in "network byte order" (big-endian) without checking the BLE spec. The iBeacon parser happened to be correct because Apple's iBeacon spec explicitly uses big-endian, and the comment on line 50 documented this awareness. The other vendor parsers lacked such documentation and fell into the natural-but-wrong big-endian pattern.

### What procedural safeguard would have caught it?

1. **A `le16()` helper function** — now implemented. If all 16-bit BLE field reads go through a named helper, the byte order convention is enforced at the API level rather than relying on each developer to remember the BLE spec.
2. **A Phase A protocol review checklist item** — "Verify all multi-byte field reads follow BLE little-endian convention per Core Spec Vol 1 Part A §1" should be a standard check for any code that parses BLE advertising data.
3. **Cross-validation with real device captures** — comparing parsed `device_type` values against known Samsung device type IDs would have revealed the byte swap. This remains a gap for Sony/Razer (ADR-0009).

### Knowledge base update

The `ble-protocol` skill already documents the little-endian convention and the iBeacon big-endian exception. The `le16()`/`le32()`/`be16()` helpers in `types.h` now encode this convention at the API level, making future byte-order bugs much less likely.
