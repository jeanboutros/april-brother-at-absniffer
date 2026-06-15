/**
 * @file airpods_label_test.cpp
 * @brief Runtime assertion tests for AirPods battery label formatting and
 *        out-of-range nibble handling.
 *
 * Tests verify that AirPods battery nibbles are displayed as percentages
 * (0-10 → "N*10% (raw=N)") and out-of-range nibbles (11-15) display as
 * "?? (raw=N)" per ADR-0007 and ADR-0008.
 *
 * All tests use runtime assert() — return 0 means all pass, non-zero means failure.
 */

#include <ble_sniffer/proprietary_parsers.h>
#include <ble_sniffer/types.h>
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

namespace {

/**
 * @brief Build a minimal AirPods manufacturer data payload.
 *
 * Layout (per furiousMAC/continuity):
 *   byte[0] = subtype (0x07 for AirPods)
 *   byte[1] = length
 *   byte[2] = reserved (0x00)
 *   byte[3] = device_model
 *   byte[4] = status byte
 *   byte[5] = reserved (0x00)
 *   byte[6] = battery byte (left nibble = bits 7:4, right nibble = bits 3:0)
 *   byte[7] = case byte (case nibble = bits 3:0, charging = bits 6:4, lid_open = bit 7)
 *
 * @param left_battery  Left earbud battery nibble (0-15)
 * @param right_battery Right earbud battery nibble (0-15)
 * @param case_battery  Case battery nibble (0-15)
 * @param charging      Charging status bits (0-7)
 * @param lid_open      Whether the case lid is open
 * @return A vector representing the AirPods manufacturer data.
 */
std::vector<uint8_t> make_airpods_data(uint8_t left_battery, uint8_t right_battery,
                                        uint8_t case_battery, uint8_t charging,
                                        bool lid_open) {
    std::vector<uint8_t> data(8);
    data[0] = 0x07;   // AirPods subtype
    data[1] = 0x06;   // length = 6
    data[2] = 0x00;   // reserved
    data[3] = 0x02;   // device_model (AirPods Pro)
    data[4] = 0x00;   // status byte
    data[5] = 0x00;   // reserved
    // Battery byte: left in high nibble, right in low nibble
    data[6] = static_cast<uint8_t>((left_battery << 4) | (right_battery & 0x0F));
    // Case byte: lid_open in bit 7, charging in bits 6:4, case_battery in bits 3:0
    data[7] = static_cast<uint8_t>(
        ((lid_open ? 1 : 0) << 7) | ((charging & 0x07) << 4) | (case_battery & 0x0F));
    return data;
}

/**
 * @brief Build a truncated AirPods payload (no battery/case bytes).
 *
 * @param num_bytes Number of bytes to include (must be < 7 for battery truncation,
 *                  < 8 for case truncation).
 */
std::vector<uint8_t> make_truncated_airpods(size_t num_bytes) {
    std::vector<uint8_t> data(num_bytes);
    if (num_bytes > 0) data[0] = 0x07;   // AirPods subtype
    if (num_bytes > 1) data[1] = 0x06;   // length
    if (num_bytes > 2) data[2] = 0x00;   // reserved
    if (num_bytes > 3) data[3] = 0x02;   // device_model
    if (num_bytes > 4) data[4] = 0x00;   // status
    if (num_bytes > 5) data[5] = 0x00;   // reserved
    if (num_bytes > 6) data[6] = 0x55;   // battery: left=5, right=5
    return data;
}

} // anonymous namespace

