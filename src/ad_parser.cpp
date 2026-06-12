#include <ble_sniffer/ad_parser.h>
#include <ble_sniffer/assigned_numbers.h>

#include <iomanip>
#include <sstream>

namespace ble_sniffer {

std::vector<AdStructure> parse_ad_structures(const std::vector<uint8_t>& adv_data) {
    std::vector<AdStructure> structures;
    size_t offset = 0;

    while (offset < adv_data.size()) {
        uint8_t length = adv_data[offset];
        if (length == 0) {
            break; // End of meaningful data
        }
        if (offset + length >= adv_data.size()) {
            break; // Incomplete structure
        }

        AdStructure ad;
        ad.type = adv_data[offset + 1];
        ad.data.assign(
            adv_data.begin() + offset + 2,
            adv_data.begin() + offset + 1 + length
        );

        structures.push_back(std::move(ad));
        offset += 1 + length;
    }

    return structures;
}

std::optional<std::string> decode_manufacturer(const std::vector<AdStructure>& structures) {
    for (const auto& ad : structures) {
        if (ad.type == 0xFF && ad.data.size() >= 2) {
            uint16_t company_id = static_cast<uint16_t>(ad.data[0]) |
                                  (static_cast<uint16_t>(ad.data[1]) << 8);
            return std::string(company_name_from_id(company_id));
        }
    }
    return std::nullopt;
}

std::optional<std::string> decode_manufacturer(const std::vector<uint8_t>& adv_data) {
    return decode_manufacturer(parse_ad_structures(adv_data));
}

std::optional<std::string> decode_local_name(const std::vector<AdStructure>& structures) {
    for (const auto& ad : structures) {
        if ((ad.type == 0x08 || ad.type == 0x09) && !ad.data.empty()) {
            return std::string(ad.data.begin(), ad.data.end());
        }
    }
    return std::nullopt;
}

std::optional<std::string> decode_local_name(const std::vector<uint8_t>& adv_data) {
    return decode_local_name(parse_ad_structures(adv_data));
}

// Reference: https://www.bluetooth.com/specifications/assigned-numbers/ (Section 2.3 - Generic Access Profile)
// Reference: https://www.bluetooth.com/specifications/specs/core-specification-supplement-11/ (Part A, Section 1)
// Reference: https://bitbucket.org/bluetooth-SIG/public/src/main/assigned_numbers/core/ad_types.yaml
std::optional<std::string> decode_ad_data(const AdStructure& ad) {
    switch (ad.type) {
    case 0x01: { // Flags - Core Spec Supplement Part A, Section 1.3
        if (ad.data.empty()) return std::nullopt;
        uint8_t flags = ad.data[0];
        std::string result;
        if (flags & 0x01) result += "LE Limited Discoverable | ";
        if (flags & 0x02) result += "LE General Discoverable | ";
        if (flags & 0x04) result += "BR/EDR Not Supported | ";
        if (flags & 0x08) result += "LE+BR/EDR Controller | ";
        if (flags & 0x10) result += "LE+BR/EDR Host | ";
        if (!result.empty()) result.erase(result.size() - 3); // trailing " | "
        return result;
    }
    // Core Spec Supplement Part A, Section 1.1
    // https://www.bluetooth.com/specifications/assigned-numbers/ (Section 3.4 - 16-bit UUIDs)
    case 0x02: // Incomplete List of 16-bit Service UUIDs
    case 0x03: { // Complete List of 16-bit Service UUIDs
        if (ad.data.size() < 2) return std::nullopt;
        std::string result;
        for (size_t i = 0; i + 1 < ad.data.size(); i += 2) {
            uint16_t uuid = static_cast<uint16_t>(ad.data[i]) |
                            (static_cast<uint16_t>(ad.data[i + 1]) << 8);
            if (!result.empty()) result += ", ";
            std::ostringstream ss;
            ss << "0x" << std::hex << std::setfill('0') << std::setw(4) << uuid;
            result += ss.str();
        }
        return result;
    }
    // Core Spec Supplement Part A, Section 1.1
    case 0x04: // Incomplete List of 32-bit Service UUIDs
    case 0x05: { // Complete List of 32-bit Service UUIDs
        if (ad.data.size() < 4) return std::nullopt;
        std::string result;
        for (size_t i = 0; i + 3 < ad.data.size(); i += 4) {
            uint32_t uuid = static_cast<uint32_t>(ad.data[i]) |
                            (static_cast<uint32_t>(ad.data[i + 1]) << 8) |
                            (static_cast<uint32_t>(ad.data[i + 2]) << 16) |
                            (static_cast<uint32_t>(ad.data[i + 3]) << 24);
            if (!result.empty()) result += ", ";
            std::ostringstream ss;
            ss << "0x" << std::hex << std::setfill('0') << std::setw(8) << uuid;
            result += ss.str();
        }
        return result;
    }
    // Core Spec Supplement Part A, Section 1.1
    // https://www.bluetooth.com/specifications/assigned-numbers/ (Section 3.5 - 128-bit UUIDs)
    case 0x06: // Incomplete List of 128-bit Service UUIDs
    case 0x07: { // Complete List of 128-bit Service UUIDs
        if (ad.data.size() < 16) return std::nullopt;
        std::string result;
        for (size_t i = 0; i + 15 < ad.data.size(); i += 16) {
            if (!result.empty()) result += ", ";
            std::ostringstream ss;
            ss << std::hex << std::setfill('0');
            // 128-bit UUID in little-endian, display as standard format
            for (int j = 15; j >= 0; --j) {
                ss << std::setw(2) << static_cast<int>(ad.data[i + j]);
                if (j == 12 || j == 10 || j == 8 || j == 6) ss << '-';
            }
            result += ss.str();
        }
        return result;
    }
    // Core Spec Supplement Part A, Section 1.5
    // https://www.bluetooth.com/specifications/specs/core-specification-supplement-11/ (Table 1.4)
    case 0x0A: { // Tx Power Level
        if (ad.data.empty()) return std::nullopt;
        int8_t tx_power = static_cast<int8_t>(ad.data[0]);
        return std::to_string(static_cast<int>(tx_power)) + " dBm";
    }
    // Core Spec Supplement Part A, Section 1.12
    // https://www.bluetooth.com/specifications/assigned-numbers/ (Section 6.2 - Appearance Values)
    case 0x19: { // Appearance
        if (ad.data.size() < 2) return std::nullopt;
        uint16_t appearance = static_cast<uint16_t>(ad.data[0]) |
                              (static_cast<uint16_t>(ad.data[1]) << 8);
        std::ostringstream ss;
        ss << "0x" << std::hex << std::setfill('0') << std::setw(4) << appearance
           << " (category=" << std::dec << (appearance >> 6) << ")";
        return ss.str();
    }
    // Core Spec Supplement Part A, Section 1.9
    case 0x12: { // Slave Connection Interval Range
        if (ad.data.size() < 4) return std::nullopt;
        uint16_t min_interval = static_cast<uint16_t>(ad.data[0]) |
                                (static_cast<uint16_t>(ad.data[1]) << 8);
        uint16_t max_interval = static_cast<uint16_t>(ad.data[2]) |
                                (static_cast<uint16_t>(ad.data[3]) << 8);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2)
           << (min_interval * 1.25) << "ms - " << (max_interval * 1.25) << "ms";
        return ss.str();
    }
    // Core Spec Supplement Part A, Section 1.2
    case 0x08: // Shortened Local Name
    case 0x09: { // Complete Local Name
        if (ad.data.empty()) return std::nullopt;
        return std::string(ad.data.begin(), ad.data.end());
    }
    default:
        return std::nullopt;
    }
}

} // namespace ble_sniffer
