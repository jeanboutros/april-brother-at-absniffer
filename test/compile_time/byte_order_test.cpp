/**
 * @file byte_order_test.cpp
 * @brief Runtime assertion tests for byte-order fixes in proprietary parsers
 *        and AD structure parsers.
 *
 * Tests verify that Samsung, Sony, Razer, and iBeacon parsers use the correct
 * le16()/be16() helpers from types.h instead of hard-coded big-endian shifts.
 *
 * Tests 10-15 verify the 6 refactored call sites in ad_parser.cpp:
 *   - Company ID (line 40): le16(&ad.data[0]) for AD type 0xFF
 *   - 16-bit UUID (line 88): le16(&ad.data[i]) for AD types 0x02/0x03
 *   - 32-bit UUID (line 102): le32(&ad.data[i]) for AD types 0x04/0x05
 *   - Appearance (line 140): le16(&ad.data[0]) for AD type 0x19
 *   - CI min_interval (line 149): le16(&ad.data[0]) for AD type 0x12
 *   - CI max_interval (line 150): le16(&ad.data[2]) for AD type 0x12
 *
 * All tests use runtime assert() — return 0 means all pass, non-zero means failure.
 */

#include <ble_sniffer/ad_parser.h>
#include <ble_sniffer/assigned_numbers.h>
#include <ble_sniffer/proprietary_parsers.h>
#include <ble_sniffer/types.h>
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

namespace {

/** Build a minimal 22-byte iBeacon payload for testing Major/Minor byte order. */
std::vector<uint8_t> make_ibeacon(uint16_t major, uint16_t minor) {
    std::vector<uint8_t> data(22);
    data[0] = 0x02;   // iBeacon subtype
    data[1] = 0x15;   // length = 21
    // UUID bytes 2-17: zeros (valid placeholder)
    for (int i = 2; i < 18; ++i) data[i] = 0x00;
    data[18] = static_cast<uint8_t>((major >> 8) & 0xFF);   // Major BE high byte
    data[19] = static_cast<uint8_t>(major & 0xFF);           // Major BE low byte
    data[20] = static_cast<uint8_t>((minor >> 8) & 0xFF);   // Minor BE high byte
    data[21] = static_cast<uint8_t>(minor & 0xFF);           // Minor BE low byte
    return data;
}

} // anonymous namespace

