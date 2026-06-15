# BLE Byte Order Conventions

## Rule

All multi-byte fields in BLE protocol data follow **little-endian** byte order: the byte at the lower index is the least significant byte (LSB).

## Specification Reference

Bluetooth Core Specification, Vol 1, Part A, Section 1:

> "All multiple-octet fields in the protocol stack shall be transmitted in little-endian byte order."

This is further reinforced in Vol 6, Part B, §1.3.1 for the Link Layer.

## Where This Applies

- Company IDs in Manufacturer Specific Data (AD type 0xFF)
- 16-bit and 32-bit Service UUIDs in Service Data (AD types 0x16, 0x20, 0x21)
- Appearance values
- Connection Interval fields (Min/Max)
- All multi-byte fields in vendor-specific manufacturer data (default assumption)

## Vendor Exceptions

| Vendor | Field(s) | Byte Order | Rationale |
|--------|----------|------------|-----------|
| Apple (iBeacon) | Major, Minor | **Big-endian** | Apple iBeacon specification deliberately uses BE for Major and Minor fields. This is an application-layer convention, not a BLE protocol field — the iBeacon payload is application data carried over BLE. |

No other known BLE vendor uses big-endian in manufacturer-specific data. If a vendor did, it would violate the Core Specification.

## Common Bug Pattern

The most frequent byte-order bug in BLE parsers is using big-endian when the protocol mandates little-endian:

```cpp
// BROKEN — big-endian (MSB at lower index)
uint16_t val = (data[0] << 8) | data[1];

// CORRECT — little-endian (LSB at lower index)
uint16_t val = data[0] | (data[1] << 8);
```

This bug was present in three vendor parsers in this codebase:
- **Samsung** `device_type` (fixed in psc-0006)
- **Sony** `protocol_ver` (fixed in psc-0006)
- **Razer** `model` (fixed in psc-0006)

## Helper Functions

Use the library's byte-order helpers for all multi-byte parsing. These are `constexpr`, self-documenting, and eliminate the risk of manual byte-order errors:

| Function | Byte Order | Width | File |
|----------|-----------|-------|------|
| `ble_sniffer::le16(const uint8_t*)` | Little-endian | 16-bit | `include/ble_sniffer/types.h` |
| `ble_sniffer::le32(const uint8_t*)` | Little-endian | 32-bit | `include/ble_sniffer/types.h` |
| `ble_sniffer::be16(const uint8_t*)` | Big-endian | 16-bit | `include/ble_sniffer/types.h` |

### Usage

```cpp
// Parse a little-endian 16-bit company ID from AD data
uint16_t company_id = ble_sniffer::le16(&ad_data[0]);

// Parse a big-endian 16-bit iBeacon Major value
uint16_t major = ble_sniffer::be16(&mfr_data[18]);
```

### Bounds Checking

All three helpers take `const uint8_t*` with no bounds checking. **Callers MUST ensure sufficient buffer length before calling.** All existing call sites in this codebase verify buffer size before accessing bytes. The functions are marked with `@warning` in their Doxygen to document the pre-condition.

## Code Review Grep Pattern

To find any remaining manual byte-order expressions that should use helpers:

```bash
rg 'data\[.*\]\s*\|\s*data\[.*\]\s*<<' --include='*.cpp' --include='*.h'
rg 'data\[.*\]\s*<<\s*8.*\|\s*data\[' --include='*.cpp' --include='*.h'
```

All instances should use `le16()`, `le32()`, or `be16()` instead.

## Related

- `include/ble_sniffer/types.h` — `le16()`, `le32()`, `be16()` definitions
- `docs/learning/vendor-parser-reverse-engineering.md` — vendor-specific byte-order assumptions
- `docs/adr/psc-adr-0006.md` — decision to standardize with `le16()`/`le32()`/`be16()` helpers
- `docs/adr/psc-adr-0009.md` — decision to apply LE convention to Sony/Razer unverified parsers
- Bluetooth Core Specification, Vol 1, Part A, Section 1
