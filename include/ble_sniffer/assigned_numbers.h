#pragma once

/**
 * @file assigned_numbers.h
 * @brief Bluetooth SIG assigned number lookups (company IDs and AD types).
 *
 * This module provides lookup functions for Bluetooth assigned numbers,
 * generated from the official Bluetooth SIG repository. To update the data,
 * run: `python3 scripts/generate_assigned_numbers.py`
 *
 * @see https://bitbucket.org/bluetooth-SIG/public/src/main/assigned_numbers/
 *
 * @example
 * @code
 * #include <ble_sniffer/assigned_numbers.h>
 *
 * // Look up a manufacturer by company ID (from Manufacturer Specific Data)
 * const char* name = ble_sniffer::company_name_from_id(0x004C);
 * // Returns: "Apple, Inc."
 *
 * // Look up an AD type name
 * const char* type_name = ble_sniffer::ad_type_name(0x09);
 * // Returns: "Complete Local Name"
 * @endcode
 */

#include <cstdint>

namespace ble_sniffer {

/**
 * @brief Look up a company name by its Bluetooth SIG company identifier.
 * @param company_id The 16-bit company identifier (from Manufacturer Specific Data).
 * @return The company name, or "Unknown" if not found.
 *
 * @example
 * @code
 * // Decode manufacturer from advertisement data (AD type 0xFF)
 * uint16_t id = ble_sniffer::le16(&adv_data[0]);
 * std::cout << ble_sniffer::company_name_from_id(id) << std::endl;
 * @endcode
 */
const char* company_name_from_id(uint16_t company_id);

/**
 * @brief Look up the name of a BLE AD (Advertising Data) type.
 * @param type The 1-byte AD type code.
 * @return The AD type name, or "Unknown" if not recognized.
 *
 * @example
 * @code
 * uint8_t ad_type = 0x01;
 * std::cout << ble_sniffer::ad_type_name(ad_type) << std::endl;
 * // Output: "Flags"
 * @endcode
 */
const char* ad_type_name(uint8_t type);

} // namespace ble_sniffer
