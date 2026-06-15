# ADR: AirPods battery output format: percentage + raw parenthetical

**Status:** Accepted
**Date:** 2026-06-15
**Decision:** AirPods battery levels display as `L=50% (raw=5)` — a human-readable percentage derived from the 0-10 nibble, followed by the raw nibble value in parentheses for debugging.

## Context

The AirPods parser in `proprietary_parsers.cpp` decodes battery levels from 4-bit nibbles in Apple's manufacturer-specific data. The nibble values 0-10 map to battery levels (0 = empty, 10 = full), but the existing code displayed only the raw nibble value: `battery L=5 R=5 Case=5`. This was misleading — a user seeing `L=5` might think the battery is at 5% when it's actually at 50%.

The nibble encoding is reverse-engineered from community references (furiousMAC/continuity, seemoo-lab/openhaystack). Values 11-15 (0xB-0xF) are not well-documented and may represent special states (e.g., "charging in case") that vary by AirPods model.

The A1 specialist reviews (SW, TX, WX) all identified this as a user-facing quality issue. The WX specialist flagged it as a blocking finding: "AirPods battery display is misleading — raw nibble values look like percentages."

## Considered Alternatives

| Option | Pros | Cons |
|--------|------|------|
| A: Raw nibble only (current) | Preserves exact data; no interpretation risk | Misleading — `L=5` looks like 5% when it's 50% |
| B: Percentage only (`L=50%`) | User-friendly; matches user expectation | Loses raw data for debugging; hides encoding details |
| C: Percentage + raw parenthetical (`L=50% (raw=5)`) | User-friendly AND debuggable; preserves all data; consistent with existing `charging=0x7` hex format | Slightly longer output string |
| D: Percentage + hex raw (`L=50% (0x5)`) | Consistent hex format with `charging=0x7` | Hex for a decimal-scale value (0-10) is less intuitive than decimal |

## Decision

**Chosen: Option C — `L=50% (raw=5)`.**

The percentage gives users an immediately actionable value that matches their mental model (iOS battery widget shows percentages). The raw parenthetical preserves the exact nibble value for debugging, cross-referencing with raw packet captures, and detecting encoding changes in future AirPods firmware.

The decimal raw format (`raw=5`) is chosen over hex (`0x5`) because the nibble encoding is a decimal scale (0-10), not a bitfield. The existing `charging=0x7` uses hex because the charging field IS a bitfield with independent flag bits.

### Out-of-Range Handling

For nibble values 11-15, the percentage displays as `??` with the raw value preserved:

```
L=?? (raw=15)
```

This is honest about uncertainty — the parser does not know what nibble 15 means for a given AirPods model. The raw value is always preserved so users can:
1. Detect when an AirPods model uses a non-standard encoding
2. Cross-reference with community research
3. Provide raw data for parser improvements

See ADR psc-adr-0008 for the full rationale on out-of-range handling.

## Consequences

### What becomes easier
- **User comprehension:** `L=50%` is immediately understood; no mental conversion from nibble to percentage needed.
- **Debugging:** The raw value is always available for cross-referencing with nRF Connect, Wireshark, or community references.
- **Encoding change detection:** If Apple changes the nibble encoding in a future firmware, users will see `L=?? (raw=15)` instead of silently incorrect percentages.
- **Consistency:** The `(raw=N)` pattern can be adopted by future battery parsers for other vendors.

### What becomes harder
- **Output parsing:** Downstream consumers that parse the CLI output must handle the new format. The change is additive (raw value was already present, percentage is new), so existing parsers that extract the raw number continue to work.

### What is blocked
- **Nothing.** This is a display-format change only. The underlying data parsing is unchanged.

### Risks
- **Nibble interpretation changes:** If Apple changes the nibble-to-percentage mapping in future firmware, the displayed percentage will be wrong. Mitigated by the raw parenthetical — users can always see the actual nibble value and cross-reference.
- **Community disagreement:** Some community sources attribute specific meanings to nibbles 11-15 (e.g., 0xF = "charging in case"). The parser intentionally does not interpret these values, which may frustrate power users. Mitigated by the `@note` in Doxygen acknowledging community attributions.
