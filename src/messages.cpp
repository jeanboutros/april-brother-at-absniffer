/**
 * @file messages.cpp
 * @brief Implementation of message parsing and type conversion utilities.
 */

#include <ble_sniffer/messages.h>
#include <ble_sniffer/types.h>

namespace ble_sniffer {

// --- Free functions ---

std::string address_to_mac_address(const std::string& address_hex) {
    if (address_hex.size() != 12) {
        return address_hex;
    }
    std::string mac;
    for (size_t i = 0; i < address_hex.size(); i += 2) {
        if (i > 0) {
            mac += ":";
        }
        mac += address_hex.substr(i, 2);
    }
    return mac;
}

MessageType message_type_from_prefix(const std::string& prefix) {
    if (prefix == "OK")       return MessageType::STATUS;
    if (prefix == "OK+ADDR")  return MessageType::ADDRESS;
    if (prefix == "OK+VERS")  return MessageType::VERSION;
    if (prefix == "OK+SCAN")  return MessageType::SCAN_RESULT;
    return MessageType::UNKNOWN;
}

const char* advertisement_type_to_string(AdvertisementType type) {
    switch (type) {
        case AdvertisementType::CONNECTABLE_UNDIRECTED:     return "Connectable undirected advertisement";
        case AdvertisementType::CONNECTABLE_DIRECTED:       return "Connectable directed advertisement";
        case AdvertisementType::SCANNABLE_UNDIRECTED:       return "Scannable undirected advertisement";
        case AdvertisementType::NON_CONNECTABLE_UNDIRECTED: return "Non-connectable undirected advertisement";
        case AdvertisementType::SCAN_RESPONSE:              return "Scan Response";
    }
    return "Unknown";
}

// --- RawMessage ---

RawMessage RawMessage::parse(const std::string& line) {
    RawMessage msg;
    auto pos = line.find(PREFIX_DELIMITER);
    if (pos != std::string::npos) {
        msg.m_prefix = line.substr(0, pos);
        msg.m_data = line.substr(pos + 1);
    } else {
        msg.m_prefix = line;
        msg.m_data = "";
    }
    msg.m_type = message_type_from_prefix(msg.m_prefix);
    return msg;
}

RawMessage RawMessage::no_data() {
    RawMessage msg;
    msg.m_type = MessageType::NO_DATA;
    return msg;
}

RawMessage RawMessage::error() {
    RawMessage msg;
    msg.m_type = MessageType::ERROR;
    return msg;
}

// --- AddressMessage ---

AddressMessage AddressMessage::from(const RawMessage& raw) {
    AddressMessage msg;
    msg.m_prefix = raw.prefix();
    msg.m_data = raw.data();
    msg.m_type = raw.type();
    msg.m_address = address_to_mac_address(raw.data());
    return msg;
}

// --- VersionMessage ---

VersionMessage VersionMessage::from(const RawMessage& raw) {
    VersionMessage msg;
    msg.m_prefix = raw.prefix();
    msg.m_data = raw.data();
    msg.m_type = raw.type();
    msg.m_version = raw.data();
    return msg;
}

// --- StatusMessage ---

StatusMessage StatusMessage::from(const RawMessage& raw) {
    StatusMessage msg;
    msg.m_prefix = raw.prefix();
    msg.m_data = raw.data();
    msg.m_type = raw.type();
    return msg;
}

// --- ScanResultMessage ---

ScanResultMessage ScanResultMessage::from(const RawMessage& raw) {
    ScanResultMessage msg;
    msg.m_prefix = raw.prefix();
    msg.m_data = raw.data();
    msg.m_type = raw.type();

    // Fields: <mac>,<rssi>,<adv_type>,<data_len>,<adv_data>
    std::string remaining = raw.data();
    size_t pos = 0;

    // MAC address (store raw hex, let caller decide formatting)
    pos = remaining.find(DATA_DELIMITER);
    if (pos != std::string::npos) {
        msg.m_mac_address = remaining.substr(0, pos);
        remaining = remaining.substr(pos + 1);
    }

    // RSSI in dBm
    pos = remaining.find(DATA_DELIMITER);
    if (pos != std::string::npos) {
        msg.m_rssi = std::stoi(remaining.substr(0, pos));
        remaining = remaining.substr(pos + 1);
    }

    // Advertisement type (integer mapped to AdvertisementType enum)
    pos = remaining.find(DATA_DELIMITER);
    if (pos != std::string::npos) {
        msg.m_adv_type = static_cast<AdvertisementType>(std::stoi(remaining.substr(0, pos)));
        remaining = remaining.substr(pos + 1);
    }

    // Data length (byte count, decimal)
    pos = remaining.find(DATA_DELIMITER);
    if (pos != std::string::npos) {
        msg.m_data_length = std::stoi(remaining.substr(0, pos));
        remaining = remaining.substr(pos + 1);
    }

    // Advertisement data (hex string → byte vector)
    for (size_t i = 0; i + 1 < remaining.size(); i += 2) {
        uint8_t byte = static_cast<uint8_t>(std::stoi(remaining.substr(i, 2), nullptr, 16));
        msg.m_adv_data.push_back(byte);
    }

    return msg;
}

} // namespace ble_sniffer
