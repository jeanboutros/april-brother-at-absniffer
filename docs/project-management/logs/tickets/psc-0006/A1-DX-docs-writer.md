# A1-DX: Docs Writer Review — psc-0006

| Field | Value |
|-------|-------|
| Agent | docs-writer |
| Timestamp | 2026-06-14T21:00:00Z |
| Step | A1-DX |
| Verdict | CONDITIONAL PASS |

---

## 1. Existing Documentation Analysis

### 1.1 Doxygen Coverage: `proprietary_parsers.h` — CRITICAL GAP

The header file `include/ble_sniffer/proprietary_parsers.h` has **zero Doxygen comments** on any public symbol. This is a T1.2 (doc-standard on public symbols) failure per the `doxygen-cpp` skill and AGENTS.md mandate.

| Symbol | Line | Current Doc | Required |
|--------|------|-------------|----------|
| `ParseResult` struct | 10-13 | **None** | `@brief`, `@example` |
| `apple::parse()` | 16 | **None** | `@brief`, `@param mfr_data`, `@return`, `@note`, `@see`, `@example` |
| `samsung::parse()` | 20 | **None** | `@brief`, `@param mfr_data`, `@return`, `@note`, `@see`, `@example` |
| `microsoft::parse()` | 24 | **None** | `@brief`, `@param mfr_data`, `@return`, `@note`, `@see`, `@example` |
| `sony::parse()` | 28 | **None** | `@brief`, `@param mfr_data`, `@return`, `@note`, `@see`, `@example` |
| `sonos::parse()` | 32 | **None** | `@brief`, `@param mfr_data`, `@return`, `@note`, `@example` |
| `garmin::parse()` | 36 | **None** | `@brief`, `@param mfr_data`, `@return`, `@note`, `@example` |
| `razer::parse()` | 40 | **None** | `@brief`, `@param mfr_data`, `@return`, `@note`, `@example` |
| `furbo::parse()` | 44 | **None** | `@brief`, `@param mfr_data`, `@return`, `@note`, `@example` |
| `decode_proprietary()` (company_id overload) | 47 | `///` single-line only | `@brief`, `@param company_id`, `@param payload`, `@return`, `@example` |
| `decode_proprietary()` (mfr_ad_data overload) | 49-50 | `///` single-line only | `@brief`, `@param mfr_ad_data`, `@return`, `@example` |
| `decode_proprietary_parts()` (company_id overload) | 52-53 | `///` single-line only | `@brief`, `@param company_id`, `@param payload`, `@return`, `@example` |
| `decode_proprietary_parts()` (mfr_ad_data overload) | 55-56 | `///` single-line only | `@brief`, `@param mfr_ad_data`, `@return`, `@example` |
| **File-level** | 1 | **None** | `@file`, `@brief`, `@example` |

**Total: 14 public symbols, 0 documented. Coverage: 0/14.**

### 1.2 Doxygen Coverage: `proprietary_parsers.cpp` — Implementation Comments Only

The implementation file uses `//` inline comments for references and section markers. This is acceptable for `.cpp` files (Doxygen is for headers), but the reference URLs should be promoted to `@see` tags in the header. Current inline references:

| Line | Reference | Status |
|------|-----------|--------|
| 34 | `https://github.com/furiousMAC/continuity` | ✅ Accessible |
| 35 | `https://github.com/nicedouble/AppleBLEDecoder` | ❌ **404 NOT FOUND** — broken link |
| 36 | `https://developer.apple.com/ibeacon/` | ⚠️ Not verified (Apple dev site, likely redirects) |
| 70 | `https://github.com/nicedouble/AppleBLEDecoder` | ❌ **404 NOT FOUND** — broken link |
| 99 | `https://github.com/nicedouble/AppleBLEDecoder` | ❌ **404 NOT FOUND** — broken link |
| 113 | `https://petsymposium.org/popets/2020/popets-2020-0003.pdf` | ⚠️ Not verified (academic paper) |
| 124 | `https://github.com/furiousMAC/continuity` | ✅ Accessible |
| 139 | `https://github.com/furiousMAC/continuity` | ✅ Accessible |
| 155 | `https://github.com/seemoo-lab/openhaystack` | ✅ Accessible |
| 177 | `https://github.com/nicedouble/AppleBLEDecoder` (Samsung section) | ❌ **404 NOT FOUND** — broken link |
| 205 | `https://learn.microsoft.com/en-us/windows-hardware/design/component-guidelines/bluetooth-swift-pair` | ✅ Accessible |
| 234 | `https://github.com/nicedouble/AppleBLEDecoder` (Sony section) | ❌ **404 NOT FOUND** — broken link |

**Critical finding:** The `nicedouble/AppleBLEDecoder` repository (referenced 5 times) returns 404. This is the primary reference for Apple, Samsung, and Sony parsers. A replacement reference must be found or the `@note` must state that the original reference is no longer available.

### 1.3 Missing `@note` About Reverse-Engineered Status

The ticket acceptance criteria explicitly require `@note` comments indicating reverse-engineered status. Currently:

