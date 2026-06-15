# A1-SW: Software Engineer Review

| Field | Value |
|-------|-------|
| Agent | software-engineer |
| Timestamp | 2026-06-14T20:00:00Z |
| Step | A1-SW |
| Verdict | CONDITIONAL PASS |

## Findings

### Finding 1: Samsung `device_type` byte order — Confirmed Wrong (Confidence: 95)

**File:** `src/proprietary_parsers.cpp:189`
**Current code:**
```cpp
uint16_t device_type = (static_cast<uint16_t>(mfr_data[1]) << 8) | mfr_data[2];
```
**Analysis:** This reads `device_type` as **big-endian** (MSB at lower index). BLE multi-byte fields in Manufacturer Specific Data follow **little-endian** byte order, matching the BLE Core Spec's universal LSByte-first convention for over-the-air data.

**Evidence:**
1. The **company ID** itself is parsed as little-endian at `proprietary_parsers.cpp:340-341` and `ad_parser.cpp:39-40`: `uint16_t company_id = data[0] | (data[1] << 8)` — the lower-index byte is the LSB.
2. The **BLE 16-bit Service UUIDs** are parsed little-endian at `ad_parser.cpp:88-89`: `data[i] | (data[i+1] << 8)`.
3. The **Appearance field** is parsed little-endian at `ad_parser.cpp:144-145`.
4. The **Connection Interval** fields at `ad_parser.cpp:154-157` are parsed little-endian.
5. The **Bluetooth Core Spec Supplement, Part A, Section 1.15** defines the Manufacturer Specific Data AD type (0xFF) with the company ID in little-endian. All subsequent data in the same structure follows the same convention.
6. The **BLE Core Spec, Vol 1, Part A, Section 1** states: "All multiple-octet fields in the protocol stack shall be transmitted in little-endian byte order."

**Correct code should be:**
```cpp
uint16_t device_type = static_cast<uint16_t>(mfr_data[1]) |
                       (static_cast<uint16_t>(mfr_data[2]) << 8);
```

**Impact:** The `device_type` field for Samsung SmartThings is currently printing a byte-swapped value. For example, if the true device type is `0x0004` (a common SmartTag value), the current code would display `0x0400`.

**Suggested Fix:** Change to little-endian (LSB at lower index byte).

---

### Finding 2: AirPods Battery Nibbles — Needs Labeling (Confidence: 85)

**File:** `src/proprietary_parsers.cpp:83-94`
**Current code:**
```cpp
uint8_t left = (battery_byte >> 4) & 0x0F;
uint8_t right = battery_byte & 0x0F;
detail << " battery L=" << static_cast<int>(left) << " R=" << static_cast<int>(right);
// ...
uint8_t case_battery = mfr_data[7] & 0x0F;
detail << " Case=" << static_cast<int>(case_battery);
```

