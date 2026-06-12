#pragma once

/**
 * @file ad_parser.h
 * @brief BLE Advertisement Data (AD) structure parser.
 *
 * Parses the raw advertisement data bytes from a scan result into individual
 * AD structures (TLV format: Length, Type, Value). Also provides a human-readable
 * decoder that uses the assigned numbers lookups to label each structure.
 *
 * @see Core Specification Supplement, Part A, Section 1
 * @see https://wiki.aprbrother.com/en/AT_Commands_For_ABSniffer_528.html
 *
 * @example
 * @code
 * #include <ble_sniffer/ad_parser.h>
 *
 * // Parse scan result advertisement data
 * std::vector<uint8_t> adv_data = scan_msg.adv_data();
 * auto structures = ble_sniffer::parse_ad_structures(adv_data);
 *
 * for (const auto& ad : structures) {
 *     std::cout << ble_sniffer::ad_type_name(ad.type) << ": ";
 *     std::cout << ad.data.size() << " bytes" << std::endl;
 * }
 *
 * // Decode manufacturer from advertisement data
 * auto mfr = ble_sniffer::decode_manufacturer(adv_data);
 * if (mfr) {
 *     std::cout << "Manufacturer: " << *mfr << std::endl;
 * }
 * @endcode
 */

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace ble_sniffer {

/**
 * @brief A single AD structure from BLE advertisement data.
 *
 * The BLE advertisement data format is a sequence of {length, type, data[length-1]}
 * structures. This struct holds one parsed structure.
 */
struct AdStructure {
    uint8_t type;                ///< AD type code (e.g. 0x01 = Flags, 0xFF = Manufacturer Specific)
    std::vector<uint8_t> data;  ///< The AD data bytes (excluding the type byte)
};

/**
 * @brief Parse raw advertisement data into a list of AD structures.
 * @param adv_data The raw advertisement bytes from a ScanResultMessage.
 * @return A vector of parsed AD structures.
 *
 * @example
 * @code
 * // Raw bytes: 02 01 06 0A FF 4C 00 ...
 * auto structures = ble_sniffer::parse_ad_structures(adv_data);
 * // structures[0].type == 0x01 (Flags), structures[0].data == {0x06}
 * @endcode
 */
std::vector<AdStructure> parse_ad_structures(const std::vector<uint8_t>& adv_data);

/**
 * @brief Extract the manufacturer name from advertisement data, if present.
 *
 * Looks for an AD structure with type 0xFF (Manufacturer Specific Data), extracts
 * the 16-bit company ID (little-endian), and returns the company name.
 *
 * @param adv_data The raw advertisement bytes.
 * @return The manufacturer name, or std::nullopt if no manufacturer data is present.
 *
 * @example
 * @code
 * auto name = ble_sniffer::decode_manufacturer(scan.adv_data());
 * if (name) std::cout << "Made by: " << *name << std::endl;
 * @endcode
 */
std::optional<std::string> decode_manufacturer(const std::vector<uint8_t>& adv_data);

/// @overload Decode manufacturer from pre-parsed AD structures (avoids re-parsing).
std::optional<std::string> decode_manufacturer(const std::vector<AdStructure>& structures);

/**
 * @brief Decode the Local Name from advertisement data, if present.
 *
 * Looks for AD type 0x08 (Shortened Local Name) or 0x09 (Complete Local Name).
 *
 * @param adv_data The raw advertisement bytes.
 * @return The device name string, or std::nullopt if not present.
 *
 * @example
 * @code
 * auto name = ble_sniffer::decode_local_name(scan.adv_data());
 * if (name) std::cout << "Device: " << *name << std::endl;
 * @endcode
 */
std::optional<std::string> decode_local_name(const std::vector<uint8_t>& adv_data);

/// @overload Decode local name from pre-parsed AD structures (avoids re-parsing).
std::optional<std::string> decode_local_name(const std::vector<AdStructure>& structures);

/**
 * @brief Decode a single AD structure into a human-readable string.
 *
 * Handles standard AD types: Flags (0x01), Service UUIDs (0x02-0x07),
 * Tx Power Level (0x0A), Appearance (0x19), etc.
 *
 * @param ad The parsed AD structure.
 * @return A human-readable string, or std::nullopt if the type is not decoded.
 */
std::optional<std::string> decode_ad_data(const AdStructure& ad);

} // namespace ble_sniffer
