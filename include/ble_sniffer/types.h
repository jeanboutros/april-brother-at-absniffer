#pragma once

/**
 * @file types.h
 * @brief Core types, enums, and constants for the ABSniffer 528 BLE protocol.
 *
 * This module defines the AT command strings, protocol delimiters, and
 * enumerations used throughout the ble_sniffer library. It has no
 * dependencies beyond the standard library.
 *
 * @example
 * @code
 * #include <ble_sniffer/types.h>
 *
 * // Send a baud rate command parameter
 * int param = static_cast<int>(ble_sniffer::BaudRate::BAUD_115200); // 4
 *
 * // Convert advertisement type to display string
 * auto type = ble_sniffer::AdvertisementType::SCAN_RESPONSE;
 * std::cout << ble_sniffer::advertisement_type_to_string(type);
 * // Output: "Scan Response"
 *
 * // Format a raw MAC address
 * std::string formatted = ble_sniffer::address_to_mac_address("123B6A1AF0E6");
 * // Result: "12:3B:6A:1A:F0:E6"
 * @endcode
 */

#include <string>
#include <string_view>

namespace ble_sniffer {

// --- AT Commands ---

/// Query device status. Response: "OK"
inline constexpr std::string_view AT = "AT";

/// Query the native MAC address. Response: "OK+ADDR:<12-char hex address>"
inline constexpr std::string_view AT_ADDR = "AT+ADDR?";

/// Query the firmware version. Response: "OK+VERS:<version>"
inline constexpr std::string_view AT_VERS = "AT+VERS?";

/// Stop scanning for BLE advertisements.
inline constexpr std::string_view AT_SCAN0 = "AT+SCAN0";

/**
 * @brief Start scanning for BLE advertisements.
 *
 * Output is ASCII, one line per discovered device.
 * Format: OK+SCAN:<mac>,<rssi>,<adv_type>,<data_len>,<adv_data>
 */
inline constexpr std::string_view AT_SCAN1 = "AT+SCAN1";

/// Set the baud rate. Append BaudRate enum value (0-5). e.g. "AT+BAUD4"
inline constexpr std::string_view AT_BAUD = "AT+BAUD";

/// Set active/passive scan mode. Append ScanMode enum value (0-1). e.g. "AT+ACT1"
inline constexpr std::string_view AT_ACT = "AT+ACT";

/// Restart the module immediately.
inline constexpr std::string_view AT_RST = "AT+RST";

// --- Delimiters ---

/// Separates prefix from data in responses (e.g. "OK+SCAN:")
inline constexpr char PREFIX_DELIMITER = ':';

/// Separates fields within a scan response line
inline constexpr char DATA_DELIMITER = ',';

/// All AT commands must be terminated with \r\n
inline constexpr std::string_view COMMAND_DELIMITER = "\r\n";

// --- Enums ---

/**
 * @brief Baud rate parameter for the AT+BAUD command.
 *
 * @example
 * @code
 * // Set device to 115200 bps (the default)
 * driver.set_baud_rate(ble_sniffer::BaudRate::BAUD_115200);
 * @endcode
 */
enum class BaudRate : int {
    BAUD_9600   = 0, ///< 9600 bps
    BAUD_19200  = 1, ///< 19200 bps
    BAUD_38400  = 2, ///< 38400 bps
    BAUD_57600  = 3, ///< 57600 bps
    BAUD_115200 = 4, ///< 115200 bps (device default)
    BAUD_230400 = 5, ///< 230400 bps
};

/**
 * @brief Scan mode parameter for the AT+ACT command.
 *
 * @example
 * @code
 * // Enable active scanning (sends scan requests to advertisers)
 * driver.set_scan_mode(ble_sniffer::ScanMode::ACTIVE);
 * @endcode
 */
enum class ScanMode : int {
    PASSIVE = 0, ///< Passive scan (default). Device listens only.
    ACTIVE  = 1, ///< Active scan. Device sends scan requests to advertisers.
};

/**
 * @brief BLE Advertisement Type as reported in scan results.
 *
 * Corresponds to Table 2 in the ABSniffer 528 AT command reference.
 * @see https://wiki.aprbrother.com/en/AT_Commands_For_ABSniffer_528.html#table-2
 *
 * @example
 * @code
 * auto scan = ble_sniffer::ScanResultMessage::from(raw);
 * if (scan.adv_type() == ble_sniffer::AdvertisementType::SCAN_RESPONSE) {
 *     // Handle scan response
 * }
 * @endcode
 */
enum class AdvertisementType : int {
    CONNECTABLE_UNDIRECTED     = 0, ///< Connectable undirected advertisement
    CONNECTABLE_DIRECTED       = 1, ///< Connectable directed advertisement
    SCANNABLE_UNDIRECTED       = 2, ///< Scannable undirected advertisement
    NON_CONNECTABLE_UNDIRECTED = 3, ///< Non-connectable undirected advertisement
    SCAN_RESPONSE              = 4, ///< Scan Response
};

/**
 * @brief Convert an AdvertisementType enum value to a human-readable string.
 * @param type The advertisement type to convert.
 * @return A null-terminated string describing the type, or "Unknown".
 *
 * @example
 * @code
 * const char* name = ble_sniffer::advertisement_type_to_string(
 *     ble_sniffer::AdvertisementType::NON_CONNECTABLE_UNDIRECTED);
 * // Returns: "Non-connectable undirected advertisement"
 * @endcode
 */
const char* advertisement_type_to_string(AdvertisementType type);

/**
 * @brief Classifies the type of response message received from the device.
 */
enum class MessageType {
    STATUS,      ///< Response to AT query ("OK")
    ADDRESS,     ///< Response to AT+ADDR? query
    VERSION,     ///< Response to AT+VERS? query
    SCAN_RESULT, ///< BLE scan result line
    NO_DATA,     ///< Read timeout — no data received
    ERROR,       ///< Read error from the serial port
    UNKNOWN      ///< Unrecognized response prefix
};

/**
 * @brief Map a response prefix string to its corresponding MessageType.
 * @param prefix The prefix portion of the response (before ':').
 * @return The matching MessageType, or MessageType::UNKNOWN.
 *
 * @example
 * @code
 * auto type = ble_sniffer::message_type_from_prefix("OK+SCAN");
 * // Returns: MessageType::SCAN_RESULT
 * @endcode
 */
MessageType message_type_from_prefix(const std::string& prefix);

/**
 * @brief Format a 12-character hex address string as a colon-separated MAC.
 * @param address_hex Raw 12-char hex string (e.g. "123B6A1AF0E6").
 * @return Formatted MAC address (e.g. "12:3B:6A:1A:F0:E6").
 *         Returns the input unchanged if it is not exactly 12 characters.
 *
 * @example
 * @code
 * std::string mac = ble_sniffer::address_to_mac_address("AABBCCDDEEFF");
 * // Returns: "AA:BB:CC:DD:EE:FF"
 * @endcode
 */
std::string address_to_mac_address(const std::string& address_hex);

} // namespace ble_sniffer
