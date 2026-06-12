---
name: proprietary-parser
description: 'Write new BLE proprietary manufacturer-specific data parsers. Use when: adding a new vendor parser, decoding a new BLE protocol, parsing manufacturer specific data, creating BLE advertisement decoders, reverse engineering BLE payloads.'
argument-hint: 'Vendor name and sample hex data (e.g. "Tile tracker: 01 00 ab cd ef...")'
---

# BLE Proprietary Parser Creation

## When to Use
- Adding a parser for a new BLE manufacturer/vendor
- Decoding a previously-unknown manufacturer-specific payload
- Extending an existing vendor parser with new sub-types
- User provides sample hex data and wants a best-effort decoder

## Prerequisites
- The vendor's Bluetooth SIG company ID (look up in `src/assigned_numbers.cpp`)
- Sample manufacturer-specific data (hex bytes AFTER the 2-byte company ID)
- At least one online reference documenting the protocol structure

## Architecture

All proprietary parsers live in:
- **Header**: `include/ble_sniffer/proprietary_parsers.h`
- **Implementation**: `src/proprietary_parsers.cpp`
- **Namespace**: `ble_sniffer::proprietary::<vendor_name>`

The shared result type is defined once at `ble_sniffer::proprietary` level:
```cpp
struct ParseResult {
    std::string description;  ///< Short label (e.g. "iBeacon", "Swift Pair")
    std::string details;      ///< Decoded fields as human-readable string
};
```

## Procedure

### 1. Research the Protocol

Before writing code, find authoritative references:
- Official vendor documentation (preferred)
- Bluetooth SIG specifications
- Trusted reverse-engineering repos (e.g. furiousMAC/continuity, seemoo-lab/openhaystack)
- Academic papers

**Required**: At least one URL reference per vendor. Prefer multiple for complex protocols.

### 2. Add the Namespace Declaration to the Header

In `include/ble_sniffer/proprietary_parsers.h`, add inside `namespace ble_sniffer::proprietary`:

```cpp
/**
 * @brief Parser for <Vendor Name> BLE manufacturer-specific data.
 *
 * Decodes payloads from company ID 0xNNNN (<Vendor Name>).
 *
 * @see <primary reference URL>
 * @see <secondary reference URL>
 *
 * @example
 * @code
 * // Raw manufacturer data (excluding company ID): XX YY ZZ ...
 * std::vector<uint8_t> payload = {0xXX, 0xYY, 0xZZ};
 * auto result = ble_sniffer::proprietary::vendor_name::parse(payload);
 * if (result) {
 *     std::cout << result->description << ": " << result->details << std::endl;
 *     // Output: "SubType Name: field1=value1 field2=value2"
 * }
 * @endcode
 */
namespace vendor_name {
std::optional<ParseResult> parse(const std::vector<uint8_t>& mfr_data);
} // namespace vendor_name
```

### 3. Implement the Parser

In `src/proprietary_parsers.cpp`, add before the `decode_proprietary` dispatcher:

```cpp
// Reference: <URL to protocol documentation>
// Reference: <URL to secondary source if available>
namespace vendor_name {

/**
 * @brief Parse <Vendor Name> manufacturer-specific BLE data.
 *
 * @param mfr_data The payload bytes AFTER the 2-byte company ID.
 *   For company ID 0xNNNN, if raw AD data is "NN NN XX YY ZZ",
 *   mfr_data contains {0xXX, 0xYY, 0xZZ}.
 *
 * @return Parsed result with description and decoded details,
 *   or std::nullopt if data is too short to parse.
 *
 * @details Protocol structure:
 *   - Byte 0: <field description>
 *   - Byte 1: <field description>
 *   - Bytes 2-N: <field description>
 *
 * @see <reference URL>
 *
 * @example
 * @code
 * std::vector<uint8_t> data = {0x01, 0x02, 0x03};
 * auto r = ble_sniffer::proprietary::vendor_name::parse(data);
 * // r->description == "SubTypeName"
 * // r->details == "field=1 other=0x02"
 * @endcode
 */
std::optional<ParseResult> parse(const std::vector<uint8_t>& mfr_data) {
    if (mfr_data.size() < MINIMUM_SIZE) return std::nullopt;

    // Decode fields...
    std::ostringstream detail;
    detail << "field=" << value;

    return ParseResult{"Description", detail.str()};
}

} // namespace vendor_name
```

### 4. Register in the Dispatcher

In the `decode_proprietary(uint16_t company_id, ...)` switch statement, add:

```cpp
case 0xNNNN: { // Vendor Name
    auto r = vendor_name::parse(payload);
    if (r) result = format(*r);
    break;
}
```

### 5. Build and Verify

```bash
cmake --build --preset conan-debug 2>&1
```

## Code Conventions (Mandatory)

### Documentation
- Every namespace declaration in the header MUST have `@brief`, `@see` (reference URLs), and `@example`
- Every `parse()` implementation MUST have:
  - `@brief` describing what it parses
  - `@param mfr_data` explaining the input format
  - `@return` describing output
  - `@details` with byte-level protocol structure
  - `@see` with reference URL
  - `@example` with concrete usage

### References
- Every vendor section MUST have at least one `// Reference: <URL>` comment above the namespace
- URLs must point to official docs, Bluetooth SIG specs, trusted GitHub repos, or academic papers
- If the protocol is undocumented, note "Best effort decode — no official specification available"

### Implementation Rules
- Input `mfr_data` is the payload AFTER the 2-byte company ID (already stripped)
- Always bounds-check before accessing any byte: `if (mfr_data.size() < N) return std::nullopt;`
- For truncated but parseable data, return partial decode: `ParseResult{"Type (truncated)", partial_hex}`
- Use `bytes_to_hex()` helper (anonymous namespace in proprietary_parsers.cpp) for hex dumps
- Use `bytes_to_ascii()` helper for ASCII payload decoding
- Multi-byte integers: document endianness explicitly in comments
- Return `std::nullopt` only when data is completely unparseable (empty)
- For unknown sub-types, return a generic decode with hex dump

### Naming
- Namespace: `snake_case` matching the vendor (e.g. `tile`, `google`, `xiaomi`)
- ParseResult.description: Short PascalCase or brand name (e.g. "iBeacon", "Swift Pair")
- ParseResult.details: Key-value pairs (e.g. "uuid=... major=1 minor=2")

### Style
- Use `std::ostringstream` for building detail strings
- Hex values formatted as `0x` + zero-padded: `std::setfill('0') << std::setw(2)`
- No unnecessary heap allocations — use string_view where possible for lookups
- Const-correct parameters

## Example: Adding a Tile Tracker Parser

Given sample data: `01 02 14 ab cd ef` (company ID 0x03C0 = Tile Inc.)

1. Research: https://github.com/nicedouble/AppleBLEDecoder (Tile section)
2. Add to header with Doxygen + @example
3. Implement with bounds checks and byte-level comments
4. Register `case 0x03C0:` in dispatcher
5. Build and test

## Checklist

- [ ] At least one reference URL per vendor
- [ ] Doxygen `@brief`, `@param`, `@return`, `@details`, `@see`, `@example` on parse()
- [ ] Doxygen `@brief`, `@see`, `@example` on namespace in header
- [ ] Bounds check before every byte access
- [ ] Graceful handling of truncated data
- [ ] Company ID registered in `decode_proprietary` switch
- [ ] Builds cleanly with `cmake --build --preset conan-debug`