int main() {
    // === Test 1: Samsung device_type uses le16 (AC-1) ===
    // Input: type_byte=0x42, device_type LE bytes {0x04, 0x00} → 0x0004
    {
        std::vector<uint8_t> samsung_data = {0x42, 0x04, 0x00};
        auto result = ble_sniffer::proprietary::samsung::parse(samsung_data);
        assert(result.has_value());
        assert(result->details.find("device_type=0x0004") != std::string::npos);
    }

    // === Test 2: Samsung device_type with non-zero MS byte (LE) ===
    // Input: type_byte=0x42, device_type LE bytes {0xFF, 0x00} → 0x00FF
    {
        std::vector<uint8_t> samsung_data2 = {0x42, 0xFF, 0x00};
        auto result2 = ble_sniffer::proprietary::samsung::parse(samsung_data2);
        assert(result2.has_value());
        assert(result2->details.find("device_type=0x00ff") != std::string::npos);
    }

    // === Test 3: Sony protocol_ver uses le16 (AC-2) ===
    // Input: {0x34, 0x12} → LE → 0x1234
    {
        std::vector<uint8_t> sony_data = {0x34, 0x12};
        auto result = ble_sniffer::proprietary::sony::parse(sony_data);
        assert(result.has_value());
        assert(result->details.find("protocol=0x1234") != std::string::npos);
    }

    // === Test 4: Razer model uses le16 (AC-3) ===
    // Input: {0xCD, 0xAB} → LE → 0xABCD
    {
        std::vector<uint8_t> razer_data = {0xCD, 0xAB};
        auto result = ble_sniffer::proprietary::razer::parse(razer_data);
        assert(result.has_value());
        assert(result->details.find("model=0xabcd") != std::string::npos);
    }

    // === Test 5: iBeacon Major uses be16 (AC-4) ===
    // Major BE bytes {0x12, 0x34} → decimal 4660
    {
        auto ibeacon_data = make_ibeacon(0x1234, 0x0000);
        auto result = ble_sniffer::proprietary::apple::parse(ibeacon_data);
        assert(result.has_value());
        assert(result->details.find("Major=4660") != std::string::npos);
    }

    // === Test 6: iBeacon Minor uses be16 (AC-4) ===
    // Minor BE bytes {0x56, 0x78} → decimal 22136
    {
        auto ibeacon_data = make_ibeacon(0x0000, 0x5678);
        auto result = ble_sniffer::proprietary::apple::parse(ibeacon_data);
        assert(result.has_value());
        assert(result->details.find("Minor=22136") != std::string::npos);
    }

    // === Test 7: company_id uses le16 — Apple 0x004C route (AC-7) ===
    // mfr_ad_data: {0x4C, 0x00, 0x02, ...} → le16 → 0x004C → Apple → iBeacon
    {
        auto ibeacon_data = make_ibeacon(0x0000, 0x0000);
        std::vector<uint8_t> mfr_ad_data;
        mfr_ad_data.push_back(0x4C);  // company_id low byte (LE)
        mfr_ad_data.push_back(0x00);  // company_id high byte (LE)
        mfr_ad_data.insert(mfr_ad_data.end(), ibeacon_data.begin(), ibeacon_data.end());

        auto result = ble_sniffer::proprietary::decode_proprietary_parts(mfr_ad_data);
        assert(result.has_value());
        assert(result->description == "iBeacon");
    }

    // === Test 8: company_id uses le16 — Samsung 0x0075 route (AC-7) ===
    // mfr_ad_data: {0x75, 0x00, 0x42, 0x04, 0x00} → le16 → 0x0075 → Samsung
    {
        std::vector<uint8_t> mfr_ad_data = {0x75, 0x00, 0x42, 0x04, 0x00};
        auto result = ble_sniffer::proprietary::decode_proprietary_parts(mfr_ad_data);
        assert(result.has_value());
        assert(result->description.find("Samsung") != std::string::npos);
    }

    // === Test 9: iBeacon be16 correctness discriminator (AC-4) ===
    // Major BE bytes {0x01, 0x02} → should be 258
    // If mistakenly parsed as LE: {0x01, 0x02} → 0x0201 = 513
    // This test catches accidental le16 usage on iBeacon fields.
    {
        auto ibeacon_data = make_ibeacon(0x0102, 0x0304);
        auto result = ble_sniffer::proprietary::apple::parse(ibeacon_data);
        assert(result.has_value());
        assert(result->details.find("Major=258") != std::string::npos);
        assert(result->details.find("Minor=772") != std::string::npos);
        // Verify the wrong values (le16 interpretation) are NOT present
        assert(result->details.find("Major=513") == std::string::npos);  // 0x0201
        assert(result->details.find("Minor=1027") == std::string::npos); // 0x0403
    }

    // =========================================================================
    // Tests 10-15: AD parser byte-order regression tests
    // Verify the 6 refactored call sites in ad_parser.cpp
    // =========================================================================

    // === Test 10: Company ID uses le16 — ad_parser.cpp line 40 ===
    // AD type 0xFF (Manufacturer Specific Data)
    // Raw AD: length=3, type=0xFF, data={0x4C, 0x00}
    // le16({0x4C, 0x00}) → 0x004C → "Apple, Inc."
    // If mistakenly parsed as BE: be16({0x4C, 0x00}) → 0x4C00 → some other company
    {
        std::vector<uint8_t> adv_data = {0x03, 0xFF, 0x4C, 0x00};
        auto result = ble_sniffer::decode_manufacturer(adv_data);
        assert(result.has_value());
        assert(*result == "Apple, Inc.");
        // Discriminator: BE interpretation would give 0x4C00, not Apple
        assert(*result != "Samsung Electronics Co. Ltd.");
    }

    // === Test 11: 16-bit UUID uses le16 — ad_parser.cpp line 88 ===
    // AD type 0x03 (Complete List of 16-bit Service UUIDs)
    // Raw AD: length=5, type=0x03, data={0x34, 0x12, 0xCD, 0xAB}
    // le16({0x34, 0x12}) → 0x1234
    // le16({0xCD, 0xAB}) → 0xABCD
    // If mistakenly parsed as BE: {0x34, 0x12} → 0x3412, {0xCD, 0xAB} → 0xCDAB
    {
        std::vector<uint8_t> adv_data = {0x05, 0x03, 0x34, 0x12, 0xCD, 0xAB};
        auto structures = ble_sniffer::parse_ad_structures(adv_data);
        assert(structures.size() == 1);
        assert(structures[0].type == 0x03);

        auto result = ble_sniffer::decode_ad_data(structures[0]);
        assert(result.has_value());
        assert(result->find("0x1234") != std::string::npos);
        assert(result->find("0xabcd") != std::string::npos);
        // Discriminator: BE interpretation must NOT be present
        assert(result->find("0x3412") == std::string::npos);
        assert(result->find("0xcdab") == std::string::npos);
    }

    // === Test 12: 32-bit UUID uses le32 — ad_parser.cpp line 102 ===
    // AD type 0x05 (Complete List of 32-bit Service UUIDs)
    // Raw AD: length=5, type=0x05, data={0x78, 0x56, 0x34, 0x12}
    // le32({0x78, 0x56, 0x34, 0x12}) → 0x12345678
    // If mistakenly parsed as BE: {0x78, 0x56, 0x34, 0x12} → 0x78563412
    {
        std::vector<uint8_t> adv_data = {0x05, 0x05, 0x78, 0x56, 0x34, 0x12};
        auto structures = ble_sniffer::parse_ad_structures(adv_data);
        assert(structures.size() == 1);
        assert(structures[0].type == 0x05);

        auto result = ble_sniffer::decode_ad_data(structures[0]);
        assert(result.has_value());
        assert(result->find("0x12345678") != std::string::npos);
        // Discriminator: BE interpretation must NOT be present
        assert(result->find("0x78563412") == std::string::npos);
    }

    // === Test 13: Appearance uses le16 — ad_parser.cpp line 140 ===
    // AD type 0x19 (Appearance)
    // Raw AD: length=3, type=0x19, data={0x40, 0x03}
    // le16({0x40, 0x03}) → 0x0340 → category = 0x0340 >> 6 = 13
    // If mistakenly parsed as BE: {0x40, 0x03} → 0x4003 → category = 0x4003 >> 6 = 256
    {
        std::vector<uint8_t> adv_data = {0x03, 0x19, 0x40, 0x03};
        auto structures = ble_sniffer::parse_ad_structures(adv_data);
        assert(structures.size() == 1);
        assert(structures[0].type == 0x19);

        auto result = ble_sniffer::decode_ad_data(structures[0]);
        assert(result.has_value());
        assert(result->find("0x0340") != std::string::npos);
        assert(result->find("category=13") != std::string::npos);
        // Discriminator: BE interpretation must NOT be present
        assert(result->find("0x4003") == std::string::npos);
        assert(result->find("category=256") == std::string::npos);
    }

    // === Test 14: CI min_interval uses le16 — ad_parser.cpp line 149 ===
    // AD type 0x12 (Slave Connection Interval Range)
    // Raw AD: length=5, type=0x12, data={0x06, 0x00, 0x0C, 0x00}
    // le16({0x06, 0x00}) → 0x0006 = 6 → 6 * 1.25 = 7.50ms (min)
    // le16({0x0C, 0x00}) → 0x000C = 12 → 12 * 1.25 = 15.00ms (max)
    // If mistakenly parsed as BE for min: {0x06, 0x00} → 0x0600 = 1536 → 1920.00ms
    {
        std::vector<uint8_t> adv_data = {0x05, 0x12, 0x06, 0x00, 0x0C, 0x00};
        auto structures = ble_sniffer::parse_ad_structures(adv_data);
        assert(structures.size() == 1);
        assert(structures[0].type == 0x12);

        auto result = ble_sniffer::decode_ad_data(structures[0]);
        assert(result.has_value());
        // Verify min_interval (offset 0, le16)
        assert(result->find("7.50ms") != std::string::npos);
        // Discriminator: BE interpretation of min must NOT be present
        assert(result->find("1920.00ms") == std::string::npos);
    }

    // === Test 15: CI max_interval uses le16 — ad_parser.cpp line 150 ===
    // Same AD structure as Test 14, verifying max_interval at offset 2
    // le16({0x0C, 0x00}) → 0x000C = 12 → 12 * 1.25 = 15.00ms (max)
    // If mistakenly parsed as BE for max: {0x0C, 0x00} → 0x0C00 = 3072 → 3840.00ms
    {
        std::vector<uint8_t> adv_data = {0x05, 0x12, 0x06, 0x00, 0x0C, 0x00};
        auto structures = ble_sniffer::parse_ad_structures(adv_data);
        assert(structures.size() == 1);
        assert(structures[0].type == 0x12);

        auto result = ble_sniffer::decode_ad_data(structures[0]);
        assert(result.has_value());
        // Verify max_interval (offset 2, le16)
        assert(result->find("15.00ms") != std::string::npos);
        // Discriminator: BE interpretation of max must NOT be present
        assert(result->find("3840.00ms") == std::string::npos);
    }

    return 0;  // All assertions passed
}