int main() {
    // === Test 1: Nibble 0 → "0% (raw=0)" (AC-5) ===
    {
        auto data = make_airpods_data(0, 0, 0, 0, false);
        auto result = ble_sniffer::proprietary::apple::parse(data);
        assert(result.has_value());
        assert(result->details.find("L=0% (raw=0)") != std::string::npos);
        assert(result->details.find("R=0% (raw=0)") != std::string::npos);
        assert(result->details.find("Case=0% (raw=0)") != std::string::npos);
    }

    // === Test 2: Nibble 5 → "50% (raw=5)" (AC-5) ===
    {
        auto data = make_airpods_data(5, 5, 5, 0, false);
        auto result = ble_sniffer::proprietary::apple::parse(data);
        assert(result.has_value());
        assert(result->details.find("L=50% (raw=5)") != std::string::npos);
        assert(result->details.find("R=50% (raw=5)") != std::string::npos);
        assert(result->details.find("Case=50% (raw=5)") != std::string::npos);
    }

    // === Test 3: Nibble 10 → "100% (raw=10)" (AC-5) ===
    {
        auto data = make_airpods_data(10, 10, 10, 0, false);
        auto result = ble_sniffer::proprietary::apple::parse(data);
        assert(result.has_value());
        assert(result->details.find("L=100% (raw=10)") != std::string::npos);
        assert(result->details.find("R=100% (raw=10)") != std::string::npos);
        assert(result->details.find("Case=100% (raw=10)") != std::string::npos);
    }

    // === Test 4: Nibble 11 → "?? (raw=11)" (AC-6) ===
    {
        auto data = make_airpods_data(11, 0, 0, 0, false);
        auto result = ble_sniffer::proprietary::apple::parse(data);
        assert(result.has_value());
        assert(result->details.find("L=?? (raw=11)") != std::string::npos);
    }

    // === Test 5: Nibble 15 → "?? (raw=15)" (AC-6) ===
    {
        auto data = make_airpods_data(15, 15, 15, 0, false);
        auto result = ble_sniffer::proprietary::apple::parse(data);
        assert(result.has_value());
        assert(result->details.find("L=?? (raw=15)") != std::string::npos);
        assert(result->details.find("R=?? (raw=15)") != std::string::npos);
        assert(result->details.find("Case=?? (raw=15)") != std::string::npos);
    }

    // === Test 6: Full AirPods parsing with left=5, right=3, case=8 (AC-5) ===
    {
        auto data = make_airpods_data(5, 3, 8, 0, false);
        auto result = ble_sniffer::proprietary::apple::parse(data);
        assert(result.has_value());
        assert(result->details.find("L=50% (raw=5)") != std::string::npos);
        assert(result->details.find("R=30% (raw=3)") != std::string::npos);
        assert(result->details.find("Case=80% (raw=8)") != std::string::npos);
        // Verify old format is NOT present
        assert(result->details.find("battery L=") == std::string::npos);
    }

    // === Test 7: Asymmetric L/R (left=10, right=0) (AC-5) ===
    {
        auto data = make_airpods_data(10, 0, 5, 0, false);
        auto result = ble_sniffer::proprietary::apple::parse(data);
        assert(result.has_value());
        assert(result->details.find("L=100% (raw=10)") != std::string::npos);
        assert(result->details.find("R=0% (raw=0)") != std::string::npos);
        assert(result->details.find("Case=50% (raw=5)") != std::string::npos);
    }

    // === Test 8: Truncated data (size < 7) — no battery fields (AC-5) ===
    {
        auto data = make_truncated_airpods(6);
        auto result = ble_sniffer::proprietary::apple::parse(data);
        assert(result.has_value());
        // Should NOT contain battery or case fields
        assert(result->details.find("L=") == std::string::npos);
        assert(result->details.find("R=") == std::string::npos);
        assert(result->details.find("Case=") == std::string::npos);
        // Should still contain model info
        assert(result->details.find("model=0x02") != std::string::npos);
    }

    // === Test 9: Truncated case data (size >= 7 but < 8) — battery but no case (AC-5) ===
    {
        auto data = make_truncated_airpods(7);
        auto result = ble_sniffer::proprietary::apple::parse(data);
        assert(result.has_value());
        // Should contain battery fields (left=5, right=5 from 0x55)
        assert(result->details.find("L=50% (raw=5)") != std::string::npos);
        assert(result->details.find("R=50% (raw=5)") != std::string::npos);
        // Should NOT contain case field
        assert(result->details.find("Case=") == std::string::npos);
    }

    return 0;  // All assertions passed
}