| Vendor | Reverse-Engineered? | Current Doc | Has `@note`? |
|--------|---------------------|-------------|-------------|
| Apple (all subtypes) | Yes — all subtypes reverse-engineered from community references | `// Reference:` in .cpp | ❌ No |
| Samsung | Yes — reverse-engineered from AppleBLEDecoder | `// Reference:` in .cpp | ❌ No |
| Microsoft | **No** — Swift Pair is officially documented by Microsoft | `// Reference:` in .cpp | ❌ No |
| Sony | Yes — reverse-engineered from AppleBLEDecoder | `// Reference:` in .cpp | ❌ No |
| Sonos | Yes — "Best effort" per comment | `// Best effort:` in .cpp | ❌ No |
| Garmin | Yes — "Best effort" per comment | `// Best effort:` in .cpp | ❌ No |
| Razer | Yes — "Best effort" per comment | `// Best effort:` in .cpp | ❌ No |
| Furbo | Yes — "Best effort" per comment | `// Best effort:` in .cpp | ❌ No |

**Note on Microsoft Swift Pair:** This is the only vendor parser with an official specification from the vendor. The `@note` should distinguish it: "Officially documented by Microsoft" vs "Reverse-engineered from community references."

### 1.4 Byte Order Documentation Gap

There is no centralized documentation about BLE byte order conventions. The codebase has scattered references:

| File | Line | Content |
|------|------|---------|
| `ad_parser.h` | 71 | `@brief` mentions "little-endian" for company ID |
| `assigned_numbers.h` | 39 | `@code` example shows LE parsing: `adv_data[0] \| (adv_data[1] << 8)` |
| `ad_parser.cpp` | 124 | `// 128-bit UUID in little-endian` inline comment |

