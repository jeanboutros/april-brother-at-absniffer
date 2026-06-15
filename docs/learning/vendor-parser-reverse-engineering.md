# Vendor Parser Reverse-Engineering Status

## Overview

The proprietary parsers in `ble_sniffer::proprietary` decode vendor-specific BLE manufacturer data (AD type 0xFF). Most formats are reverse-engineered from community references and may be incomplete or change with firmware updates.

Only one vendor parser (Microsoft Swift Pair) is based on official vendor documentation. All others are reverse-engineered with varying levels of confidence.

## Parser Status Table

| Vendor | Company ID | Status | Reference | Reliability |
|--------|-----------|--------|-----------|-------------|
| Apple | 0x004C | Reverse-engineered | [furiousMAC/continuity](https://github.com/furiousMAC/continuity), [seemoo-lab/openhaystack](https://github.com/seemoo-lab/openhaystack) | **High** — multiple independent academic and community sources; Wireshark dissector available |
| Samsung | 0x0075 | Reverse-engineered | Community analysis (original ref: nicedouble/AppleBLEDecoder, now unavailable) | **Medium** — single source; byte order assumption per BLE LE convention; SmartTag device type 0x0004 confirmed from known device |
| Microsoft | 0x0006 | **Officially documented** | [Microsoft Swift Pair specification](https://learn.microsoft.com/en-us/windows-hardware/design/component-guidelines/bluetooth-swift-pair) | **High** — vendor documentation |
| Sony | 0x012D | Reverse-engineered | Community analysis (original ref: nicedouble/AppleBLEDecoder, now unavailable) | **Low** — single source; byte order assumption per BLE LE convention; not verified against vendor docs (ADR-0009) |
| Sonos | 0x05A7 | Best effort | Limited community data | **Low** — minimal format knowledge; field meanings unconfirmed |
| Garmin | 0x0087 | Best effort | Limited community data | **Low** — minimal format knowledge |
| Razer | 0x068E | Best effort | Limited community data | **Low** — minimal format knowledge; byte order assumption per BLE LE convention; not verified against vendor docs (ADR-0009) |
| Furbo | 0x3030 | Best effort | ASCII payload observation | **Low** — format may vary between firmware versions |

## Reliability Ratings

| Rating | Criteria |
|--------|----------|
| **High** | Multiple independent sources confirm format; vendor documentation exists; or Wireshark dissector available. |
| **Medium** | Single community source; byte-order assumed per BLE spec; at least one known-good value cross-validates the parser. |
| **Low** | Single community source or best-effort decoding; byte-order assumed; no known-good values; format may change. |

## Byte Order Assumptions

All multi-byte fields in manufacturer-specific data are assumed to follow the BLE little-endian convention (Core Spec Vol 1, Part A, §1) unless the vendor specification explicitly states otherwise.

The only documented vendor exception is **Apple iBeacon Major/Minor**, which uses big-endian per Apple's iBeacon specification.

| Vendor | Field | Assumed Order | ADR Reference | Risk |
|--------|-------|---------------|---------------|------|
| Samsung | `device_type` | LE | [ADR-0006](../adr/psc-adr-0006.md) | Low — SmartTag known value confirms LE |
| Sony | `protocol_ver` | LE | [ADR-0009](../adr/psc-adr-0009.md) | Medium — no known-good value to confirm |
| Razer | `model` | LE | [ADR-0009](../adr/psc-adr-0009.md) | Medium — no known-good value to confirm |
| Apple (iBeacon) | `Major`, `Minor` | **BE** (vendor exception) | [ADR-0006](../adr/psc-adr-0006.md) | None — confirmed per Apple iBeacon spec |

## Reference History

The original community reference for Samsung, Sony, and Apple parsers was `nicedouble/AppleBLEDecoder` (GitHub). This repository is no longer available (returns 404 as of 2025). Current decoding is based on:

- **Apple:** [furiousMAC/continuity](https://github.com/furiousMAC/continuity) (PETS 2019/ShmooCon 2020, Wireshark dissector) and [seemoo-lab/openhaystack](https://github.com/seemoo-lab/openhaystack) (PoPETs 2021, Find My reverse engineering)
- **Samsung/Sony/Razer:** Community analysis preserved from the original reference, updated to use library byte-order helpers

## Validation Methods

To cross-validate parser output against ground truth:

1. **nRF Connect for Mobile** (Android/iOS) — displays raw manufacturer data bytes. Compare the hex display against parser output.
2. **LightBlue** (iOS) — displays raw advertisement data including manufacturer-specific fields.
3. **Wireshark + nRF52840 Dongle** — packet-level BLE capture with the Continuity dissector (covers Apple subtypes).
4. **nRF52840 Sniffer** — independent BLE packet capture for cross-validation (see `nrf52840-sniffer` skill).

## Adding a New Vendor Parser

When adding a new vendor `parse()` function:

1. Determine the company ID from the [Bluetooth SIG assigned numbers](https://bitbucket.org/bluetooth-SIG/public/src/main/assigned_numbers/).
2. Research existing community references (GitHub, academic papers, protocol specs).
3. Classify reliability (High/Medium/Low) based on documentation availability.
4. Document byte-order assumptions — default to `le16()`/`le32()` per BLE convention.
5. Add `@note` tags to the Doxygen: reverse-engineering status, reference sources, byte order assumptions.
6. Update this status table with the new vendor entry.
7. Validate with real devices before merging.

## Related

- `docs/learning/ble-byte-order-conventions.md` — BLE LE convention and helper functions
- `docs/modules/proprietary-parsers.md` — full module documentation
- `include/ble_sniffer/proprietary_parsers.h` — public API with Doxygen
- `docs/adr/psc-adr-0006.md` — byte-order helper decision
- `docs/adr/psc-adr-0009.md` — LE convention for unverified parsers
