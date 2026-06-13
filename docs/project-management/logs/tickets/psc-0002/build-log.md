# B1→B2→B3: Fix std::stoi crash vectors in ScanResultMessage::from()

| Field | Value |
|-------|-------|
| Agent | code-architect |
| Timestamp | 2026-06-13T12:00:00Z |
| Step | B1→B2-1→B3 |

### B1: PLAN

| Units declared | 1 |
| Unit descriptions | Add safe_stoi() helper + replace all 4 bare std::stoi calls in ScanResultMessage::from() |
| Files identified | src/messages.cpp |

### B2-1: APPLY Unit 1

| Unit | 1 |
| Build result | PASS — exit 0, 0 warnings |
| Files changed | src/messages.cpp (+46 lines, -4 lines) |

**Changes made:**

1. Added `#include <climits>` for `INT_MIN`/`INT_MAX` constants
2. Added `safe_stoi()` function in anonymous namespace — wraps `std::stol` in try/catch, validates at least one character consumed and result fits in `int`, returns `default_val` on any failure
3. Line 128 (RSSI): `std::stoi(...)` → `safe_stoi(...)`
4. Line 135 (adv_type): `static_cast<AdvertisementType>(std::stoi(...))` → range-validated `safe_stoi()` with [0,4] check, default `CONNECTABLE_UNDIRECTED`
5. Line 141 (data_length): `std::stoi(...)` → `safe_stoi(...)`
6. Line 148 (hex byte): `std::stoi(..., nullptr, 16)` → `safe_stoi(..., 16, -1)` with range check [0,0xFF], skip on failure

### B3: VALIDATE

| Full build | PASS — exit 0, 0 warnings |
| AC coverage | 7/7 acceptance criteria satisfied |

**Acceptance Criteria:**

| # | Criterion | Status |
|---|-----------|--------|
| AC1 | No std::stoi call can throw uncaught exception | ✅ PASS |
| AC2 | AdvertisementType only static_cast from [0,4]; default CONNECTABLE_UNDIRECTED | ✅ PASS |
| AC3 | Malformed RSSI defaults to 0 | ✅ PASS |
| AC4 | Malformed adv_type defaults to CONNECTABLE_UNDIRECTED | ✅ PASS |
| AC5 | Malformed hex pair is skipped | ✅ PASS |
| AC6 | Malformed data_length defaults to 0 | ✅ PASS |
| AC7 | Build passes with zero warnings | ✅ PASS |

### Self-Reflection

1. **Why was this bug missed?** — The original code used `std::stoi` on untrusted serial data without exception handling. This is a common C++ pitfall — `std::stoi` is deceptively convenient but throws on malformed input. No code review caught it because the review likely assumed well-formed protocol data.

2. **What procedural safeguard would have caught it?** — A policy of "never use `std::stoi` on untrusted input" — or a code review checklist item checking that every `std::stoi`/`std::stol`/`std::stoul` call either has a try/catch or operates on validated data.

3. **Update the knowledge base** — Added lesson: when parsing serial/network protocol data, always use a safe conversion wrapper. `std::stoi` family is only safe for trusted input.