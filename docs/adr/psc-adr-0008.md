# ADR: Out-of-range battery nibble display: "??" with raw value preserved

**Status:** Accepted
**Date:** 2026-06-15
**Decision:** Battery nibble values outside the documented 0-10 range display as `??` for the percentage, with the raw nibble value always preserved in parentheses.

## Context

The AirPods battery level is encoded as a 4-bit nibble in Apple's manufacturer-specific data. Community references (furiousMAC/continuity) document values 0-10 as mapping to battery levels (0 = empty, 10 = full). Values 11-15 (0xB-0xF) are not well-documented and may represent special states (e.g., "charging in case", "not reporting") that vary by AirPods model and firmware version.

The parser is reverse-engineered — there is no official Apple documentation for this encoding. Displaying a percentage for values 11-15 would be misleading (the parser would be guessing). Displaying only the raw value would be inconsistent with the percentage format used for values 0-10.

The A1 specialist reviews (TX, WX) identified this as a design decision requiring explicit handling. The TX specialist proposed three options; the user confirmed Option 1 (`??`).

## Considered Alternatives

| Option | Pros | Cons |
|--------|------|------|
| **1: `??` with raw preserved (`L=?? (raw=15)`)** | Honest about uncertainty; preserves all data; consistent format with in-range values; users can detect encoding anomalies | `??` may look like an error to non-technical users |
| 2: Clamp to nearest valid value (`L=100% (raw=15)`) | Always shows a percentage; no `??` in output | **Misleading** — silently fabricates data; hides encoding anomalies; user cannot distinguish real 100% from clamped 15 |
| 3: Hex only (`L=0xF`) | Preserves exact data; no interpretation risk | Inconsistent format with in-range values (percentage vs hex); loses the user-friendly percentage for valid values |
| 4: Skip display entirely for out-of-range | Clean output for valid data | **Data loss** — user never sees the anomalous value; cannot detect encoding changes or new AirPods models |

## Decision

**Chosen: Option 1 — `L=?? (raw=15)`.**

The `??` marker is honest: it tells the user "this value is outside the documented range, and the parser does not know what it means." The raw value is always preserved so the user can:

1. **Detect encoding anomalies:** If a new AirPods firmware uses nibble 12 to mean "90%", users will see `?? (raw=12)` and can report it.
2. **Cross-reference with community research:** Power users can compare the raw value against community documentation (e.g., furiousMAC/continuity issues, OpenHaystack research).
3. **Provide data for parser improvements:** Raw values in bug reports enable maintainers to update the parser with new encodings.

The `??` marker is intentionally distinct from any valid percentage value — it cannot be confused with 0%, 50%, or 100%.

### Community Attribution Acknowledgment

Some community sources attribute specific meanings to nibble values 11-15:
- Nibble 15 (0xF) is commonly reported as "charging in case" for some AirPods models
- Other values may indicate "not reporting" or "disconnected"

The parser intentionally does not interpret these values because:
1. Attributions are not consistent across AirPods models and firmware versions
2. No official Apple documentation exists to validate the attributions
3. Displaying a guessed meaning could mislead users more than `??`

The Doxygen `@note` for the AirPods parser acknowledges these community attributions but states the parser's intentional non-interpretation.

## Consequences

### What becomes easier
- **Encoding change detection:** Users immediately see when a nibble value falls outside the known range.
- **Parser improvement:** Bug reports include the raw nibble value, enabling data-driven parser updates.
- **Honest uncertainty:** The parser never fabricates data — `??` is a clear signal that the value is unknown.
- **Consistent format:** The `(raw=N)` parenthetical is present for both in-range and out-of-range values, making output parsing consistent.

### What becomes harder
- **Nothing.** The `??` marker is additive — it replaces what would otherwise be a misleading percentage or a silent clamp.

### What is blocked
- **Nothing.** This is a display convention for a specific edge case.

### Risks
- **User confusion:** Non-technical users may interpret `??` as an error or bug. Mitigated by the raw parenthetical — `(raw=15)` makes it clear this is a data value, not a software error.
- **Community pressure:** Power users familiar with community attributions may expect the parser to display "charging in case" for nibble 15. Mitigated by the `@note` acknowledging community attributions and explaining the intentional non-interpretation.
