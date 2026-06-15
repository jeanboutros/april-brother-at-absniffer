# ADR: Apply BLE little-endian convention to unverified vendor parsers

**Status:** Accepted
**Date:** 2026-06-15
**Decision:** Fix Sony `protocol_ver` and Razer `model` byte order from big-endian to little-endian based on the BLE Core Specification mandate, with explicit documentation of the assumption and its risks.

## Context

The Sony and Razer proprietary parsers in `proprietary_parsers.cpp` contain multi-byte field parsing that uses big-endian byte order:

```cpp
// Sony (line 240) — BUGGY: big-endian
uint16_t protocol_ver = (static_cast<uint16_t>(mfr_data[0]) << 8) | mfr_data[1];

// Razer (line 301) — BUGGY: big-endian
uint16_t model = (static_cast<uint16_t>(mfr_data[0]) << 8) | mfr_data[1];
```

The Bluetooth Core Specification (Vol 1, Part A, Section 1; Vol 6, Part B, §1.3.1) mandates that **all** multi-byte fields in the BLE protocol stack are transmitted in little-endian byte order. Manufacturer Specific Data (AD type 0xFF) is part of the BLE advertising protocol stack and therefore follows this convention.

However, **neither Sony nor Razer publish official documentation** for their BLE manufacturer-specific data format. The parsers are reverse-engineered from community references (originally `nicedouble/AppleBLEDecoder`, now unavailable). There is no vendor documentation to confirm or refute the byte order assumption.

The Samsung `device_type` field had the same bug and is being fixed in the same change — but Samsung has known device type values (SmartTag = 0x0004) that can cross-validate the fix. Sony and Razer have no such known values.

## Considered Alternatives

| Option | Pros | Cons |
|--------|------|------|
| **A: Apply LE fix with documented assumption** | Consistent with BLE spec; matches Samsung fix; correct for the vast majority of BLE vendor data | Risk: vendor could use BE (unlikely but possible); displayed values change (byte-swapped) |
| B: Leave as-is (BE) | No risk of introducing wrong values; preserves current behavior | **Known bug** — violates BLE spec; inconsistent with Samsung fix; leaves incorrect values in output |
| C: Apply LE fix but hide values until verified | Safe; no misleading output | Data loss — users lose the field entirely; inconsistent with other parsers that display values |
| D: Display both LE and BE interpretations | Maximum transparency; user can decide | Clutters output; confusing for non-technical users; doubles output length for every multi-byte field |

## Decision

**Chosen: Option A — Apply LE fix with documented assumption.**

The BLE Core Specification is unambiguous: all multi-byte fields in the protocol stack use little-endian. Manufacturer-specific data is part of the advertising protocol stack. While a vendor COULD theoretically use big-endian in their proprietary payload, this would violate the BLE specification and is extremely unlikely — no known BLE vendor does this (the only documented exception is Apple iBeacon Major/Minor, which is an application-layer format, not a BLE protocol field).

The fix is applied with explicit documentation at every level:

1. **Code comment:** `// LE per BLE Core Spec Vol 1 Part A §1; not verified against vendor docs`
2. **Doxygen `@note`:** "Multi-byte fields assumed little-endian per BLE Core Spec convention. Not verified against vendor documentation."
3. **Learning doc:** `docs/learning/vendor-parser-reverse-engineering.md` marks Sony and Razer as "Low" reliability with "byte order assumption" noted.
4. **Module doc:** `docs/modules/proprietary-parsers.md` documents the assumption in the parser descriptions.

### Risk Mitigation

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| Vendor actually uses BE | Very low — violates BLE spec; no known precedent | Displayed values are byte-swapped (wrong) | Comment + Doxygen note the assumption; manual testing with real devices; cross-reference with nRF Connect raw bytes |
| Community reference had correct BE values | Low — community reference was reverse-engineered, may have had the same bug | Values change from community-expected to BLE-correct | The Samsung fix (same bug pattern) was validated against known device type values — suggests the community reference had the same BE bug |
| User relies on current (buggy) values | Low — values were likely wrong; no user has reported them as correct | User's workflow breaks if they depended on specific values | Values were already unreliable (reverse-engineered, undocumented); the fix makes them consistent with BLE spec |

### Verification Plan

1. **Manual testing with real devices:** Capture Sony and Razer BLE advertisements with the fixed parser and cross-reference raw bytes against nRF Connect for Mobile.
2. **Community cross-reference:** If community members report known Sony `protocol_ver` or Razer `model` values, compare against the fixed output.
3. **Golden file diff:** The byte-swapped values will be visible in the before/after golden file comparison.

## Consequences

### What becomes easier
- **Consistency:** All BLE multi-byte fields in the codebase now follow the same little-endian convention. No special cases for "unverified" vendors.
- **Future vendor parsers:** New parsers will use `le16()` by default — the BLE convention is the default assumption.
- **Bug detection:** If a vendor actually uses BE, the discrepancy will be immediately visible in manual testing (values will look wrong compared to nRF Connect raw bytes).

### What becomes harder
- **Nothing.** The fix is a one-line change per field, using the new `le16()` helper.

### What is blocked
- **Nothing.** The fix is additive — it corrects existing buggy behavior.

### Risks (Accepted)
- **Displayed values may be wrong** if Sony or Razer actually use big-endian. This risk is accepted because:
  1. BLE Core Spec mandates LE — BE would be a spec violation
  2. No known BLE vendor uses BE in manufacturer-specific data
  3. The values were already unreliable (reverse-engineered, undocumented)
  4. The assumption is clearly documented at all levels
  5. Manual testing with real devices will catch any discrepancy
