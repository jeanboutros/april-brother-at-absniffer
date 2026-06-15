# B2-2: APPLY Unit 2

| Field | Value |
|-------|-------|
| Agent | code-architect |
| Timestamp | 2026-06-14T23:30:00Z |
| Step | B2-2 |

## B2-2: APPLY Unit 2 — Byte Order Fixes + URL Fixes

| Unit | 2 |
| Build result | PASS — exit 0, zero warnings |
| Files changed | `src/proprietary_parsers.cpp` (15 edits, +1 include), `test/CMakeLists.txt` (2 lines enabled), `test/compile_time/byte_order_test.cpp` (new, 130 lines) |

### Changes Applied

#### Source: `src/proprietary_parsers.cpp`

| # | Line | Change |
|---|------|--------|
| 1 | 2 | Added `#include <ble_sniffer/types.h>` for `le16()`/`be16()` helpers |
| 2 | 35-36 | Replaced `nicedouble/AppleBLEDecoder` reference with `furiousMAC/continuity` + `developer.apple.com/ibeacon/` (URL fix AC-10) |
| 3 | 50 | Clarified iBeacon comment: "BE -- Apple iBeacon spec, vendor exception to BLE LE convention" + noted byte offset 22 is 0-based |
| 4 | 57 | iBeacon Major: `(static_cast<uint16_t>(mfr_data[18]) << 8) \| mfr_data[19]` → `be16(&mfr_data[18])` (AC-4) |
| 5 | 58 | iBeacon Minor: `(static_cast<uint16_t>(mfr_data[20]) << 8) \| mfr_data[21]` → `be16(&mfr_data[20])` (AC-4) |
| 6 | 70 | AirPods URL: `nicedouble/AppleBLEDecoder` → `furiousMAC/continuity` (AC-10) |
| 7 | 99 | AirPlay URL: `nicedouble/AppleBLEDecoder` → `furiousMAC/continuity` (AC-10) |
| 8 | 177 | Samsung section URL: `nicedouble/AppleBLEDecoder (Samsung section)` → `furiousMAC/continuity (Samsung section)` (AC-10) |
| 9 | 189 | Samsung device_type: `(static_cast<uint16_t>(mfr_data[1]) << 8) \| mfr_data[2]` → `le16(&mfr_data[1])` (AC-1, AC-7) |
| 10 | 234 | Sony section URL: `nicedouble/AppleBLEDecoder (Sony section)` → `furiousMAC/continuity (Sony section)` (AC-10) |
| 11 | 240 | Sony protocol_ver: `(static_cast<uint16_t>(mfr_data[0]) << 8) \| mfr_data[1]` → `le16(&mfr_data[0])` (AC-2, AC-7) |
| 12 | 301 | Razer model: `(static_cast<uint16_t>(mfr_data[0]) << 8) \| mfr_data[1]` → `le16(&mfr_data[0])` (AC-3, AC-7) |
| 13 | 340-341 | company_id: `(cast<<8)\|cast` → `le16(&mfr_ad_data[0])` (AC-7) |

Note: Line 35 already had `furiousMAC/continuity` pre-applied; only the `nicedouble` reference on line 36 was replaced. The 5 URL fixes cover lines 36, 70, 99, 177, 234.

#### Test: `test/compile_time/byte_order_test.cpp` (new file)

9 runtime assertions:
1. **Test 1:** Samsung device_type LE {0x04, 0x00} → 0x0004 displayed in output
2. **Test 2:** Samsung device_type LE {0xFF, 0x00} → 0x00FF (MS byte = 0)
3. **Test 3:** Sony protocol_ver LE {0x34, 0x12} → 0x1234
4. **Test 4:** Razer model LE {0xCD, 0xAB} → 0xABCD
5. **Test 5:** iBeacon Major BE {0x12, 0x34} → decimal 4660
6. **Test 6:** iBeacon Minor BE {0x56, 0x78} → decimal 22136
7. **Test 7:** company_id LE route: {0x4C, 0x00} → 0x004C → Apple → iBeacon description
8. **Test 8:** company_id LE route: {0x75, 0x00} → 0x0075 → Samsung description
9. **Test 9:** iBeacon discriminator: BE {0x01,0x02},{0x03,0x04} → Major=258, Minor=772 (not 513/1027 which would be LE interpretation)

#### Build: `test/CMakeLists.txt`
Enabled `byte_order_test` target (was commented out).

### Verification Results

```bash
# Build: zero warnings, exit 0
cmake --build --preset conan-debug  # PASS

# U1 tests (regression): PASS
./build/Debug/test/le_helpers_test  # PASS

# U2 tests: PASS
./build/Debug/test/byte_order_test  # PASS
```

### Acceptance Criteria Evidence

| AC | Description | Status |
|----|-------------|--------|
| AC-1 | Samsung device_type uses le16(&mfr_data[1]) | ✅ Line 189; Tests 1-2 pass |
| AC-2 | Sony protocol_ver uses le16(&mfr_data[0]) | ✅ Line 240; Test 3 passes |
| AC-3 | Razer model uses le16(&mfr_data[0]) | ✅ Line 301; Test 4 passes |
| AC-4 | iBeacon Major/Minor use be16() per Apple spec | ✅ Lines 57-58; Tests 5-6,9 pass |
| AC-7 | All call sites use le16()/be16() helpers | ✅ 6 helper calls; no raw shifts remain in file |
| AC-10 | 5 broken URLs replaced with furiousMAC/continuity | ✅ grep nicedouble src/ → zero results |

### Notes
- The Sony and Razer comments include `// LE per BLE Core Spec Vol 1 Part A §1; not verified against Sony/Razer docs` — matching the A1-DX condition requiring @note about reverse-engineering status.
- Test 9 explicitly discriminates be16 vs le16 to prevent regression.
- No remaining `nicedouble` references in src/ or test/ directories.