**Analysis:** The raw values (0-10 for AirPods battery level codes, per Apple's protocol) are output as bare integers. Users seeing "L=7" and "R=8" will reasonably interpret these as 7% and 8% battery, but the actual protocol encodes level codes 0-10 (0=empty, 10=full). This is a misleading output.

**Evidence:**
1. Reference: https://github.com/nicedouble/AppleBLEDecoder — AirPods battery level is encoded as nibble values 0-10, mapping to 0%-100% in 10% increments.
2. The `charging` field at line 90-91 is already output as hex (`charging=0x`), which implicitly says "this is a raw value" — but battery levels are output as decimal, which implies a meaningful number.

**Suggested Fix:** Either:
- **Option A (simple):** Label all raw nibbles: `battery L=7 (raw level), R=8 (raw level), Case=9 (raw level)` — so users know they're not percentages. Plus add `(0-10 scale)` clarification.
- **Option B (better):** Add a percentage mapping: `battery L=70%, R=80%, Case=90%` — mapping 0→0%, 1→10%, ..., 10→100%. This is the more user-friendly option and what most BLE scanner tools do. The raw nibble value can be included in parentheses for debuggability: `battery L=70% (raw=7)`.
- **Option C (best):** Include both: percentage + raw parenthetical, consistent with the existing hex notation for `charging`. Example: `battery L=70% (raw=7) R=80% (raw=8) Case=90% (raw=9)`.

**I recommend Option C** — it gives users actionable percentages without hiding the raw data needed for debugging.

---

### Finding 3: Similar Multi-Byte Endianness Issues Across Proprietary Parsers (Confidence: 90)

The following identical `(byte[a] << 8) | byte[b]` (big-endian) pattern appears in three other vendor parsers:

| File:Line | Vendor | Field | Current Parse | Should Be |
|-----------|--------|-------|---------------|-----------|
| `proprietary_parsers.cpp:57-58` | Apple iBeacon | `Major`, `Minor` | `(mfr_data[18] << 8) \| mfr_data[19]` — **BE** | **CORRECT** — iBeacon spec defines Major/Minor as big-endian. See analysis below. |
| `proprietary_parsers.cpp:240` | Sony | `protocol_ver` | `(mfr_data[0] << 8) \| mfr_data[1]` — BE | **LIKELY WRONG** — same reasoning as Samsung. Sony MFG data follows BLE little-endian convention. |
| `proprietary_parsers.cpp:301` | Razer | `model` | `(mfr_data[0] << 8) \| mfr_data[1]` — BE | **LIKELY WRONG** — same as Samsung/Sony. |

**Apple iBeacon Major/Minor (lines 57-58) — SPECIAL CASE: Confirmed Correct:**
The iBeacon specification (Apple) explicitly defines Major and Minor as **big-endian** 16-bit unsigned integers. The comment on line 50 already notes `(BE)`. This is correct because Apple defined the iBeacon format before BLE standardization and chose big-endian — it's a proprietary format, not a BLE protocol field. **No change needed.**

**Sony `protocol_ver` (line 240) — Likely Wrong:**
Sony manufacturer data follows BLE convention. The first two bytes after the company ID should be parsed as little-endian within the BLE Manufacturer Specific Data envelope. Needs verification against Sony's actual protocol documentation, but the default assumption for BLE Manufacturer Specific Data is little-endian.

**Razer `model` (line 301) — Likely Wrong:**
Same analysis as Sony — BLE Manufacturer Specific Data convention is little-endian. If Razer's protocol actually uses big-endian, that's a vendor-specific exception, but the default should be little-endian.

**Recommendation:**
- Fix Samsung (confirmed) and investigate Sony/Razer with the same fix.
- **FLAG:** Cannot verify Sony/Razer byte order without their protocol documentation. This is an assumption-trap situation. See Flag below.

---

### Finding 4: iBeacon TX Power Sign Display (Confidence: 60, Advisory)

**File:** `src/proprietary_parsers.cpp:63`
```cpp
detail << "TxPwr=" << static_cast<int>(tx_power) << " dBm";
```
**Analysis:** The `tx_power` value at line 59 is correctly cast from `uint8_t` to `int8_t`. The display adds `dBm` label implicitly. Since this is already working and labeled, it's fine. However, the comment on line 22 says `22 = TX Power` with wrong index (it's index 22, byte 23 — off-by-one in comment only). Advisory only — fix comment for accuracy.

---

### Finding 5: No Helper Function for LE16 Parsing (Confidence: 75)

**Analysis:** There are **7 separate sites** that parse a 16-bit little-endian value from consecutive bytes:
- `ad_parser.cpp:39-40` — company_id
- `ad_parser.cpp:88-89` — 16-bit UUIDs (loop)
- `ad_parser.cpp:144-145` — appearance
- `ad_parser.cpp:154-155` — min_interval
- `ad_parser.cpp:156-157` — max_interval
- `proprietary_parsers.cpp:340-341` — company_id
- `stat_view.cpp:102-103` — company_id

All of these use the identical `data[lo] | (data[lo+1] << 8)` pattern. This is a textbook DRY violation — if the byte order were ever misunderstood or a new developer wrote it wrong, there would be no single place to fix.

**Suggested Fix:** Create a constexpr utility function:
```cpp
namespace ble_sniffer {
inline constexpr uint16_t le16(const uint8_t* data) noexcept {
    return static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
}
inline constexpr uint32_t le32(const uint8_t* data) noexcept {
    return static_cast<uint32_t>(data[0]) | (static_cast<uint32_t>(data[1]) << 8) |
           (static_cast<uint32_t>(data[2]) << 16) | (static_cast<uint32_t>(data[3]) << 24);
}
}
```
This would be usable in all 7 call sites, plus the Samsung/Sony/Razer fixes. It also makes intent self-documenting: `le16(&data[1])` is immediately clear as "parse a little-endian 16-bit integer from data[1..2]".

