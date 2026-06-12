#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace ble_sniffer::proprietary {

struct ParseResult {
    std::string description;
    std::string details;
};

namespace apple {
std::optional<ParseResult> parse(const std::vector<uint8_t>& mfr_data);
} // namespace apple

namespace samsung {
std::optional<ParseResult> parse(const std::vector<uint8_t>& mfr_data);
} // namespace samsung

namespace microsoft {
std::optional<ParseResult> parse(const std::vector<uint8_t>& mfr_data);
} // namespace microsoft

namespace sony {
std::optional<ParseResult> parse(const std::vector<uint8_t>& mfr_data);
} // namespace sony

namespace sonos {
std::optional<ParseResult> parse(const std::vector<uint8_t>& mfr_data);
} // namespace sonos

namespace garmin {
std::optional<ParseResult> parse(const std::vector<uint8_t>& mfr_data);
} // namespace garmin

namespace razer {
std::optional<ParseResult> parse(const std::vector<uint8_t>& mfr_data);
} // namespace razer

namespace furbo {
std::optional<ParseResult> parse(const std::vector<uint8_t>& mfr_data);
} // namespace furbo

std::optional<std::string> decode_proprietary(uint16_t company_id, const std::vector<uint8_t>& payload);

/// Convenience: decode directly from raw manufacturer-specific AD data (including the 2-byte company ID prefix).
std::optional<std::string> decode_proprietary(const std::vector<uint8_t>& mfr_ad_data);

/// Structured variant: returns the per-vendor ParseResult so callers can use the description (e.g. "Find My") and details separately.
std::optional<ParseResult> decode_proprietary_parts(uint16_t company_id, const std::vector<uint8_t>& payload);

/// @overload Structured decode from raw manufacturer-specific AD data (including the 2-byte company ID prefix).
std::optional<ParseResult> decode_proprietary_parts(const std::vector<uint8_t>& mfr_ad_data);

} // namespace ble_sniffer::proprietary
