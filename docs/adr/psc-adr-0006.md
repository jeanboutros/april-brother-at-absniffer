# ADR: Standardize BLE little-endian parsing with constexpr helper functions

**Status:** Accepted
**Date:** 2026-06-15
**Decision:** Add `le16()`, `le32()`, and `be16()` constexpr helper functions to `types.h` to prevent byte-order bugs, make parsing intent self-documenting, and eliminate DRY violations across 10+ call sites.

## Context

The codebase contains 10+ call sites across 4 files (`ad_parser.cpp`, `proprietary_parsers.cpp`, `stat_view.cpp`) that manually parse multi-byte BLE fields using inline `data[lo] | (data[hi] << 8)` expressions. Three of these sites (Samsung `device_type`, Sony `protocol_ver`, Razer `model`) were **buggy** — they used big-endian byte order `(data[0] << 8) | data[1]` instead of the BLE-mandated little-endian order `data[0] | (data[1] << 8)`.

The Bluetooth Core Specification (Vol 1, Part A, Section 1; Vol 6, Part B, §1.3.1) mandates that all multi-byte fields in the BLE protocol stack are transmitted in little-endian byte order. The existing code had no helper function to encode this convention — every developer had to remember the correct byte order and implement it manually, leading to the three bugs.

Additionally, the iBeacon parser uses big-endian for Apple's Major/Minor fields — a vendor-specific exception to the BLE convention. This intent was documented only in a comment `// (BE)`, making it vulnerable to a future developer "fixing" it to use `le16()`.

## Considered Alternatives

| Option | Pros | Cons |
|--------|------|------|
| **A: `le16(const uint8_t*)` — pointer-based, no bounds checking** | Minimal, maximally reusable; works with any contiguous buffer (vector, array, raw pointer); matches existing call-site pattern where bounds are already checked | Raw pointer API — callers can pass dangling/OOB pointers; no compile-time enforcement of buffer validity; UB if misused |
| B: `read_le16(const vector<uint8_t>&, size_t offset)` — vector+bounds | Bounds-checked; safer for new callers | Vector-specific (not reusable with arrays or raw buffers); introduces new failure mode (throw or silent return); redundant with existing call-site bounds checks |
| C: `read_le16(const uint8_t*, size_t len)` — pointer+length | Bounds-checked; works with any buffer type | Requires passing length at every call site; adds boilerplate; existing call sites already check bounds before accessing bytes |
| D: No helper — keep inline expressions | No new API surface | DRY violation continues; byte-order bugs remain possible; intent not self-documenting |
| **E: `be16()` — big-endian helper for iBeacon** | Self-documenting BE intent; prevents regression where iBeacon is "fixed" to LE; completes the byte-order helper suite; minimal effort (3 lines + 5 static_assert tests) | Adds one more public symbol; iBeacon is the only BE call site currently |

## Decision

**Chosen: Option A (`le16`/`le32` pointer-based) + Option E (`be16`).**

The `le16()` and `le32()` functions use a `const uint8_t*` parameter with no bounds checking. This is the minimal, maximally reusable form — it works with any contiguous buffer and matches the existing pattern where all 10 call sites already verify `size() >= offset + N` before accessing bytes. Adding bounds checking in the helper would be redundant with existing checks and introduce inconsistency (some callers would double-check, others wouldn't).

The `be16()` function is added alongside `le16()`/`le32()` to complete the byte-order helper suite. The iBeacon parser is the only big-endian call site in the codebase, and `be16()` makes the BE intent self-documenting in code — not just in comments. This prevents a future developer from "fixing" the iBeacon code to use `le16()`, which would introduce a regression.

### Type Design Self-Assessment (Corrected per Challenger F1)

The challenger review identified that the primary's type design score of 10.0/10 was inflated. The corrected assessment:

| Dimension | Score | Justification |
|-----------|-------|---------------|
| Encapsulation | 8 | `const uint8_t*` parameter allows invalid states (dangling pointer, insufficient buffer length). The function cannot verify preconditions. This is appropriate for a low-level byte-parsing utility but does not meet the "users cannot create invalid states" criterion for a 10. |
| Invariant Expression | 7 | `constexpr` enables compile-time evaluation when inputs are compile-time constants, but the primary use case (parsing runtime BLE advertisement buffers) has zero compile-time verification. Invalid states (OOB pointer) are not rejected at compile time. |
| Usefulness | 9 | Self-documenting name (`le16`/`be16`/`le32`) makes byte-order intent immediately clear. The pointer API is less debuggable than a bounded API (can't inspect source buffer from signature), but the naming convention is strong. |
| Enforcement | 6 | `const uint8_t*` is a raw type. There is no type enforcement that the caller passes a valid pointer with sufficient bytes. The type-design-review skill defines Enforcement 10 as "All public API surface uses typed enums and struct wrappers." This API is a raw pointer — appropriate for its role but not type-enforced. |
| **Overall** | **7.5** | Acceptable for a low-level byte-parsing utility. The API is correct for its use case, but the self-assessment must honestly acknowledge the pointer-based tradeoffs. |

### Bounds-Checking Documentation (Challenger F3)

The Doxygen for all three helpers uses `@warning` (not `@note`) for the bounds-checking requirement:

```cpp
@warning Undefined behavior if data points to fewer than N consecutive bytes.
         Callers MUST ensure sufficient buffer length before calling.
```

This is the strongest Doxygen tag available and accurately reflects the consequence of misuse (buffer overread → UB).

## Consequences

### What becomes easier
- **Bug prevention:** New BLE parsers use `le16()`/`le32()` by default — byte-order errors are impossible if the helper is used correctly.
- **Self-documenting code:** `le16(&data[offset])` immediately communicates "parse a little-endian 16-bit value" without needing a comment.
- **DRY compliance:** 10 duplicate inline expressions replaced with 3 helper functions. If the byte-order convention ever needs to change (e.g., for a new protocol), only the helpers need updating.
- **iBeacon safety:** `be16()` makes the big-endian exception explicit in code. A future developer cannot accidentally "fix" iBeacon to LE.
- **Code review:** Reviewers can grep for `(data[` patterns to find any remaining manual byte-order parsing that should use helpers.

### What becomes harder
- **Nothing.** The helpers are additive — existing code continues to work. The refactoring is behavior-preserving for all correct call sites.

### What is blocked
- **Nothing.** The helpers are pure `constexpr` functions in the lowest-level header (`types.h`), which all modules can depend on.

### Risks
- **Pointer misuse:** Future callers could call `le16()` without bounds checking. Mitigated by the `@warning` Doxygen and the existing pattern of bounds-checking at all call sites. A future `read_le16()` bounded variant could be added if needed.
- **Alignment assumptions:** Mitigated by byte-level access (not `reinterpret_cast`), documented with `@note`: "Safe for unaligned access — uses byte-level reads, not pointer cast."
