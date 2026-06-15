#pragma once

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
 *       as exceptions. Use @ref ble_sniffer::le16() and @ref ble_sniffer::le32()
 *       for all LE parsing.
 *
 * @note Some parsers are reverse-engineered and not verified against vendor
 *       documentation. See the parser status table in
 *       `docs/learning/vendor-parser-reverse-engineering.md` for reliability
 *       ratings and byte-order assumptions.
 *
 * @see https://github.com/furiousMAC/continuity (Apple Continuity protocol)
 * @see https://github.com/seemoo-lab/openhaystack (Apple Find My protocol)
 * @see https://learn.microsoft.com/en-us/windows-hardware/design/component-guidelines/bluetooth-swift-pair (Microsoft Swift Pair)
 *
 * @example
 * @code
 * #include <ble_sniffer/proprietary_parsers.h>
 *
 * // Decode Apple manufacturer data (company ID 0x004C already stripped)
 * std::vector<uint8_t> mfr_data = {0x07, 0x04, 0x00, 0x02, 0x00, 0x00, 0x55, 0x05};
 * auto result = ble_sniffer::proprietary::decode_proprietary_parts(0x004C, mfr_data);
 * if (result) {
 *     std::cout << result->description << ": " << result->details << std::endl;
 * }
 * // Output: "AirPods: len=4 model=0x02 L=50% (raw=5) R=50% (raw=5)"
 *
 * // Decode from raw AD data (includes 2-byte company ID prefix)
 * std::vector<uint8_t> ad_data = {0x4C, 0x00, 0x07, 0x04, 0x00, 0x02, 0x00, 0x00, 0x55, 0x05};
 * auto str = ble_sniffer::proprietary::decode_proprietary(ad_data);
 * if (str) std::cout << *str << std::endl;
 * // Output: "AirPods: len=4 model=0x02 L=50% (raw=5) R=50% (raw=5)"
 * @endcode
 */

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace ble_sniffer::proprietary {

/**
 * @brief Structured result from a vendor-specific parser.
 *
 * Separates the human-readable description (e.g., "AirPods", "Find My")
 * from the detailed field values so callers can use them independently —
 * display the description in a summary column and show details on demand.
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
    std::string details;      ///< Field values (e.g. "L=70% (raw=7) R=80% (raw=8)")
};

namespace apple {

/**
 * @brief Parse Apple Inc. (company ID 0x004C) manufacturer-specific data.
 *
 * Handles subtypes: iBeacon (0x02), AirDrop (0x05), AirPods (0x07),
 * AirPlay (0x09), Handoff (0x0C), Nearby Info (0x0F), Nearby Action (0x10),
 * and Find My (0x12). Unknown subtypes return a parse result with the raw
 * sub_type byte.
 *
 * @note All Apple subtypes are reverse-engineered from community references.
 *       Field meanings may change with firmware updates. The original reference
 *       (nicedouble/AppleBLEDecoder) is no longer available; current decoding
 *       is based on furiousMAC/continuity and seemoo-lab/openhaystack.
 *
 * @note iBeacon Major and Minor fields use big-endian byte order per Apple's
 *       iBeacon specification — this is a vendor-specific exception to the
 *       BLE little-endian convention. The parser uses @ref ble_sniffer::be16()
 *       to make this intent explicit.
 *
 * @note AirPods battery level is encoded as 4-bit nibbles (0-10 scale).
 *       Nibble values 0-10 map to 0%-100% in 10% increments (e.g., nibble 7
 *       displays as "70%"). Values 11-15 are out-of-range and display as
 *       "?? (raw=N)" for the percentage while always showing the raw value.
 *       See ADR-0008 for the out-of-range design decision.
 *
 * @param mfr_data  Manufacturer-specific data bytes (excluding the 2-byte
 *                  company ID prefix). Must not be empty.
 * @return          A ParseResult with description and details, or
 *                  std::nullopt if the data is empty.
 *
 * @see https://github.com/furiousMAC/continuity
 * @see https://github.com/seemoo-lab/openhaystack
 * @see https://developer.apple.com/ibeacon/
 *
 * @example
 * @code
 * // AirPods advertisement (sub_type 0x07)
 * std::vector<uint8_t> data = {0x07, 0x04, 0x00, 0x02, 0x00, 0x00, 0x55, 0x05};
 * auto result = ble_sniffer::proprietary::apple::parse(data);
 * // result->description == "AirPods"
 * // result->details == "len=4 model=0x02 L=50% (raw=5) R=50% (raw=5) ..."
 * @endcode
 */
std::optional<ParseResult> parse(const std::vector<uint8_t>& mfr_data);

} // namespace apple

