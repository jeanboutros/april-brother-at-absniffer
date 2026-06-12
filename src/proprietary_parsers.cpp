#include "ble_sniffer/proprietary_parsers.h"

#include <iomanip>
#include <sstream>

namespace ble_sniffer::proprietary {

namespace {

std::string bytes_to_hex(const std::vector<uint8_t>& data, size_t offset = 0, size_t len = 0) {
    std::ostringstream ss;
    size_t end = (len == 0) ? data.size() : std::min(offset + len, data.size());
    for (size_t i = offset; i < end; ++i) {
        if (i > offset) ss << ' ';
        ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(data[i]);
    }
    return ss.str();
}

std::string bytes_to_ascii(const std::vector<uint8_t>& data, size_t offset = 0) {
    std::string result;
    for (size_t i = offset; i < data.size(); ++i) {
        char c = static_cast<char>(data[i]);
        if (c >= 0x20 && c <= 0x7E)
            result += c;
        else
            result += '.';
    }
    return result;
}

} // anonymous namespace

// Reference: https://github.com/furiousMAC/continuity
// Reference: https://github.com/nicedouble/AppleBLEDecoder
// Reference: https://developer.apple.com/ibeacon/
namespace apple {

std::optional<ParseResult> parse(const std::vector<uint8_t>& mfr_data) {
    if (mfr_data.empty()) return std::nullopt;

    uint8_t sub_type = mfr_data[0];

    switch (sub_type) {
    case 0x02: { // iBeacon
        if (mfr_data.size() < 22) {
            return ParseResult{"iBeacon (truncated)", bytes_to_hex(mfr_data, 1)};
        }
        // Byte 1 = length (0x15 = 21)
        // Bytes 2-17 = UUID, 18-19 = Major (BE), 20-21 = Minor (BE), 22 = TX Power
        std::ostringstream uuid;
        uuid << std::hex << std::setfill('0');
        for (int i = 2; i < 18; ++i) {
            uuid << std::setw(2) << static_cast<int>(mfr_data[i]);
            if (i == 5 || i == 7 || i == 9 || i == 11) uuid << '-';
        }
        uint16_t major = (static_cast<uint16_t>(mfr_data[18]) << 8) | mfr_data[19];
        uint16_t minor = (static_cast<uint16_t>(mfr_data[20]) << 8) | mfr_data[21];
        int8_t tx_power = (mfr_data.size() > 22) ? static_cast<int8_t>(mfr_data[22]) : 0;

        std::ostringstream detail;
        detail << "UUID=" << uuid.str() << " Major=" << major << " Minor=" << minor
               << " TxPwr=" << static_cast<int>(tx_power) << " dBm";
        return ParseResult{"iBeacon", detail.str()};
    }
    case 0x05: { // AirDrop
        return ParseResult{"AirDrop", bytes_to_hex(mfr_data, 1)};
    }
    case 0x07: { // AirPods
        // Reference: https://github.com/nicedouble/AppleBLEDecoder
        if (mfr_data.size() < 4) {
            return ParseResult{"AirPods (truncated)", bytes_to_hex(mfr_data, 1)};
        }
        uint8_t length = mfr_data[1];
        uint8_t device_model = (mfr_data.size() > 3) ? mfr_data[3] : 0;

        std::ostringstream detail;
        detail << "len=" << static_cast<int>(length)
               << " model=0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(device_model);

        if (mfr_data.size() >= 7) {
            // Battery and status bits are encoded in later bytes
            uint8_t battery_byte = mfr_data[6];
            uint8_t left = (battery_byte >> 4) & 0x0F;
            uint8_t right = battery_byte & 0x0F;
            detail << " battery L=" << static_cast<int>(left) << " R=" << static_cast<int>(right);
        }
        if (mfr_data.size() >= 8) {
            uint8_t case_battery = mfr_data[7] & 0x0F;
            uint8_t charging = (mfr_data[7] >> 4) & 0x07;
            bool lid_open = (mfr_data[7] >> 7) & 0x01;
            detail << " Case=" << static_cast<int>(case_battery)
                   << " charging=0x" << std::hex << static_cast<int>(charging)
                   << (lid_open ? " lid=open" : " lid=closed");
        }
        return ParseResult{"AirPods", detail.str()};
    }
    case 0x09: { // AirPlay
        // Reference: https://github.com/nicedouble/AppleBLEDecoder
        if (mfr_data.size() < 3) {
            return ParseResult{"AirPlay", bytes_to_hex(mfr_data, 1)};
        }
        uint8_t flags = mfr_data[2];
        std::ostringstream detail;
        detail << "flags=0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(flags);
        if (mfr_data.size() > 3) {
            detail << " seed=0x" << std::setw(2) << static_cast<int>(mfr_data[3]);
        }
        detail << " payload=" << bytes_to_hex(mfr_data, 4);
        return ParseResult{"AirPlay", detail.str()};
    }
    case 0x0C: { // Handoff
        // Reference: https://petsymposium.org/popets/2020/popets-2020-0003.pdf
        // byte[0]=type, byte[1]=sub_length, byte[2]=version, byte[3..]=encrypted payload
        if (mfr_data.size() < 4) {
            return ParseResult{"Handoff", bytes_to_hex(mfr_data, 1)};
        }
        uint8_t version = mfr_data[2];
        std::ostringstream detail;
        detail << "ver=" << static_cast<int>(version) << " encrypted=" << bytes_to_hex(mfr_data, 3);
        return ParseResult{"Handoff", detail.str()};
    }
    case 0x0F: { // Nearby Info
        // Reference: https://github.com/furiousMAC/continuity
        if (mfr_data.size() < 3) {
            return ParseResult{"Nearby Info", bytes_to_hex(mfr_data, 1)};
        }
        uint8_t status_flags = mfr_data[2];
        uint8_t action_code = (mfr_data.size() > 3) ? mfr_data[3] : 0;
        std::ostringstream detail;
        detail << "status=0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(status_flags)
               << " action=0x" << std::setw(2) << static_cast<int>(action_code);
        if (mfr_data.size() > 4) {
            detail << " extra=" << bytes_to_hex(mfr_data, 4);
        }
        return ParseResult{"Nearby Info", detail.str()};
    }
    case 0x10: { // Nearby Action
        // Reference: https://github.com/furiousMAC/continuity
        // byte[0]=type, byte[1]=sub_length, byte[2]=action_flags, byte[3]=action_type, byte[4..]=auth_tag
        if (mfr_data.size() < 4) {
            return ParseResult{"Nearby Action", bytes_to_hex(mfr_data, 1)};
        }
        uint8_t action_flags = mfr_data[2];
        uint8_t action_type = mfr_data[3];
        std::ostringstream detail;
        detail << "flags=0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(action_flags)
               << " type=0x" << std::setw(2) << static_cast<int>(action_type);
        if (mfr_data.size() > 4) {
            detail << " auth=" << bytes_to_hex(mfr_data, 4);
        }
        return ParseResult{"Nearby Action", detail.str()};
    }
    case 0x12: { // Find My
        // Reference: https://github.com/seemoo-lab/openhaystack
        // byte[0]=type, byte[1]=sub_length, byte[2]=status, byte[3..]=public_key
        if (mfr_data.size() < 4) {
            return ParseResult{"Find My", bytes_to_hex(mfr_data, 1)};
        }
        uint8_t status = mfr_data[2];
        std::ostringstream detail;
        detail << "status=0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(status)
               << " key=" << bytes_to_hex(mfr_data, 3);
        return ParseResult{"Find My", detail.str()};
    }
    default: {
        std::ostringstream detail;
        detail << "sub_type=0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(sub_type)
               << " data=" << bytes_to_hex(mfr_data, 1);
        return ParseResult{"Apple (unknown type)", detail.str()};
    }
    }
}

} // namespace apple

// Reference: https://github.com/nicedouble/AppleBLEDecoder (Samsung section)
namespace samsung {

std::optional<ParseResult> parse(const std::vector<uint8_t>& mfr_data) {
    if (mfr_data.empty()) return std::nullopt;

    uint8_t type_byte = mfr_data[0];
    std::ostringstream detail;

    if (type_byte == 0x42) {
        detail << "SmartThings/Galaxy";
        if (mfr_data.size() >= 3) {
            uint16_t device_type = (static_cast<uint16_t>(mfr_data[1]) << 8) | mfr_data[2];
            detail << " device_type=0x" << std::hex << std::setfill('0') << std::setw(4) << device_type;
        }
        if (mfr_data.size() > 3) {
            detail << " payload=" << bytes_to_hex(mfr_data, 3);
        }
        return ParseResult{"Samsung Galaxy/SmartThings", detail.str()};
    }

    detail << "type=0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(type_byte)
           << " payload=" << bytes_to_hex(mfr_data, 1);
    return ParseResult{"Samsung", detail.str()};
}

} // namespace samsung

// Reference: https://learn.microsoft.com/en-us/windows-hardware/design/component-guidelines/bluetooth-swift-pair
namespace microsoft {

std::optional<ParseResult> parse(const std::vector<uint8_t>& mfr_data) {
    if (mfr_data.empty()) return std::nullopt;

    uint8_t scenario = mfr_data[0];
    std::ostringstream detail;

    if (scenario == 0x01) {
        detail << "Swift Pair";
        if (mfr_data.size() >= 2) {
            uint8_t version_flags = mfr_data[1];
            detail << " ver=" << static_cast<int>(version_flags >> 4)
                   << " flags=0x" << std::hex << std::setfill('0') << std::setw(1) << static_cast<int>(version_flags & 0x0F);
        }
        if (mfr_data.size() > 2) {
            detail << " payload=" << bytes_to_hex(mfr_data, 2);
        }
        return ParseResult{"Microsoft Swift Pair", detail.str()};
    }

    detail << "scenario=0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(scenario)
           << " payload=" << bytes_to_hex(mfr_data, 1);
    return ParseResult{"Microsoft", detail.str()};
}

} // namespace microsoft

// Reference: https://github.com/nicedouble/AppleBLEDecoder (Sony section)
namespace sony {

std::optional<ParseResult> parse(const std::vector<uint8_t>& mfr_data) {
    if (mfr_data.size() < 2) return std::nullopt;

    uint16_t protocol_ver = (static_cast<uint16_t>(mfr_data[0]) << 8) | mfr_data[1];
    std::ostringstream detail;
    detail << "protocol=0x" << std::hex << std::setfill('0') << std::setw(4) << protocol_ver;
    if (mfr_data.size() > 2) {
        detail << " payload=" << bytes_to_hex(mfr_data, 2);
    }
    return ParseResult{"Sony", detail.str()};
}

} // namespace sony

// Best effort: device type, group info, volume state
namespace sonos {

std::optional<ParseResult> parse(const std::vector<uint8_t>& mfr_data) {
    if (mfr_data.empty()) return std::nullopt;

    std::ostringstream detail;
    if (mfr_data.size() >= 2) {
        uint8_t type = mfr_data[0];
        uint8_t version = mfr_data[1];
        detail << "type=0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(type)
               << " ver=0x" << std::setw(2) << static_cast<int>(version);
        if (mfr_data.size() > 2) {
            detail << " payload=" << bytes_to_hex(mfr_data, 2);
        }
    } else {
        detail << "data=" << bytes_to_hex(mfr_data);
    }
    return ParseResult{"Sonos", detail.str()};
}

} // namespace sonos

// Best effort: very short payloads (category + status)
namespace garmin {

std::optional<ParseResult> parse(const std::vector<uint8_t>& mfr_data) {
    if (mfr_data.empty()) return std::nullopt;

    std::ostringstream detail;
    if (mfr_data.size() >= 2) {
        detail << "category=0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(mfr_data[0])
               << " status=0x" << std::setw(2) << static_cast<int>(mfr_data[1]);
    } else {
        detail << "data=0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(mfr_data[0]);
    }
    if (mfr_data.size() > 2) {
        detail << " extra=" << bytes_to_hex(mfr_data, 2);
    }
    return ParseResult{"Garmin", detail.str()};
}

} // namespace garmin

// Best effort: model + device state
namespace razer {

std::optional<ParseResult> parse(const std::vector<uint8_t>& mfr_data) {
    if (mfr_data.size() < 2) return std::nullopt;

    uint16_t model = (static_cast<uint16_t>(mfr_data[0]) << 8) | mfr_data[1];
    std::ostringstream detail;
    detail << "model=0x" << std::hex << std::setfill('0') << std::setw(4) << model;
    if (mfr_data.size() > 2) {
        detail << " state=" << bytes_to_hex(mfr_data, 2);
    }
    return ParseResult{"Razer", detail.str()};
}

} // namespace razer

// Entire payload is ASCII hex string (device ID/session)
namespace furbo {

std::optional<ParseResult> parse(const std::vector<uint8_t>& mfr_data) {
    if (mfr_data.empty()) return std::nullopt;

    std::string ascii = bytes_to_ascii(mfr_data);
    return ParseResult{"Furbo", "ascii=\"" + ascii + "\""};
}

} // namespace furbo

std::optional<ParseResult> decode_proprietary_parts(uint16_t company_id, const std::vector<uint8_t>& payload) {
    switch (company_id) {
    case 0x004C: return apple::parse(payload);
    case 0x0075: return samsung::parse(payload);
    case 0x0006: return microsoft::parse(payload);
    case 0x012D: return sony::parse(payload);
    case 0x05A7: return sonos::parse(payload);
    case 0x0087: return garmin::parse(payload);
    case 0x068E: return razer::parse(payload);
    case 0x3030: return furbo::parse(payload);
    default: return std::nullopt;
    }
}

std::optional<ParseResult> decode_proprietary_parts(const std::vector<uint8_t>& mfr_ad_data) {
    if (mfr_ad_data.size() < 2) return std::nullopt;
    uint16_t company_id = static_cast<uint16_t>(mfr_ad_data[0]) |
                           (static_cast<uint16_t>(mfr_ad_data[1]) << 8);
    std::vector<uint8_t> payload(mfr_ad_data.begin() + 2, mfr_ad_data.end());
    return decode_proprietary_parts(company_id, payload);
}

std::optional<std::string> decode_proprietary(uint16_t company_id, const std::vector<uint8_t>& payload) {
    auto parts = decode_proprietary_parts(company_id, payload);
    if (!parts) return std::nullopt;
    return parts->description + ": " + parts->details;
}

std::optional<std::string> decode_proprietary(const std::vector<uint8_t>& mfr_ad_data) {
    auto parts = decode_proprietary_parts(mfr_ad_data);
    if (!parts) return std::nullopt;
    return parts->description + ": " + parts->details;
}

} // namespace ble_sniffer::proprietary
