# B2-1: APPLY Unit 1 — Byte-Order Helper Functions + Test Infrastructure

| Field | Value |
|-------|-------|
| Agent | code-architect |
| Timestamp | 2026-06-14T23:30:00Z |
| Step | B2-1 |
| Unit | 1 of 7 |

## Implementation Summary

### Files Changed

| File | Change | Lines |
|------|--------|-------|
| `include/ble_sniffer/types.h` | Added `#include <cstdint>` + 3 helper functions (`le16`, `le32`, `be16`) with full Doxygen | +82 lines (29→328) |
| `test/CMakeLists.txt` | New file — CMake targets for test executables | +10 lines |
| `test/compile_time/le_helpers_test.cpp` | New file — 10 `static_assert` tests for all 3 helpers | +42 lines |
| `CMakeLists.txt` | Added `add_subdirectory(test)` | +3 lines (34→37) |

### Functions Implemented

| Function | Signature | Description |
|----------|-----------|-------------|
| `le16` | `inline constexpr uint16_t le16(const uint8_t* data) noexcept` | Little-endian 16-bit parse (LSB at data[0]) |
| `le32` | `inline constexpr uint32_t le32(const uint8_t* data) noexcept` | Little-endian 32-bit parse (LSB at data[0]) |
| `be16` | `inline constexpr uint16_t be16(const uint8_t* data) noexcept` | Big-endian 16-bit parse (MSB at data[0]) |

### Doxygen Coverage

All 3 functions have:
- `@brief` — one-sentence summary
- `@param data` — pointer description
- `@return` — return value description
- `@warning` — UB if insufficient buffer length
- `@note` — safe for unaligned access
- `@example` / `@code` — usage example with typed vocabulary
- `@see` (le16 only) — BLE Core Spec reference

### Test Coverage

10 `static_assert` tests in `le_helpers_test.cpp`:

| # | Function | Input | Expected | Status |
|---|----------|-------|----------|--------|
| 1 | `le16` | `{0x34, 0x12}` | `0x1234` | PASS |
| 2 | `le16` | `{0x00, 0x00}` | `0x0000` | PASS |
| 3 | `le16` | `{0xFF, 0xFF}` | `0xFFFF` | PASS |
| 4 | `le16` | `{0x01, 0x00}` | `0x0001` | PASS |
| 5 | `le16` | `{0x00, 0x80}` | `0x8000` | PASS |
| 6 | `be16` | `{0x12, 0x34}` | `0x1234` | PASS |
| 7 | `be16` | `{0x12, 0x00}` | `0x1200` | PASS |
| 8 | `le32` | `{0x78, 0x56, 0x34, 0x12}` | `0x12345678` | PASS |
| 9 | `le32` | `{0x00, 0x00, 0x00, 0x00}` | `0x00000000` | PASS |
| 10 | `le32` | `{0xFF, 0xFF, 0xFF, 0xFF}` | `0xFFFFFFFF` | PASS |

### Build Verification

```
cmake --preset conan-debug → Configuring done (0.1s)
cmake --build --preset conan-debug → [100%] Built target le_helpers_test
Exit code: 0
Warnings: 0
./build/Debug/test/le_helpers_test → EXIT: 0
```

### T1 Compliance

| Check | Result |
|-------|--------|
| T1.1 Build passes | PASS — exit 0, zero warnings |
| T1.2 Doc-standard on public symbols | PASS — all 3 functions have `@brief`, `@param`, `@return`, `@warning`, `@note`, `@example` |
| T1.3 No decision references | PASS — no `D-\d`, `F-\d`, `(decision` patterns |
| T1.4 No changelog-style comments | PASS — no `replaces the`, `was previously`, `formerly` |
| T1.5 No raw integers in public API | PASS — pointer-based API appropriate for byte utilities |
| T1.6 No magic numbers in doc examples | PASS — examples use hex literals with explanation |
| T1.7 Constants in correct module | PASS — helpers in `types.h` (lowest-level header) |
| T1.8 Reserved/padding fields handled | N/A — not applicable |
| T1.9 No hardcoded secrets | PASS — no secret patterns found |

### Issue Resolved During Implementation

**Compound literal lifetime in `static_assert`:** The initial test file used `(const uint8_t[]){0x34, 0x12}` compound literals, which Clang rejected in `static_assert` context because the temporary array's lifetime ends before the `static_assert` evaluation. Fixed by using named `constexpr uint8_t` arrays with file-scope lifetime.

**Self-reflection:**
1. **Why was this bug missed?** The A2 design proposal used compound literals in the test examples. Compound literals are a C99 feature with different lifetime rules in C++ — in C++ they create temporaries that don't survive the full expression. The design didn't account for this C/C++ language difference.
2. **What procedural safeguard would have caught it?** A "compile the test file before declaring the design complete" step in Phase A. The test examples in the design proposal should have been verified against the actual compiler.
3. **Knowledge base update:** This lesson should be added to `tdd-cpp` skill: "Use named `constexpr` variables for `static_assert` test data, not compound literals. Compound literals have C99 lifetime semantics that don't work in C++ `static_assert` context."

### Acceptance Criteria Satisfied

- AC-8: `le16()`, `le32()`, `be16()` have full Doxygen with `@warning` for bounds ✅
- AC-9 (partial): `le_helpers_test.cpp` compiles and passes ✅ (byte_order_test and airpods_label_test deferred to U2/U3)

### Next Unit

U2: Fix Samsung/Sony/Razer byte order (BE→LE) in `proprietary_parsers.cpp` + broken URL fixes + iBeacon comment