namespace samsung {

/**
 * @brief Parse Samsung Electronics (company ID 0x0075) manufacturer-specific data.
 *
 * Recognises the Galaxy/SmartThings device type format (type byte 0x42).
 * Other type bytes are reported with the raw payload.
 *
 * @note Reverse-engineered from community analysis. The original reference
 *       (nicedouble/AppleBLEDecoder) is no longer available. Multi-byte fields
 *       are assumed little-endian per the BLE Core Specification
 *       (Vol 1, Part A, Section 1). Not verified against Samsung documentation.
 *
 * @param mfr_data  Manufacturer-specific data bytes (excluding company ID).
 *                  Must not be empty.
 * @return          A ParseResult with description and details, or
 *                  std::nullopt if the data is empty.
 *
 * @example
 * @code
 * // Samsung SmartTag advertisement
 * std::vector<uint8_t> data = {0x42, 0x04, 0x00};
 * auto result = ble_sniffer::proprietary::samsung::parse(data);
 * // result->description == "Samsung Galaxy/SmartThings"
 * // result->details == "SmartThings/Galaxy device_type=0x0004"
 * @endcode
 */
std::optional<ParseResult> parse(const std::vector<uint8_t>& mfr_data);

} // namespace samsung

namespace microsoft {

/**
 * @brief Parse Microsoft Corporation (company ID 0x0006) manufacturer-specific data.
 *
 * Handles the Microsoft Swift Pair protocol (scenario byte 0x01). Other
 * scenario bytes are reported with the raw payload.
 *
 * @note **Officially documented by Microsoft.** See the Microsoft Swift Pair
 *       specification for the full payload structure and field definitions.
 *
 * @param mfr_data  Manufacturer-specific data bytes (excluding company ID).
 *                  Must not be empty.
 * @return          A ParseResult with description and details, or
 *                  std::nullopt if the data is empty.
 *
 * @see https://learn.microsoft.com/en-us/windows-hardware/design/component-guidelines/bluetooth-swift-pair
 *
 * @example
 * @code
 * // Microsoft Swift Pair advertisement (scenario 0x01)
 * std::vector<uint8_t> data = {0x01, 0x10, 0x80};
 * auto result = ble_sniffer::proprietary::microsoft::parse(data);
 * // result->description == "Microsoft Swift Pair"
 * // result->details == "Swift Pair ver=1 flags=0x0"
 * @endcode
 */
std::optional<ParseResult> parse(const std::vector<uint8_t>& mfr_data);

} // namespace microsoft

namespace sony {

/**
 * @brief Parse Sony Corporation (company ID 0x012D) manufacturer-specific data.
 *
 * Decodes the protocol version field (2 bytes) followed by an optional payload.
 *
 * @note Reverse-engineered from community analysis. The original reference
 *       (nicedouble/AppleBLEDecoder) is no longer available. Multi-byte fields
 *       are assumed little-endian per the BLE Core Specification
 *       (Vol 1, Part A, Section 1). **Not verified against Sony documentation**
 *       — byte order may differ from the assumption. See ADR-0009.
 *
 * @param mfr_data  Manufacturer-specific data bytes (excluding company ID).
 *                  Must contain at least 2 bytes.
 * @return          A ParseResult with description and details, or
 *                  std::nullopt if fewer than 2 bytes.
 *
 * @example
 * @code
 * std::vector<uint8_t> data = {0x01, 0x00, 0xAA};
 * auto result = ble_sniffer::proprietary::sony::parse(data);
 * // result->description == "Sony"
 * // result->details == "protocol=0x0001 payload=aa"
 * @endcode
 */
std::optional<ParseResult> parse(const std::vector<uint8_t>& mfr_data);

} // namespace sony

