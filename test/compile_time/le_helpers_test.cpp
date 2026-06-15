/**
 * @file le_helpers_test.cpp
 * @brief Compile-time verification of le16(), le32(), and be16() helpers.
 *
 * All tests use static_assert — compilation success = all tests pass.
 * No runtime execution needed.
 */

#include <ble_sniffer/types.h>
#include <cstdint>

// --- le16() tests ---

constexpr uint8_t le16_basic[] = {0x34, 0x12};
static_assert(ble_sniffer::le16(le16_basic) == 0x1234,
              "le16 basic: {0x34, 0x12} → 0x1234");

constexpr uint8_t le16_zero[] = {0x00, 0x00};
static_assert(ble_sniffer::le16(le16_zero) == 0x0000,
              "le16 zero: {0x00, 0x00} → 0x0000");

constexpr uint8_t le16_max[] = {0xFF, 0xFF};
static_assert(ble_sniffer::le16(le16_max) == 0xFFFF,
              "le16 max: {0xFF, 0xFF} → 0xFFFF");

constexpr uint8_t le16_lsb_only[] = {0x01, 0x00};
static_assert(ble_sniffer::le16(le16_lsb_only) == 0x0001,
              "le16 LSB-only: {0x01, 0x00} → 0x0001");

constexpr uint8_t le16_msb_only[] = {0x00, 0x80};
static_assert(ble_sniffer::le16(le16_msb_only) == 0x8000,
              "le16 MSB-only: {0x00, 0x80} → 0x8000");

// --- be16() tests ---

constexpr uint8_t be16_basic[] = {0x12, 0x34};
static_assert(ble_sniffer::be16(be16_basic) == 0x1234,
              "be16 basic: {0x12, 0x34} → 0x1234");

constexpr uint8_t be16_msb_only[] = {0x12, 0x00};
static_assert(ble_sniffer::be16(be16_msb_only) == 0x1200,
              "be16 MSB-only: {0x12, 0x00} → 0x1200");

// --- le32() tests ---

constexpr uint8_t le32_basic[] = {0x78, 0x56, 0x34, 0x12};
static_assert(ble_sniffer::le32(le32_basic) == 0x12345678,
              "le32 basic: {0x78, 0x56, 0x34, 0x12} → 0x12345678");

constexpr uint8_t le32_zero[] = {0x00, 0x00, 0x00, 0x00};
static_assert(ble_sniffer::le32(le32_zero) == 0x00000000,
              "le32 zero: all zeros → 0x00000000");

constexpr uint8_t le32_max[] = {0xFF, 0xFF, 0xFF, 0xFF};
static_assert(ble_sniffer::le32(le32_max) == 0xFFFFFFFF,
              "le32 max: all 0xFF → 0xFFFFFFFF");

int main() { return 0; }  // compilation = test pass
