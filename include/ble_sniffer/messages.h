#pragma once

/**
 * @file messages.h
 * @brief Message types for parsing responses from the ABSniffer 528 device.
 *
 * Responses from the device are first parsed into a RawMessage (prefix + data),
 * then converted into typed message structs using their static `from()` factory.
 *
 * @example
 * @code
 * #include <ble_sniffer/messages.h>
 *
 * // Parse a raw response line
 * auto raw = ble_sniffer::RawMessage::parse("OK+SCAN:123B6A1AF0E6,-86,3,30,0201061AFF4C00");
 *
 * // Check type and convert to a specific message
 * if (raw.type() == ble_sniffer::MessageType::SCAN_RESULT) {
 *     auto scan = ble_sniffer::ScanResultMessage::from(raw);
 *     std::cout << scan.mac_address() << std::endl; // "123B6A1AF0E6"
 *     std::cout << scan.rssi() << std::endl;        // -86
 * }
 *
 * // Handle address query response
 * auto addr_raw = ble_sniffer::RawMessage::parse("OK+ADDR:AABBCCDDEEFF");
 * auto addr = ble_sniffer::AddressMessage::from(addr_raw);
 * std::cout << addr.address() << std::endl; // "AA:BB:CC:DD:EE:FF"
 * @endcode
 */

#include <ble_sniffer/types.h>
#include <string>
#include <vector>
#include <cstdint>

namespace ble_sniffer {

/**
 * @brief Base message representing any response line from the device.
 *
 * A response line is split into a prefix (before the first ':') and data
 * (everything after). The message type is determined from the prefix.
 *
 * @example
 * @code
 * auto msg = ble_sniffer::RawMessage::parse("OK+VERS:1.1");
 * msg.prefix(); // "OK+VERS"
 * msg.data();   // "1.1"
 * msg.type();   // MessageType::VERSION
 *
 * // Sentinel values for error/timeout
 * auto empty = ble_sniffer::RawMessage::no_data();
 * empty.type(); // MessageType::NO_DATA
 * @endcode
 */
struct RawMessage {
    /**
     * @brief Parse a complete response line into prefix, data, and type.
     * @param line A full response line (without the trailing \\r\\n).
     * @return A populated RawMessage.
     */
    static RawMessage parse(const std::string& line);

    /// Create a sentinel message indicating no data was received (timeout).
    static RawMessage no_data();

    /// Create a sentinel message indicating a read error occurred.
    static RawMessage error();

    /// The response prefix (e.g. "OK", "OK+SCAN", "OK+ADDR").
    const std::string& prefix() const { return m_prefix; }

    /// The response payload after the prefix delimiter.
    const std::string& data() const { return m_data; }

    /// The classified message type.
    MessageType type() const { return m_type; }

protected:
    std::string m_prefix;
    std::string m_data;
    MessageType m_type = MessageType::UNKNOWN;
};

/**
 * @brief Typed message for AT+ADDR? responses.
 *
 * @example
 * @code
 * auto raw = ble_sniffer::RawMessage::parse("OK+ADDR:AABBCCDDEEFF");
 * auto addr = ble_sniffer::AddressMessage::from(raw);
 * addr.address(); // "AA:BB:CC:DD:EE:FF" (formatted with colons)
 * @endcode
 */
struct AddressMessage : public RawMessage {
    /**
     * @brief Construct from a raw message. Formats the address with colons.
     * @param raw A RawMessage with type == MessageType::ADDRESS.
     */
    static AddressMessage from(const RawMessage& raw);

    /// The device MAC address, formatted as "XX:XX:XX:XX:XX:XX".
    const std::string& address() const { return m_address; }

private:
    std::string m_address;
};

/**
 * @brief Typed message for AT+VERS? responses.
 *
 * @example
 * @code
 * auto raw = ble_sniffer::RawMessage::parse("OK+VERS:1.1");
 * auto ver = ble_sniffer::VersionMessage::from(raw);
 * ver.version(); // "1.1"
 * @endcode
 */
struct VersionMessage : public RawMessage {
    /**
     * @brief Construct from a raw message.
     * @param raw A RawMessage with type == MessageType::VERSION.
     */
    static VersionMessage from(const RawMessage& raw);

    /// The firmware version string.
    const std::string& version() const { return m_version; }

private:
    std::string m_version;
};

/**
 * @brief Typed message for AT status responses ("OK").
 *
 * @example
 * @code
 * auto raw = ble_sniffer::RawMessage::parse("OK");
 * auto status = ble_sniffer::StatusMessage::from(raw);
 * status.type(); // MessageType::STATUS
 * @endcode
 */
struct StatusMessage : public RawMessage {
    /**
     * @brief Construct from a raw message.
     * @param raw A RawMessage with type == MessageType::STATUS.
     */
    static StatusMessage from(const RawMessage& raw);
};

/**
 * @brief Typed message for BLE scan result lines.
 *
 * Parses the comma-separated data field into individual components:
 * MAC address, RSSI, advertisement type, data length, and raw advertisement bytes.
 *
 * @example
 * @code
 * auto raw = ble_sniffer::RawMessage::parse(
 *     "OK+SCAN:123B6A1AF0E6,-86,3,30,0201061AFF4C000215B5B182C7EAB14988AA99");
 * auto scan = ble_sniffer::ScanResultMessage::from(raw);
 *
 * scan.mac_address(); // "123B6A1AF0E6" (raw hex, use address_to_mac_address to format)
 * scan.rssi();        // -86
 * scan.adv_type();    // AdvertisementType::NON_CONNECTABLE_UNDIRECTED
 * scan.data_length(); // 30 (byte count)
 * scan.adv_data();    // vector of parsed bytes
 * @endcode
 */
struct ScanResultMessage : public RawMessage {
    /**
     * @brief Parse a raw scan result into typed fields.
     * @param raw A RawMessage with type == MessageType::SCAN_RESULT.
     */
    static ScanResultMessage from(const RawMessage& raw);

    /// Raw 12-character hex MAC address (e.g. "123B6A1AF0E6").
    const std::string& mac_address() const { return m_mac_address; }

    /// Received Signal Strength Indicator in dBm (negative value).
    int rssi() const { return m_rssi; }

    /// BLE advertisement type as defined in Table 2.
    AdvertisementType adv_type() const { return m_adv_type; }

    /// Declared byte count of the advertisement data.
    int data_length() const { return m_data_length; }

    /// The parsed advertisement data bytes.
    const std::vector<uint8_t>& adv_data() const { return m_adv_data; }

private:
    std::string m_mac_address;
    int m_rssi = 0;
    AdvertisementType m_adv_type = AdvertisementType::CONNECTABLE_UNDIRECTED;
    int m_data_length = 0;
    std::vector<uint8_t> m_adv_data;
};

} // namespace ble_sniffer