namespace sonos {

/**
 * @brief Parse Sonos Inc. (company ID 0x05A7) manufacturer-specific data.
 *
 * Decodes the type and version bytes followed by an optional payload.
 *
 * @note Best-effort decoding based on limited community data. Field meanings
 *       are not confirmed against Sonos documentation and may change with
 *       firmware updates.
 *
 * @param mfr_data  Manufacturer-specific data bytes (excluding company ID).
 *                  Must not be empty.
 * @return          A ParseResult with description and details, or
 *                  std::nullopt if the data is empty.
 *
 * @example
 * @code
 * std::vector<uint8_t> data = {0x01, 0x03, 0xAA, 0xBB};
 * auto result = ble_sniffer::proprietary::sonos::parse(data);
 * // result->description == "Sonos"
 * // result->details == "type=0x01 ver=0x03 payload=aa bb"
 * @endcode
 */
std::optional<ParseResult> parse(const std::vector<uint8_t>& mfr_data);

} // namespace sonos

namespace garmin {

/**
 * @brief Parse Garmin Ltd. (company ID 0x0087) manufacturer-specific data.
 *
 * Decodes the category and status bytes followed by optional extra data.
 *
 * @note Best-effort decoding based on limited community data. Field meanings
 *       are not confirmed against Garmin documentation.
 *
 * @param mfr_data  Manufacturer-specific data bytes (excluding company ID).
 *                  Must not be empty.
 * @return          A ParseResult with description and details, or
 *                  std::nullopt if the data is empty.
 *
 * @example
 * @code
 * std::vector<uint8_t> data = {0x03, 0x01};
 * auto result = ble_sniffer::proprietary::garmin::parse(data);
 * // result->description == "Garmin"
 * // result->details == "category=0x03 status=0x01"
 * @endcode
 */
std::optional<ParseResult> parse(const std::vector<uint8_t>& mfr_data);

} // namespace garmin

namespace razer {

/**
 * @brief Parse Razer Inc. (company ID 0x068E) manufacturer-specific data.
 *
 * Decodes the model number (2 bytes) followed by an optional device state
 * payload.
 *
 * @note Best-effort decoding based on limited community data. Field meanings
 *       are not confirmed against Razer documentation. Multi-byte fields
 *       are assumed little-endian per the BLE Core Specification
 *       (Vol 1, Part A, Section 1). **Not verified against Razer documentation**
 *       — byte order may differ from the assumption. See ADR-0009.
 *
 * @param mfr_data  Manufacturer-specific data bytes (excluding company ID).
 *                  Must contain at least 2 bytes.
 * @return          A ParseResult with description and details, or
 *                  std::nullopt if fewer than 2 bytes.
 *
 * @example
 * @code
 * std::vector<uint8_t> data = {0x1A, 0x02, 0xFF};
 * auto result = ble_sniffer::proprietary::razer::parse(data);
 * // result->description == "Razer"
 * // result->details == "model=0x021a state=ff"
 * @endcode
 */
std::optional<ParseResult> parse(const std::vector<uint8_t>& mfr_data);

} // namespace razer

namespace furbo {

/**
 * @brief Parse Furbo (company ID 0x3030) manufacturer-specific data.
 *
 * The entire payload is an ASCII string (device ID/session data).
 *
 * @note Best-effort decoding based on observation of ASCII payloads. The
 *       format may vary between firmware versions. Non-printable bytes are
 *       displayed as '.' in the ASCII representation.
 *
 * @param mfr_data  Manufacturer-specific data bytes (excluding company ID).
 *                  Must not be empty.
 * @return          A ParseResult with description and details, or
 *                  std::nullopt if the data is empty.
 *
 * @example
 * @code
 * std::vector<uint8_t> data = {'F', 'U', 'R', 'B', 'O', '0', '1'};
 * auto result = ble_sniffer::proprietary::furbo::parse(data);
 * // result->description == "Furbo"
 * // result->details == "ascii=\"FURBO01\""
 * @endcode
 */
std::optional<ParseResult> parse(const std::vector<uint8_t>& mfr_data);

} // namespace furbo