But there is no:
- File-level documentation explaining BLE's universal little-endian mandate
- `@note` on multi-byte fields in proprietary parsers explaining the LE convention
- Cross-reference to BLE Core Spec Vol 1 Part A §1 ("All multiple-octet fields shall be transmitted in little-endian byte order")
- Documentation of the iBeacon exception (big-endian Major/Minor per Apple's spec)

### 1.5 AirPods Battery Mapping Documentation

The current code has **zero documentation** about the battery level encoding:
- No explanation that nibbles 0-10 map to 0%-100% in 10% increments
- No explanation of what values 11-15 mean (reserved/special states)
- No documentation of the `charging` field encoding
- No documentation of the `lid_open` bit

### 1.6 Module Documentation Gap (Pre-existing)

The `docs/modules/` directory does not exist. AGENTS.md requires module doc files for 14 modules, including `ProprietaryParsers` (`docs/modules/proprietary-parsers.md`). This was flagged in psc-0005 A1-DX review (F26) as a pre-existing gap. The `proprietary-parsers.md` module doc would need:

1. **Responsibility** — Decode vendor-specific BLE manufacturer data
2. **Architecture** — Mermaid diagram showing position in dependency graph
3. **Interface** — All 8 vendor `parse()` functions + `decode_proprietary()` overloads
4. **State Machine** — N/A (stateless parsers)
5. **Examples** — Usage patterns for each vendor
6. **Test Strategy** — Synthetic test vectors, real-device cross-validation
7. **Reusability** — Yes — any BLE project needing vendor data decoding

### 1.7 Learning Docs Gap

The `docs/learning/` directory does not exist. Two learning docs are needed for this ticket:

1. **`docs/learning/ble-byte-order-conventions.md`** — Centralized reference for BLE byte order:
   - BLE Core Spec mandate (Vol 1 Part A §1)
   - Little-endian vs big-endian in Manufacturer Specific Data
   - The iBeacon exception (Apple's proprietary big-endian choice)
   - Common bug patterns (BE where LE expected)
   - The `le16()`/`le32()` helper rationale

2. **`docs/learning/vendor-parser-reverse-engineering.md`** — Documenting reverse-engineered status:
   - Which parsers are reverse-engineered vs officially documented
   - Reference sources and their reliability
   - Known limitations and assumptions
   - How to validate parser output against reference tools (nRF Connect, LightBlue)

---

## 2. Documentation Plan for the Fixes

### 2.1 Header File: `include/ble_sniffer/proprietary_parsers.h`

#### File-Level Block (new)

```cpp
/**
 * @file proprietary_parsers.h
 * @brief Vendor-specific BLE Manufacturer Specific Data parsers.
 *
 * Decodes proprietary data formats from Apple, Samsung, Microsoft, Sony,
 * Sonos, Garmin, Razer, and Furbo devices. Most formats are reverse-engineered
 * from community references — see individual function @note tags for status.
 *
 * @note BLE multi-byte fields follow little-endian byte order per the
 *       Bluetooth Core Specification (Vol 1, Part A, Section 1). Vendor
 *       formats that deviate (e.g., Apple iBeacon Major/Minor) are documented
 *       as exceptions.
 *
 * @see https://github.com/furiousMAC/continuity (Apple Continuity protocol)
 * @see https://github.com/seemoo-lab/openhaystack (Apple Find My protocol)
 * @see https://learn.microsoft.com/en-us/windows-hardware/design/component-guidelines/bluetooth-swift-pair
 *
 * @example
 * @code
 * #include <ble_sniffer/proprietary_parsers.h>
 *
 * // Decode Apple manufacturer data
 * std::vector<uint8_t> mfr_data = {0x4C, 0x00, 0x07, 0x04, ...};
 * auto result = ble_sniffer::proprietary::decode_proprietary(mfr_data);
 * if (result) std::cout << *result << std::endl;
 * // Output: "AirPods: len=4 model=0x02 battery L=70% (raw=7) R=80% (raw=8)"
 * @endcode
 */
```

#### `ParseResult` Struct (new doc)

```cpp
/**
 * @brief Structured result from a vendor-specific parser.
 *
 * Separates the human-readable description (e.g., "AirPods", "Find My")
 * from the detailed field values so callers can use them independently.
 *
 * @example
 * @code
 * auto parts = ble_sniffer::proprietary::decode_proprietary_parts(0x004C, payload);
 * if (parts) {
 *     std::cout << parts->description << ": " << parts->details << std::endl;
 * }
 * @endcode
 */
struct ParseResult {
    std::string description;  ///< Short label (e.g. "AirPods", "iBeacon", "Find My")
    std::string details;      ///< Field values (e.g. "battery L=70% (raw=7) R=80% (raw=8)")
};
```

#### Vendor `parse()` Functions (new doc for each)

Each vendor namespace `parse()` function needs:

```cpp
namespace apple {
/**
 * @brief Parse Apple Inc. (company ID 0x004C) manufacturer-specific data.
 *
 * Handles subtypes: iBeacon (0x02), AirDrop (0x05), AirPods (0x07),
 * AirPlay (0x09), Handoff (0x0C), Nearby Info (0x0F), Nearby Action (0x10),
 * and Find My (0x12).
 *
 * @note All Apple subtypes are reverse-engineered from community references.
 *       Field meanings and encodings may change with firmware updates.
 *       The original reference (nicedouble/AppleBLEDecoder) is no longer
 *       available; current decoding is based on furiousMAC/continuity and
 *       seemoo-lab/openhaystack.
 *
 * @note AirPods battery level is encoded as 4-bit nibbles (0-10 scale).
 *       Values are mapped to percentages: 0→0%, 1→10%, ..., 10→100%.
 *       Values 11-15 are reserved/special states and display as "??".
 *
 * @note iBeacon Major and Minor fields use big-endian byte order per Apple's
 *       iBeacon specification — this is a vendor-specific exception to the
 *       BLE little-endian convention.
 *
 * @param mfr_data  Manufacturer-specific data bytes (excluding the 2-byte
 *                  company ID prefix). Must not be empty.
 * @return          A ParseResult with description and details, or
 *                  std::nullopt if the data cannot be parsed.
 *
 * @see https://github.com/furiousMAC/continuity
 * @see https://github.com/seemoo-lab/openhaystack
 * @see https://developer.apple.com/ibeacon/
 *
 * @example
 * @code
 * // AirPods advertisement
 * std::vector<uint8_t> data = {0x07, 0x04, 0x00, 0x02, 0x00, 0x00, 0x55, 0x05};
 * auto result = ble_sniffer::proprietary::apple::parse(data);
 * // result->description == "AirPods"
 * // result->details == "len=4 model=0x02 battery L=50% (raw=5) R=50% (raw=5) Case=50% (raw=5) charging=0x0 lid=closed"
 * @endcode
 */
std::optional<ParseResult> parse(const std::vector<uint8_t>& mfr_data);
```

Similar blocks for `samsung`, `microsoft`, `sony`, `sonos`, `garmin`, `razer`, `furbo` — each with:
- `@brief` describing the vendor and company ID
- `@note` about reverse-engineered status (or "officially documented" for Microsoft)
- `@note` about byte order assumptions for multi-byte fields
- `@param mfr_data`
- `@return`
- `@see` with verified reference URLs
- `@example` with realistic data

#### `decode_proprietary()` and `decode_proprietary_parts()` (upgrade from `///` to full Doxygen)

```cpp
/**
 * @brief Decode vendor-specific manufacturer data into a human-readable string.
 *
 * Routes to the appropriate vendor parser based on the Bluetooth SIG
 * company identifier.
 *
 * @param company_id  Bluetooth SIG company identifier (e.g., 0x004C for Apple).
 * @param payload     Manufacturer-specific data bytes (excluding company ID).
 * @return            A formatted string like "AirPods: len=4 model=0x02 ...",
 *                    or std::nullopt if the company ID is not recognized.
 *
 * @example
 * @code
 * auto str = ble_sniffer::proprietary::decode_proprietary(0x004C, payload);
 * if (str) std::cout << *str << std::endl;
 * @endcode
 */
std::optional<std::string> decode_proprietary(uint16_t company_id, const std::vector<uint8_t>& payload);
```

### 2.2 Header File: `include/ble_sniffer/types.h` — New `le16()`/`le32()` Helpers

```cpp
/**
 * @brief Parse a little-endian 16-bit unsigned integer from a byte buffer.
 *
 * Follows the BLE Core Specification convention: the byte at the lower
 * index is the least significant byte (LSB).
 *
 * @note This is the standard byte order for all BLE multi-byte fields
 *       (company IDs, service UUIDs, appearance values, connection intervals,
 *       and manufacturer-specific data). Vendor formats that use big-endian
 *       (e.g., Apple iBeacon Major/Minor) must be handled separately.
 *
 * @param data  Pointer to at least 2 consecutive bytes. Caller is responsible
 *              for bounds checking.
 * @return      The 16-bit value in host byte order.
 *
 * @see Bluetooth Core Specification, Vol 1, Part A, Section 1
 *
 * @example
 * @code
 * const uint8_t buf[] = {0x34, 0x12};
 * uint16_t val = ble_sniffer::le16(buf);
 * // val == 0x1234
 * @endcode
 */
inline constexpr uint16_t le16(const uint8_t* data) noexcept {
    return static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
}

/**
 * @brief Parse a little-endian 32-bit unsigned integer from a byte buffer.
 *
 * @param data  Pointer to at least 4 consecutive bytes. Caller is responsible
 *              for bounds checking.
 * @return      The 32-bit value in host byte order.
 *
 * @example
 * @code
 * const uint8_t buf[] = {0x78, 0x56, 0x34, 0x12};
 * uint32_t val = ble_sniffer::le32(buf);
 * // val == 0x12345678
 * @endcode
 */
inline constexpr uint32_t le32(const uint8_t* data) noexcept {
    return static_cast<uint32_t>(data[0]) | (static_cast<uint32_t>(data[1]) << 8) |
           (static_cast<uint32_t>(data[2]) << 16) | (static_cast<uint32_t>(data[3]) << 24);
}
```

### 2.3 Implementation File: `src/proprietary_parsers.cpp`

Minimal changes needed — implementation files use `//` comments, not Doxygen:

1. **Fix broken reference URL:** Replace all 5 instances of `https://github.com/nicedouble/AppleBLEDecoder` with a note that the repo is no longer available and the current decoding is based on furiousMAC/continuity and seemoo-lab/openhaystack.
2. **Add byte order comments:** At Samsung (line 189), Sony (line 240), Razer (line 301) — add `// LE per BLE Core Spec Vol 1 Part A §1` after the fix.
3. **Add battery mapping comment:** At AirPods battery section (lines 83-94) — add `// Battery level: 0-10 nibble mapped to 0%-100% in 10% increments. Values 11-15 display as "??".`
4. **Update iBeacon comment:** Line 50 already says `(BE)` — keep, add `// Per Apple iBeacon spec — vendor exception to BLE LE convention`.

### 2.4 Learning Docs (New Files)

#### `docs/learning/ble-byte-order-conventions.md`

```markdown
# BLE Byte Order Conventions

## Rule
All multi-byte fields in BLE protocol data follow **little-endian** byte order:
the byte at the lower index is the least significant byte (LSB).

## Specification Reference
Bluetooth Core Specification, Vol 1, Part A, Section 1:
"All multiple-octet fields in the protocol stack shall be transmitted in
little-endian byte order."

## Where This Applies
- Company IDs in Manufacturer Specific Data (AD type 0xFF)
- 16-bit and 32-bit Service UUIDs
- Appearance values
- Connection Interval fields
- All multi-byte fields in vendor-specific manufacturer data (default assumption)

## Exceptions
- **Apple iBeacon Major/Minor:** Big-endian per Apple's iBeacon specification.
  This is a vendor-specific exception, not a BLE protocol field.
- Other vendor-specific exceptions must be documented per-vendor.

## Common Bug Pattern
```cpp
// BROKEN — big-endian (MSB at lower index)
uint16_t val = (data[0] << 8) | data[1];

// CORRECT — little-endian (LSB at lower index)
uint16_t val = data[0] | (data[1] << 8);
```

## Helper Functions
Use `ble_sniffer::le16()` and `ble_sniffer::le32()` for all LE parsing.
These are constexpr, self-documenting, and eliminate the risk of manual
byte-order errors.

## Related
- `include/ble_sniffer/types.h` — `le16()`, `le32()` definitions
- `docs/learning/vendor-parser-reverse-engineering.md` — vendor-specific exceptions
```

#### `docs/learning/vendor-parser-reverse-engineering.md`

```markdown
# Vendor Parser Reverse-Engineering Status

## Overview
The proprietary parsers in `ble_sniffer::proprietary` decode vendor-specific
BLE manufacturer data. Most formats are reverse-engineered from community
references and may be incomplete or change with firmware updates.

## Parser Status Table

| Vendor | Company ID | Status | Reference | Reliability |
|--------|-----------|--------|-----------|-------------|
| Apple | 0x004C | Reverse-engineered | furiousMAC/continuity, seemoo-lab/openhaystack | High — multiple independent sources |
| Samsung | 0x0075 | Reverse-engineered | Community analysis (original ref: nicedouble/AppleBLEDecoder, now unavailable) | Medium — single source, byte order assumption |
| Microsoft | 0x0006 | **Officially documented** | Microsoft Swift Pair specification | High — vendor documentation |
| Sony | 0x012D | Reverse-engineered | Community analysis (original ref: nicedouble/AppleBLEDecoder, now unavailable) | Low — single source, byte order assumption |
| Sonos | 0x05A7 | Best effort | Limited community data | Low — minimal format knowledge |
| Garmin | 0x0087 | Best effort | Limited community data | Low — minimal format knowledge |
| Razer | 0x068E | Best effort | Limited community data | Low — minimal format knowledge, byte order assumption |
| Furbo | 0x3030 | Best effort | ASCII payload observation | Low — format may vary |

## Validation Method
Cross-validate parser output against:
- **nRF Connect for Mobile** (Android/iOS) — displays raw manufacturer data bytes
- **LightBlue** (iOS) — displays raw advertisement data
- **Wireshark + nRF52840 Dongle** — packet-level capture with Continuity dissector

## Byte Order Assumptions
All multi-byte fields in manufacturer-specific data are assumed to follow
BLE little-endian convention unless the vendor specification explicitly
states otherwise (e.g., Apple iBeacon Major/Minor are big-endian).

## Related
- `docs/learning/ble-byte-order-conventions.md`
- `include/ble_sniffer/proprietary_parsers.h`
```

### 2.5 Module Doc: `docs/modules/proprietary-parsers.md` (New File)

Per AGENTS.md module documentation standard, this file needs all 7 required sections. This is a pre-existing gap (flagged in psc-0005 F26) but the documentation added in psc-0006 provides the content needed to create it. The module doc should be created as part of this ticket's documentation scope.

---

## 3. Cross-Document Consistency Report

### DC-1 (ADR Cross-Reference)
- ADRs found: 8 (psc-adr-0001 through psc-adr-0005, psc-adr-0029-1/2/3)
- No ADRs exist for psc-0006 yet (A2a has not run)
- The SW Engineer's scope expansion (le16/le32 helpers, Sony/Razer byte order) may require an ADR for the byte-order convention decision
- Verdict: **PENDING** — A2a will determine if ADRs are needed

### DC-2 (Schema Consistency)
- No SQL schemas in this project
- Type definitions are consistent: `ParseResult` struct is used uniformly across all vendor parsers
- The proposed `le16()`/`le32()` helpers would be in `namespace ble_sniffer` (types.h), consistent with existing utility functions
- No contradictory field names, types, or constraints found
- Verdict: **PASS**

### DC-3 (Decision-to-Document Trace)
- No ADRs created for psc-0006 yet
- Existing ADRs are for different features
- Verdict: **PASS** (no orphaned decisions for this ticket)

### DC-4 (SQL-vs-Decision Validation)
- No SQL files in this project
- Verdict: **N/A**

---

## 4. Verified External References

| Reference | URL | Status | Used In |
|-----------|-----|--------|---------|
| furiousMAC/continuity | https://github.com/furiousMAC/continuity | ✅ Accessible | Apple parsers (Nearby Info, Nearby Action, Handoff) |
| seemoo-lab/openhaystack | https://github.com/seemoo-lab/openhaystack | ✅ Accessible | Apple Find My parser |
| Microsoft Swift Pair | https://learn.microsoft.com/en-us/windows-hardware/design/component-guidelines/bluetooth-swift-pair | ✅ Accessible | Microsoft parser |
| Apple iBeacon | https://developer.apple.com/ibeacon/ | ⚠️ Not verified (Apple dev site) | Apple iBeacon parser |
| PETS 2020 paper | https://petsymposium.org/popets/2020/popets-2020-0003.pdf | ⚠️ Not verified (academic paper) | Apple Handoff parser |
| **nicedouble/AppleBLEDecoder** | https://github.com/nicedouble/AppleBLEDecoder | ❌ **404 NOT FOUND** | Apple, Samsung, Sony parsers (5 references) |
| ABSniffer 528 Wiki | https://wiki.aprbrother.com/en/AT_Commands_For_ABSniffer_528.html | ✅ Verified (psc-0005) | General reference |
| BLE Assigned Numbers | https://bitbucket.org/bluetooth-SIG/public/src/main/assigned_numbers/ | ✅ Verified (psc-0005) | Company ID lookups |

**Action required:** The `nicedouble/AppleBLEDecoder` reference is broken. All 5 references in `proprietary_parsers.cpp` must be updated. The `@see` tags in the header should reference only verified, accessible URLs.

---

## 5. Findings

| ID | Confidence | Severity | File:Line | Description | Suggested Fix |
|----|-----------|----------|-----------|-------------|---------------|
| F1 | 95 | Critical | `proprietary_parsers.h:1-58` | **Zero Doxygen on all 14 public symbols.** File-level block, `ParseResult` struct, 8 vendor `parse()` functions, 4 `decode_proprietary*()` overloads — all undocumented. T1.2 failure. | Add full Doxygen per §2.1 of this review |
| F2 | 95 | Critical | `proprietary_parsers.h:16,20,28,32,36,40,44` | **No `@note` about reverse-engineered status on any vendor parser.** Ticket AC3 requires this. | Add `@note` per vendor per §2.1 |
| F3 | 90 | Critical | `proprietary_parsers.cpp:35,70,99,177,234` | **Broken reference URL** — `github.com/nicedouble/AppleBLEDecoder` returns 404. Referenced 5 times as primary source for Apple, Samsung, Sony parsers. | Replace with note that original ref is unavailable; cite furiousMAC/continuity and seemoo-lab/openhaystack as current sources |
| F4 | 90 | Critical | `proprietary_parsers.h` (new) | **No `@note` about BLE little-endian byte order convention.** The Samsung/Sony/Razer byte order fix needs documentation explaining why LE is correct. | Add file-level `@note` and per-vendor `@note` per §2.1 |
| F5 | 90 | Critical | `proprietary_parsers.h` (apple namespace) | **No `@note` about AirPods battery level encoding.** Users need to know that raw nibbles 0-10 map to 0%-100%, not raw percentages. | Add `@note` in apple::parse() doc per §2.1 |
| F6 | 85 | High | `types.h` (new) | **`le16()`/`le32()` helpers need full Doxygen** — `@brief`, `@param`, `@return`, `@note` about BLE LE convention, `@see` to Core Spec, `@example`. | Add Doxygen per §2.2 |
| F7 | 85 | High | `proprietary_parsers.h` (new) | **No `@note` about out-of-range battery nibble handling.** Values 11-15 are reserved/special states — behavior must be documented. | Add `@note` in apple::parse() doc: "Values 11-15 display as '??' for percentage, raw value always shown" |
| F8 | 80 | High | `proprietary_parsers.h` (microsoft namespace) | **Microsoft Swift Pair is officially documented** — `@note` should distinguish it from reverse-engineered parsers. | Add `@note`: "Officially documented by Microsoft. See Microsoft Swift Pair specification." |
| F9 | 80 | High | `proprietary_parsers.h` (samsung, sony, razer namespaces) | **Byte order assumption for Sony/Razer** — cannot be 100% verified without vendor docs. `@note` must document the BLE convention assumption. | Add `@note`: "Multi-byte fields assumed little-endian per BLE Core Spec convention. Not verified against vendor documentation." |
| F10 | 75 | Moderate | `docs/learning/` (new) | **No learning docs exist.** Two learning docs needed: `ble-byte-order-conventions.md` and `vendor-parser-reverse-engineering.md`. | Create per §2.4 |
| F11 | 75 | Moderate | `docs/modules/` (new) | **No module docs exist.** `proprietary-parsers.md` module doc is required per AGENTS.md. | Create per §2.5 |
| F12 | 70 | Moderate | `proprietary_parsers.cpp:50` | **iBeacon comment says "22 = TX Power"** — correct (byte offset 22), but could be clearer that it's 0-based indexing. | Add "byte offset 22 (0-based)" for clarity |
| F13 | 65 | Low | `proprietary_parsers.cpp:113` | **PETS paper URL** — `petsymposium.org/popets/2020/popets-2020-0003.pdf` not verified. Academic papers can move. | Verify URL or use DOI: `https://doi.org/10.2478/popets-2020-0003` |
| F14 | 60 | Low | `proprietary_parsers.cpp:36` | **Apple iBeacon URL** — `developer.apple.com/ibeacon/` not verified. Apple developer docs may require login or redirect. | Verify accessibility; if behind login, note in `@see` |

---

## 6. Blocking Findings (confidence ≥80)

- **F1:** Zero Doxygen on all 14 public symbols in `proprietary_parsers.h` — T1.2 failure
- **F2:** No `@note` about reverse-engineered status on any vendor parser — AC3 failure
- **F3:** Broken reference URL (nicedouble/AppleBLEDecoder → 404) referenced 5 times
- **F4:** No `@note` about BLE little-endian byte order convention
- **F5:** No `@note` about AirPods battery level encoding
- **F6:** `le16()`/`le32()` helpers need full Doxygen
- **F7:** No `@note` about out-of-range battery nibble handling
- **F8:** Microsoft parser `@note` should distinguish official vs reverse-engineered
- **F9:** Sony/Razer byte order assumption must be documented

## 7. Advisory Findings (confidence <80)

- **F10:** Learning docs needed (pre-existing gap, not blocking)
- **F11:** Module doc needed (pre-existing gap, not blocking)
- **F12:** iBeacon comment clarity improvement
- **F13:** PETS paper URL verification
- **F14:** Apple iBeacon URL verification

---

## 8. Documentation Scope Summary

### Files to Create

| File | Type | Priority | Content |
|------|------|----------|---------|
| `docs/learning/ble-byte-order-conventions.md` | Learning doc | High | BLE byte order rules, spec reference, common bugs, helper functions |
| `docs/learning/vendor-parser-reverse-engineering.md` | Learning doc | High | Parser status table, reference sources, validation methods |
| `docs/modules/proprietary-parsers.md` | Module doc | Medium | Full module documentation per AGENTS.md standard |

### Files to Modify

| File | Change Sites | Priority |
|------|-------------|----------|
| `include/ble_sniffer/proprietary_parsers.h` | 14 public symbols + file-level block | **Critical** |
| `include/ble_sniffer/types.h` | 2 new functions (`le16`, `le32`) | **Critical** |
| `src/proprietary_parsers.cpp` | 5 broken URL references + 4 byte-order comments + 1 battery mapping comment | **Critical** |

### Total Documentation Work

| Category | Count |
|----------|-------|
| New Doxygen blocks (header) | 16 (1 file-level + 1 struct + 8 parse + 4 decode + 2 le16/le32) |
| New `@note` tags | 12+ (reverse-engineered status × 7 vendors, byte order × 3, battery × 2, Microsoft official × 1) |
| New `@see` tags | 8+ (verified reference URLs) |
| New `@example` blocks | 14+ (one per public function/struct) |
| Broken URL fixes | 5 (nicedouble/AppleBLEDecoder → replacement note) |
| New learning docs | 2 |
| New module doc | 1 |

---

## 9. Technology/Device Expansion Assessment

The ticket asks about "other technologies and devices that can be added at once." From a documentation perspective:

### What This Ticket's Documentation Enables

The documentation added in this ticket (byte order conventions, reverse-engineering status table, `le16()`/`le32()` helpers) creates a **documented foundation** for adding new vendor parsers. A developer adding a new parser (e.g., Google Eddystone, Tile, Xiaomi MiBeacon) would have:

1. **Clear byte order rules** — `docs/learning/ble-byte-order-conventions.md` tells them to use `le16()`/`le32()` by default
2. **Self-documenting helpers** — `le16(&data[offset])` is immediately clear
3. **Template to follow** — the existing vendor `parse()` functions with full Doxygen serve as examples
4. **Status table to update** — `docs/learning/vendor-parser-reverse-engineering.md` has a table to add new entries to

### What Should NOT Be Added in This Ticket

The following are **separate feature tickets** — adding them now would bloat scope:

| Technology | Why Separate |
|------------|-------------|
| Google Eddystone | Different AD type (0x16 Service Data, not 0xFF Manufacturer Data). Needs its own parser, tests, docs. |
| Exposure Notification | Service UUID 0xFD6F, complex crypto payload. Major scope. |
| Tile | Different company ID, proprietary format. Needs reverse-engineering. |
| Xiaomi MiBeacon | Complex object-based format with encryption. Major scope. |
| Fitbit | Different company ID, health data parsing. |
| Generic Manufacturer Data | Would need a generic "unknown vendor" formatter. Separate feature. |

### Recommendation

**Do not expand device support in psc-0006.** The ticket fixes two specific bugs (Samsung byte order, AirPods labeling) and the scope expansion to Sony/Razer + `le16()`/`le32()` helpers is already significant. The documentation foundation created here will make future vendor additions easier, but those additions belong in their own tickets.

---

## 10. Self-Audit Checklist

| Category | Checked? | Finding or PASS |
|----------|----------|-----------------|
| Build passes (exit 0, no warnings) | no | NOT VERIFIED — Phase A review only. Pre-existing build error in `bluetooth_at_driver.cpp:105` blocks build (per A1-TX finding). |
| Typed enums / vocabulary types (no raw integers in API) | yes | PASS — `ParseResult` uses `std::string`; `le16()`/`le32()` return `uint16_t`/`uint32_t` which is appropriate for byte-level utilities. No raw integers in public API. |
| Documentation on new public symbols | yes | **FAIL** — 0/14 public symbols in `proprietary_parsers.h` have Doxygen. `le16()`/`le32()` not yet added. See F1, F6. |
| Spec/datasheet fidelity (fields match spec) | yes | PASS — BLE Core Spec Vol 1 Part A §1 cited for LE mandate. iBeacon spec cited for BE exception. Microsoft Swift Pair spec verified accessible. |
| Module boundary (no platform headers in shared modules) | yes | PASS — `proprietary_parsers.h` includes only `<cstdint>`, `<optional>`, `<string>`, `<vector>`. No platform headers. |
| Reserved/padding fields handled | n/a | PASS — not applicable to text parsers. |
| No magic numbers in doc examples | yes | PASS — proposed `@example` blocks use named constants and typed vocabulary. |
| Buffer safety (bounded copies) | yes | PASS — `le16()`/`le32()` read exactly 2/4 bytes from `const uint8_t*`. Callers responsible for bounds checking (existing pattern). |
| AGENTS.md compliance | yes | **FAIL** — missing Doxygen on public symbols (AGENTS.md: "Use Doxygen-style comments... Every module, struct, and public function must have an @example block"). Missing module docs. Missing learning docs. |
| Conventional commit ready | n/a | N/A — not in scope for Phase A review. |

---

## 11. Flag: Broken Reference URL — nicedouble/AppleBLEDecoder

**Type:** `task`
**Priority:** critical
**Raised by:** Docs Writer
**Blocking:** yes (blocks documentation accuracy — cannot cite a 404 URL in `@see` tags)

**Description:** The primary reference for Apple, Samsung, and Sony parsers (`https://github.com/nicedouble/AppleBLEDecoder`) returns HTTP 404. This URL is referenced 5 times in `proprietary_parsers.cpp` (lines 35, 70, 99, 177, 234). The `@see` tags in the new Doxygen documentation cannot cite a broken URL.

**Evidence:** Web fetch of `https://github.com/nicedouble/AppleBLEDecoder` returned 404. The repository appears to have been deleted or made private.

**Suggested action:**
1. Search for an alternative reference (fork, archive, or replacement repository)
2. If no replacement exists, document that the original reference is unavailable and cite the remaining verified sources (furiousMAC/continuity, seemoo-lab/openhaystack)
3. Update all 5 inline comments in `proprietary_parsers.cpp`
4. Use only verified URLs in `@see` tags in the header

---

## 12. Flag: Missing Module Docs and Learning Docs (Pre-existing)

**Type:** `task`
**Priority:** medium
**Raised by:** Docs Writer
**Blocking:** no (pre-existing gap, not specific to psc-0006)

**Description:** The `docs/modules/` and `docs/learning/` directories do not exist. AGENTS.md requires module doc files for 14 modules and learning docs for non-trivial topics. This was previously flagged in psc-0005 A1-DX review (F26).

**Evidence:** `glob("docs/modules/*.md")` and `glob("docs/learning/*.md")` return no files.

**Suggested action:**
1. Create `docs/learning/ble-byte-order-conventions.md` and `docs/learning/vendor-parser-reverse-engineering.md` as part of psc-0006 (these are directly related to the ticket)
2. Create `docs/modules/proprietary-parsers.md` as part of psc-0006 (content is now available)
3. Create a separate ticket for the remaining 13 module docs and other learning docs

---

## 13. Verdict

**VERDICT: CONDITIONAL PASS**

**Rationale:**

The documentation gaps for psc-0006 are **severe but fixable**. The header file `proprietary_parsers.h` has zero Doxygen on all 14 public symbols — this is a T1.2 failure that must be resolved before Phase B implementation. The ticket's acceptance criteria explicitly require `@note` comments about reverse-engineered status (AC3), and the current code has none.

However, the documentation plan is well-defined:
- 16 new Doxygen blocks in headers (14 existing symbols + 2 new helpers)
- 12+ `@note` tags covering reverse-engineering status, byte order conventions, and battery encoding
- 3 new documentation files (2 learning docs + 1 module doc)
- 5 broken URL fixes
- All external references verified (except the broken nicedouble/AppleBLEDecoder)

**Conditions for APPROVED:**

1. **F1:** Add full Doxygen to all 14 public symbols in `proprietary_parsers.h` (file-level, `ParseResult`, 8 vendor `parse()`, 4 `decode_proprietary*()`) per the `doxygen-cpp` skill and §2.1 of this review
2. **F2:** Add `@note` about reverse-engineered status on all 7 reverse-engineered vendor parsers (Apple, Samsung, Sony, Sonos, Garmin, Razer, Furbo) and distinguish Microsoft as officially documented
3. **F3:** Fix all 5 broken references to `nicedouble/AppleBLEDecoder` — find replacement or document unavailability
4. **F4:** Add `@note` about BLE little-endian byte order convention at file level and on Samsung/Sony/Razer parsers
5. **F5:** Add `@note` about AirPods battery level encoding (0-10 nibble → 0-100% mapping)
6. **F6:** Add full Doxygen to `le16()` and `le32()` in `types.h` per §2.2
7. **F7:** Add `@note` about out-of-range battery nibble handling (values 11-15 → "??")
8. **F8:** Add `@note` on Microsoft parser: "Officially documented by Microsoft"
9. **F9:** Add `@note` on Sony/Razer parsers: "Byte order assumed little-endian per BLE convention, not verified against vendor documentation"
10. **F10:** Create `docs/learning/ble-byte-order-conventions.md`
11. **F11:** Create `docs/learning/vendor-parser-reverse-engineering.md`
12. **F12:** Create `docs/modules/proprietary-parsers.md` per AGENTS.md module documentation standard

**Advisory (should fix but not blocking):**
- F12: Clarify iBeacon TX power comment (add "0-based")
- F13: Verify PETS paper URL or use DOI
- F14: Verify Apple iBeacon URL accessibility

**Coverage target after fixes:** 16/16 public symbols documented (100%).

**ROUTING:** Proceed to Wireless Expert (A1-WX) for protocol-level verification, then to A2 Dual-Model Challenge. Documentation implementation deferred to Phase B (docs are code changes in headers and new doc files).

---

## 14. Documentation Logical Units for Phase B

To align with the SW Engineer's 7-unit implementation plan, the documentation work maps as:

| Doc Unit | Depends On Code Unit | Work |
|----------|---------------------|------|
| **DU1** | U1 (`le16`/`le32` in types.h) | Add Doxygen to `le16()`/`le32()` in `types.h` |
| **DU2** | U2 (byte order fixes) | Add `@note` about LE convention on Samsung/Sony/Razer; fix broken URLs |
| **DU3** | U3 (AirPods labeling) | Add `@note` about battery encoding, out-of-range handling |
| **DU4** | U1-U6 (all code changes) | Add full Doxygen to all 14 symbols in `proprietary_parsers.h` |
| **DU5** | U1-U6 (all code changes) | Create `docs/learning/ble-byte-order-conventions.md` |
| **DU6** | U1-U6 (all code changes) | Create `docs/learning/vendor-parser-reverse-engineering.md` |
| **DU7** | U1-U6 (all code changes) | Create `docs/modules/proprietary-parsers.md` |

DU4 (header Doxygen) should be done **early** — ideally alongside U1-U3 — because it documents the functions being changed. DU5-DU7 (new doc files) can be done after all code changes are stable.