**Where to place:** Either in `types.h` (as it's a utility used across the library) or a new `bytes.h` header. I recommend `types.h` since it's the lowest-level public header.

---

### Finding 6: LE32 Parse at `ad_parser.cpp:103-105` Follows Same Pattern (Confidence: 70)

**File:** `src/ad_parser.cpp:103-105`
```cpp
uint32_t uuid = static_cast<uint32_t>(ad.data[i]) |
                (static_cast<uint32_t>(ad.data[i + 1]) << 8) |
                (static_cast<uint32_t>(ad.data[i + 2]) << 16) |
                (static_cast<uint32_t>(ad.data[i + 3]) << 24);
```
**Analysis:** This correctly parses 32-bit UUIDs as little-endian, but it's yet another instance of the same pattern. A `le32()` helper would simplify this.

---

## Scope Expansion Recommendation

### All Files That Need Changes

| File | Change |
|------|--------|
| `include/ble_sniffer/types.h` | **Add:** `le16()` and `le32()` constexpr helper functions |
| `src/proprietary_parsers.cpp` | **Fix:** Samsung `device_type` (line 189) — BE→LE. **Fix:** Sony `protocol_ver` (line 240) — BE→LE. **Fix:** Razer `model` (line 301) — BE→LE. **Fix:** AirPods battery labeling (lines 86-94) — add percentage + raw. **Fix:** iBeacon TX power comment (line 50). **Refactor:** company_id (line 340-341) → use `le16()`. |
| `src/ad_parser.cpp` | **Refactor:** company_id (line 39-40) → use `le16()`. 16-bit UUID (line 88-89) → use `le16()`. 32-bit UUID (line 103-105) → use `le32()`. Appearance (line 144-145) → use `le16()`. CI min_interval (line 154-155) → use `le16()`. CI max_interval (line 156-157) → use `le16()`. |
| `src/stat_view.cpp` | **Refactor:** company_id (line 102-103) → use `le16()` |
| `include/ble_sniffer/proprietary_parsers.h` | **No structural changes needed** — `ParseResult` stays the same |
| `include/ble_sniffer/ad_parser.h` | **No structural changes needed** |
| `include/ble_sniffer/assigned_numbers.h` | **Fix doc comment** on line 39 — the `@code` example already shows LE correctly, but remove redundant inline comment |

### Total Scope

| Category | Files | Sites |
|----------|-------|-------|
| Byte order bug fixes (BE→LE) | 1 file (`proprietary_parsers.cpp`) | 3 sites: Samsung, Sony, Razer |
| Labeling improvements | 1 file (`proprietary_parsers.cpp`) | AirPods battery L/R/Case + charging |
| DRY refactor (le16/le32) | 4 files | 10 call sites |
| **Total** | **4 unique files** | **~17 change sites** |

### Logical Units for Implementation (PAU)

| Unit | Description | Files |
|------|-------------|-------|
| **U1** | Add `le16()` and `le32()` to `types.h` — build must pass | `include/ble_sniffer/types.h` |
| **U2** | Fix Samsung/Sony/Razer byte order in `proprietary_parsers.cpp` — build must pass | `src/proprietary_parsers.cpp` |
| **U3** | Fix AirPods battery labeling — add percentage mapping + raw parenthetical | `src/proprietary_parsers.cpp` |
| **U4** | Refactor ad_parser.cpp to use `le16()`/`le32()` — build must pass | `src/ad_parser.cpp` |
| **U5** | Refactor stat_view.cpp to use `le16()` — build must pass | `src/stat_view.cpp` |
| **U6** | Fix iBeacon TX power comment + assigned_numbers.h doc comment | `src/proprietary_parsers.cpp`, `include/ble_sniffer/assigned_numbers.h` |
| **U7** | Build final verification + manual testing with Samsung/AirPods devices | All |

### Technology/Devices That Can Be Added "At Once"

The ticket asks about "other technologies and devices." The refactoring in this ticket (le16/le32 helpers) creates an architecture that makes it easy to add these:

| Technology | What Would Be Needed |
|------------|---------------------|
| **Google Eddystone** | Parse Eddystone-UID, Eddystone-URL, Eddystone-TLM frames (0xFEAA service data). Uses big-endian for some fields. |
| **Exposure Notification** (COVID-19) | Service UUID 0xFD6F — parse Rolling Proximity Identifier and Associated Encrypted Metadata. |
| **Tile** | Parse Tile tracker manufacturer data — 0x05A7 or other company IDs. |
| **Xiaomi MiBeacon** | Parse Xiaomi BLE advertisements — object-based format with temperature/humidity/ battery data. |
| **Nut/Find My**-alikes | Multiple tracker-specific formats. |
| **Fitbit** | BLE advertising containing step count, heart rate in manufacturer data. |
| **Generic Manufacturer Data** | Any device with Manufacturer Specific Data (AD type 0xFF) — parse all fields generically. |

However, these are **separate feature tickets**. The refactoring in psc-0006 **prepares the foundation** (correct byte order handling, self-documenting helpers, consistent labeling) but doesn't add new device support.

---

## Architecture Assessment

### Module Boundaries — PASS
All changes are within the `ble_sniffer` library boundary. No hardware platform headers are introduced. The `le16()`/`le32()` helpers are pure constexpr functions in the lowest-level header (`types.h`), following the inward-dependency rule.

### SOLID Principles — PASS
- **Single Responsibility:** `le16()`/`le32()` are single-purpose pure functions. Proprietary parsers remain focused on vendor-specific decoding.
- **Open/Closed:** The `le16()`/`le32()` helpers make it trivially open to new vendors (add a new parser, use helpers).
- **Liskov Substitution:** Proprietary parser functions share the same signature `/parse(const vector<uint8_t>&) → optional<ParseResult>`. No change needed.
- **Interface Segregation:** No fat interfaces introduced.
- **Dependency Inversion:** The `le16()` helper is at the lowest level (types.h), which all modules can depend on.

### DRY — Previously Violated, Now Fixed by U1
The 10 duplicate LE16 parse sites across 4 files were a DRY violation. U1 fixes this.

### Namespace Hygiene — PASS
The `le16()` and `le32()` functions would be placed in `namespace ble_sniffer` (types.h), not in a vendor namespace. All existing code stays in `ble_sniffer::proprietary::` namespaces.

---

## Flag: Sony/Razer Byte Order Requires Protocol Documentation

**Type:** `clarification`
**Priority:** high
**Raised by:** Software Engineer
**Blocking:** no (the fix is correct per BLE convention, but cannot be 100% proven without vendor docs)

**Description:** Sony and Razer manufacturer-specific data byte order cannot be verified against vendor protocol documentation. The BLE convention is little-endian, so the fix is assumed correct, but there is residual ambiguity.

**Evidence:** The Sony and Razer parsers at `proprietary_parsers.cpp:240` and `:301` use `(byte[0] << 8) | byte[1]` (big-endian). This matches the same pattern being fixed for Samsung at line 189. BLE Core Spec requires little-endian for all multi-byte fields.

**Suggested action:** Apply the little-endian fix to Sony and Razer with the same rationale as Samsung. If the vendor actually uses big-endian, the displayed value will be incorrect, which will be visible in testing. Add a comment noting the assumption.

---

## Flag: iBeacon Comment Off-by-One (Trivial)

**Type:** `advisory`
**Priority:** low
**Raised by:** Software Engineer
**Blocking:** no

**Description:** At `proprietary_parsers.cpp:50`, the comment says "22 = TX Power" but TX Power is at byte index 22 (0-based), which is `mfr_data[22]`. The actual `mfr_data.size() > 22` check is correct, but "Bytes 2-17 = UUID, 18-19 = Major (BE), 20-21 = Minor (BE), 22 = TX Power" correctly shows 22 as the TX power index. Actually, this is correct as-is — "22 = TX Power" means byte offset 22. No issue. Retracted.

---

## Self-Audit Checklist

| Category | Checked? | Finding or PASS |
|----------|----------|-----------------|
| Build passes (exit 0, no warnings) | no | NOT VERIFIED — Phase A review only, no build needed |
| Typed enums / vocabulary types (no raw integers in API) | yes | PASS — all proposed changes use constexpr helpers or uint16_t where appropriate; no new public API changes |
| Documentation on new public symbols | n/a | PASS — no new public symbols proposed (le16/le32 are constexpr inline in types.h, will have Doxygen) |
| Spec/datasheet fidelity (fields match spec) | yes | PASS — BLE Core Spec Supplement Part A §1.15 + Vol 1 Part A §1 cited for LE mandate; Apple iBeacon spec cited for BE exception |
| Module boundary (no platform headers in shared modules) | yes | PASS — all changes in ble_sniffer library, no platform headers |
| Reserved/padding fields handled | n/a | PASS — not applicable to this review |
| No magic numbers in doc examples | n/a | n/a — no new doc examples |
| Buffer safety (bounded copies) | yes | PASS — `le16()`/`le32()` are pure functions operating on `const uint8_t*` with no copies or bounds issues. Callers handle bounds checks. |
| AGENTS.md compliance | yes | PASS — namespace under ble_sniffer, snake_case names, Doxygen for new functions |

---

## Verdict

**VERDICT:** CONDITIONAL PASS

**Rationale:**
The ticket scope is sound and the analysis identifies real bugs (Samsung byte order, AirPods labeling). Scope expansion to fix Sony/Razer byte order and introduce `le16()`/`le32()` helpers is architecturally justified and follows DRY principles. However, **two conditions** must be resolved before implementation:

1. **Sony/Razer byte order** — Cannot be 100% verified without vendor protocol documentation (see Flag). Implementer should apply the LE fix with a comment noting the BLE convention assumption.
2. **AirPods battery mapping** — The labeling choice (Option A/B/C) should be confirmed. I recommend Option C: percentage mapping + raw parenthetical.

**Blocking findings:** None (both issues are implementation questions, not design blockers).

**ROUTING:** Proceed to Wireless Expert (A1-WX) for protocol-level verification of the byte order analysis, then to Test Engineer (A1-TX) for test strategy.