/**
 * @brief Decode vendor-specific manufacturer data into a human-readable string.
 *
 * Routes to the appropriate vendor parser based on the Bluetooth SIG
 * company identifier.
 *
 * @param company_id  Bluetooth SIG 16-bit company identifier
 *                    (e.g., 0x004C for Apple, 0x0075 for Samsung).
 * @param payload     Manufacturer-specific data bytes (excluding the 2-byte
 *                    company ID prefix).
 * @return            A formatted string like "AirPods: len=4 model=0x02 ...",
 *                    or std::nullopt if the company ID is not recognized.
 *
 * @example
 * @code
 * auto str = ble_sniffer::proprietary::decode_proprietary(0x004C, payload);
 * if (str) std::cout << *str << std::endl;
 * // Output: "AirPods: len=4 model=0x02 L=70% (raw=7) R=80% (raw=8)"
 * @endcode
 */
std::optional<std::string> decode_proprietary(uint16_t company_id, const std::vector<uint8_t>& payload);

/**
 * @brief Decode vendor-specific data from raw AD data (including company ID).
 *
 * Convenience wrapper that extracts the 2-byte company ID prefix from the
 * raw manufacturer-specific AD data before delegating to the company_id
 * overload above.
 *
 * @param mfr_ad_data  Raw manufacturer-specific AD data, including the
 *                     2-byte company ID prefix at offset 0.
 * @return             A formatted string, or std::nullopt if the data is
 *                     too short or the company ID is not recognized.
 *
 * @example
 * @code
 * std::vector<uint8_t> ad = {0x4C, 0x00, 0x07, 0x04, 0x00, 0x02, ...};
 * auto str = ble_sniffer::proprietary::decode_proprietary(ad);
 * @endcode
 */
std::optional<std::string> decode_proprietary(const std::vector<uint8_t>& mfr_ad_data);

/**
 * @brief Decode vendor-specific data into a structured ParseResult.
 *
 * Routes to the appropriate vendor parser based on the Bluetooth SIG
 * company identifier. Returns a ParseResult so callers can use the
 * description (e.g., "Find My") and details separately — useful for
 * tabular output where the label and field values are in different columns.
 *
 * @param company_id  Bluetooth SIG 16-bit company identifier.
 * @param payload     Manufacturer-specific data bytes (excluding company ID).
 * @return            A ParseResult with description and details, or
 *                    std::nullopt if the company ID is not recognized.
 *
 * @example
 * @code
 * auto parts = ble_sniffer::proprietary::decode_proprietary_parts(0x004C, payload);
 * if (parts) {
 *     std::cout << "Type: " << parts->description << std::endl;
 *     std::cout << "Data: " << parts->details << std::endl;
 * }
 * @endcode
 */
std::optional<ParseResult> decode_proprietary_parts(uint16_t company_id, const std::vector<uint8_t>& payload);

/**
 * @brief Structured decode from raw AD data (including company ID).
 *
 * @overload Convenience wrapper that extracts the 2-byte company ID prefix
 *           before routing to the vendor-specific parser.
 *
 * @param mfr_ad_data  Raw manufacturer-specific AD data, including the
 *                     2-byte company ID prefix at offset 0.
 * @return             A ParseResult, or std::nullopt if the data is too short
 *                     or the company ID is not recognized.
 *
 * @example
 * @code
 * std::vector<uint8_t> ad = {0x4C, 0x00, 0x07, 0x04, ...};
 * auto parts = ble_sniffer::proprietary::decode_proprietary_parts(ad);
 * @endcode
 */
std::optional<ParseResult> decode_proprietary_parts(const std::vector<uint8_t>& mfr_ad_data);

} // namespace ble_sniffer::proprietary
