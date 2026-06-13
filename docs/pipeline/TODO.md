# Task Tracker

### Full Codebase Review — ABSniffer 528 BLE AT Driver Library + CLI
- **Status:** [~] active
- **Ticket:** psc-0001
- **Passport:** docs/project-management/passports/psc-0001-passport.md
- **Acceptance Criteria:**
  1. Skill gap analysis complete — missing skills identified and flagged for recruitment
  2. Codebase issues catalogued — logical bugs, memory leaks, missing documentation, silent failures
  3. Pending build items identified — incomplete features from README.md TODO list, broken builds, missing tests
  4. Feature proposals produced — relevant, useful additions grounded in codebase domain (BLE, serial, AT commands)
  5. Change recommendations issued — per-file or per-module suggestions with justification
- **Files:** include/ble_sniffer/*.h, src/*.cpp, src/*.h, CMakeLists.txt, conanfile.py, README.md
- **Dependencies:** none (first task)
- **Assigned to:** SW, TX, DX, HW, WX, SX (6 specialists for Phase A review)
- **Domain Classification:** hardware (serial port, termios) + wireless (BLE advertising, AT commands) + security (serial I/O, external input parsing)
- **Specialist Roster:** Software Engineer, Test Engineer, Docs Writer, Hardware Engineer, Wireless Expert, Security Reviewer

---

### T-0001: Fix std::stoi crash vectors in ScanResultMessage::from()

**Priority:** Critical
**Files:** src/messages.cpp:128,135,141,148
**Specialist:** code-architect

**Issue Description:**
Four `std::stoi` calls in `ScanResultMessage::from()` process untrusted data from a serial BLE device without exception handling. If the device sends malformed data (non-numeric characters in RSSI, adv_type, or data_length fields, or invalid hex in adv_data), `std::stoi` throws `std::invalid_argument` or `std::out_of_range`, crashing the program. All data from the serial device is untrusted (OWASP A03 — Injection).

**Issue Analysis:**
```cpp
// Line 128 — RSSI parsing — CRASHES on non-numeric input
msg.m_rssi = std::stoi(remaining.substr(0, pos));

// Line 135 — adv_type parsing — CRASHES + creates invalid enum
msg.m_adv_type = static_cast<AdvertisementType>(std::stoi(remaining.substr(0, pos)));
// If device sends 99, creates invalid AdvertisementType → UB in switch

// Line 142 — data_length parsing — CRASHES on non-numeric input
msg.m_data_length = std::stoi(remaining.substr(0, pos));

// Line 148 — hex byte parsing — CRASHES on non-hex chars
uint8_t byte = static_cast<uint8_t>(std::stoi(remaining.substr(i, 2), nullptr, 16));
```

**Change Suggested:**
- Create a `safe_stoi()` helper that returns `std::optional<int>` (or sets a default on failure)
- Wrap all 4 `std::stoi` calls in try/catch, setting safe defaults on failure
- Add range validation for `AdvertisementType` before `static_cast` (valid range: 0-4)
- On parse failure, set sensible defaults (rssi=0, adv_type=CONNECTABLE_UNDIRECTED, data_length=0, skip bad hex pairs)

**Acceptance Criteria:**
- [ ] No `std::stoi` call in `ScanResultMessage::from()` can throw an uncaught exception
- [ ] `AdvertisementType` is only `static_cast` from integers in range [0,4]
- [ ] Malformed RSSI field defaults to 0, not crash
- [ ] Malformed adv_type defaults to CONNECTABLE_UNDIRECTED, not UB
- [ ] Malformed hex pair is skipped, not crashed
- [ ] Build passes with zero warnings

**Execution Plan:**
1. Create `safe_stoi()` helper in messages.cpp (or a utility header)
2. Replace bare `std::stoi` calls at lines 128, 135, 141, 148
3. Add AdvertisementType range check at line 135
4. Build and verify
5. Test manually with malformed serial data (or add unit tests when framework is ready)

---

### T-0002: Fix silent error handling in BluetoothATDriver

**Priority:** Critical
**Files:** src/bluetooth_at_driver.cpp:89,17-19,94-97,21-29; include/ble_sniffer/bluetooth_at_driver.h
**Specialist:** code-architect

**Issue Description:**
Multiple error handling failures in the serial driver:
1. `write()` return value is ignored — commands silently dropped on error
2. `init()` failure is invisible to callers — constructor completes with `fd=-1`, subsequent calls cause UB
3. `read_buffer` grows unbounded — OOM from streaming device without `\r\n`
4. Destructor calls `stop_scan()`/`reset_device()` on potentially invalid fd

**Issue Analysis:**
```cpp
// Line 89 — write() return ignored — command silently lost
write(file_descriptor, full_command.c_str(), full_command.size());

// Lines 32-36 — init() failure sets fd=-1 but constructor completes
void BluetoothATDriver::init() {
    file_descriptor = open(m_device.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (file_descriptor < 0) {
        std::cerr << "Error opening device: " << m_device << std::endl;
        return;  // ← Object in invalid state, caller can't detect this
    }
    // ...
}

// Lines 94-97 — read_buffer unbounded
ssize_t n = read(file_descriptor, buf, sizeof(buf));
if (n > 0) {
    read_buffer.append(buf, n);  // ← No size limit!
}

// Lines 21-28 — destructor sends AT commands on bad fd
BluetoothATDriver::~BluetoothATDriver() {
    if (file_descriptor >= 0) {
        stop_scan();     // ← Sends AT command even if device disconnected
        reset_device();  // ← Same
        close(file_descriptor);
    }
}
```

**Change Suggested:**
1. Check `write()` return value: store result, log error on failure, add `ssize_t written = write(...)` and compare against `full_command.size()`
2. Change `init()` to return `bool`; add `is_open() const` accessor; all public methods check `file_descriptor < 0` and return error sentinels
3. Add `MAX_READ_BUFFER = 65536` constant; check `read_buffer.size()` after append; clear and return error on overflow
4. Guard destructor: only call `stop_scan()`/`reset_device()` if fd is valid AND device was known responsive; suppress errors in destructor path

**Acceptance Criteria:**
- [ ] `write()` return value is checked in `send_command()`; error is logged
- [ ] `init()` returns `bool` indicating success/failure
- [ ] `is_open()` public accessor exists and returns `file_descriptor >= 0`
- [ ] All public methods (read_line, send_command, query_*, start_scan, stop_scan, etc.) check `is_open()` and return error sentinels when closed
- [ ] `read_buffer` has a maximum size cap (64KB); overflow returns `RawMessage::error()`
- [ ] Destructor only calls `stop_scan()`/`reset_device()` if fd is valid
- [ ] Build passes with zero warnings

**Execution Plan:**
1. Add `is_open()` method to header
2. Change `init()` return type to `bool`; update constructor
3. Add fd validity checks to all public methods
4. Add `write()` return value check
5. Add `MAX_READ_BUFFER` constant and overflow check
6. Guard destructor cleanup calls
7. Build and verify

---

### T-0003: Fix VTIME integer division truncation

**Priority:** High
**Files:** src/bluetooth_at_driver.cpp:70
**Specialist:** code-architect

**Issue Description:**
`VTIME = sniffer_timeout / 100` uses integer division. With `sniffer_timeout = 1000` this yields VTIME=10 (correct). But if anyone changes `sniffer_timeout` to <100 (e.g. 50ms), `50/100 = 0` → VTIME=0 means "return immediately" — NOT the intended 50ms timeout. Also, VTIME is `cc_t` (unsigned char, max 255), so timeouts >25500ms silently overflow.

**Issue Analysis:**
```cpp
// Line 70 — Integer division truncation
constexpr int sniffer_timeout = 1000;  // defined in header as 1000
tty.c_cc[VTIME] = sniffer_timeout / 100;  // 1000/100 = 10 ✓
// But if changed to 50: 50/100 = 0 → NO TIMEOUT (immediate return)
// If changed to 25600: 25600/100 = 256 → wraps to 0 (cc_t max is 255)
```

**Change Suggested:**
- Add `static_assert(sniffer_timeout >= 100, "VTIME requires timeout >= 100ms")`
- Add `static_assert(sniffer_timeout / 100 <= 255, "VTIME exceeds cc_t max (25.5s)")`
- Use explicit `static_cast<cc_t>(sniffer_timeout / 100)`
- Document the VTIME constraints in the constant's Doxygen

**Acceptance Criteria:**
- [ ] `static_assert` prevents compilation with `sniffer_timeout < 100`
- [ ] `static_assert` prevents compilation with `sniffer_timeout / 100 > 255`
- [ ] VTIME assignment uses explicit `static_cast<cc_t>`
- [ ] Doxygen on `sniffer_timeout` documents the 100ms-25.5s range constraint

**Execution Plan:**
1. Add two `static_assert` statements near `sniffer_timeout` definition
2. Add `static_cast<cc_t>` to VTIME assignment
3. Update `sniffer_timeout` Doxygen with range constraint
4. Build and verify

---

### T-0004: Remove termios.h from public header (POSIX testability)

**Priority:** High (not for Windows portability — POSIX-only is fine — but for clean API + mock testing)
**Files:** include/ble_sniffer/bluetooth_at_driver.h:38,45; src/bluetooth_at_driver.cpp
**Specialist:** code-architect

**Issue Description:**
The public library header `bluetooth_at_driver.h` includes `<termios.h>` and exposes `speed_t sniffer_baud_rate = B115200`. While the project is POSIX-only (Linux + macOS), this has two problems:
1. `speed_t` and `B115200` are POSIX macros with platform-varying definitions — they shouldn't be in a clean public API
2. Including `termios.h` in the public header makes it impossible to create a mock `SerialPort` for testing without pulling in the entire POSIX serial stack

**Issue Analysis:**
```cpp
// bluetooth_at_driver.h line 38 — POSIX header in PUBLIC API
#include <termios.h>

// Line 45-46 — POSIX types in public constants
constexpr speed_t sniffer_baud_rate = B115200;  // speed_t varies by platform
constexpr int sniffer_timeout = 1000;
```

**Change Suggested:**
- Remove `#include <termios.h>` from the public header
- Move `sniffer_baud_rate` to `.cpp`; replace public constant with `constexpr int sniffer_baud_rate_bps = 115200` (the numeric baud rate)
- Keep `sniffer_timeout` in the header but as `constexpr int` (it already is)
- In `init()`, use `B115200` directly (now inside the `.cpp` where `<termios.h>` is included)

**Acceptance Criteria:**
- [ ] `#include <termios.h>` is NOT in `bluetooth_at_driver.h`
- [ ] `speed_t` and `B115200` do NOT appear in any public header
- [ ] Public API exposes `constexpr int sniffer_baud_rate_bps = 115200`
- [ ] `<termios.h>` is only in `bluetooth_at_driver.cpp`
- [ ] `B115200` is only used in `bluetooth_at_driver.cpp`
- [ ] Build passes on Linux and macOS

**Execution Plan:**
1. Remove `#include <termios.h>` from `bluetooth_at_driver.h`
2. Add `#include <termios.h>` to `bluetooth_at_driver.cpp` (if not already present)
3. Replace `constexpr speed_t sniffer_baud_rate = B115200` with `constexpr int sniffer_baud_rate_bps = 115200` in header
4. In `init()`, replace `cfsetospeed(&tty, sniffer_baud_rate)` with `cfsetospeed(&tty, B115200)` (direct constant in .cpp)
5. Update all references to `sniffer_baud_rate` → `sniffer_baud_rate_bps` or direct `B115200`
6. Build and verify

---

### T-0005: Fix Samsung device_type byte order + AirPods battery labeling

**Priority:** High
**Files:** src/proprietary_parsers.cpp:189,83-86
**Specialist:** code-architect

**Issue Description:**
1. Samsung SmartThings parser reads `device_type` as big-endian `(mfr_data[1] << 8) | mfr_data[2]` — BLE uses little-endian for multi-byte fields, so this is likely wrong
2. Apple AirPods battery nibbles are printed as raw integers without labeling — users will interpret raw level codes (0-10) as percentages

**Issue Analysis:**
```cpp
// Line 189 — Samsung: likely wrong byte order
uint16_t device_type = (static_cast<uint16_t>(mfr_data[1]) << 8) | mfr_data[2];
// Should be little-endian: mfr_data[1] | (mfr_data[2] << 8)

// Lines 83-86 — AirPods: raw nibbles look like percentages
uint8_t battery_byte = mfr_data[6];
uint8_t left = (battery_byte >> 4) & 0x0F;
uint8_t right = battery_byte & 0x0F;
detail << " battery L=" << static_cast<int>(left) << " R=" << static_cast<int>(right);
// Output: "battery L=5 R=8" — user reads as "5% and 8%" but these are coded levels
```

**Change Suggested:**
1. Change Samsung `device_type` to little-endian: `mfr_data[1] | (mfr_data[2] << 8)`
2. Label AirPods battery output as "(raw level)" or add a mapping table:
   - 0 = unknown, 1-4 = ≤50%, 5-7 = 50-75%, 8-A = 75-100%
3. Add `@note` in Doxygen that Samsung and AirPods formats are reverse-engineered

**Acceptance Criteria:**
- [ ] Samsung `device_type` uses little-endian byte order
- [ ] AirPods battery output is labeled as "(raw level)" or includes level-to-range mapping
- [ ] Doxygen `@note` on each vendor parser indicating reverse-engineered status
- [ ] Build passes

**Execution Plan:**
1. Fix Samsung byte order at line 189
2. Update AirPods battery detail string at lines 85-86
3. Add `@note` comments in proprietary_parsers.h for apple and samsung namespaces
4. Build and verify

---

### T-0006: Add AdvertisementType @note + BLE PDU type documentation

**Priority:** High
**Files:** include/ble_sniffer/types.h:124-130
**Specialist:** code-architect

**Issue Description:**
The `AdvertisementType` enum values match the ABSniffer 528's proprietary Table 2 encoding, NOT the Bluetooth Core Spec PDU type codes. For example, ABSniffer code 2 = "Scannable undirected" but BLE PDU type 0x02 = ADV_NONCONN_IND. This WILL confuse developers familiar with BLE. No documentation warning exists.

**Issue Analysis:**
```cpp
enum class AdvertisementType : int {
    CONNECTABLE_UNDIRECTED     = 0, // ABSniffer: same as BLE PDU 0x00
    CONNECTABLE_DIRECTED       = 1, // ABSniffer: same as BLE PDU 0x01
    SCANNABLE_UNDIRECTED       = 2, // ABSniffer:2 ≠ BLE PDU 0x02 (which is ADV_NONCONN_IND)
    NON_CONNECTABLE_UNDIRECTED = 3, // ABSniffer:3 ≠ BLE PDU 0x03 (which is SCAN_REQ)
    SCAN_RESPONSE              = 4, // ABSniffer:4 ≠ BLE PDU 0x04 (which is SCAN_RSP)
};
```

**Change Suggested:**
Add a prominent `@note` in the Doxygen:
```
@note These values follow the ABSniffer 528's proprietary Table 2 encoding,
NOT the Bluetooth Core Spec PDU type codes. For example, value 2 here means
"Scannable undirected advertisement" but BLE PDU type 0x02 = ADV_NONCONN_IND.
```

**Acceptance Criteria:**
- [ ] `AdvertisementType` Doxygen contains `@note` warning about ABSniffer vs BLE PDU type mismatch
- [ ] Build passes

**Execution Plan:**
1. Add `@note` to `AdvertisementType` Doxygen block in types.h
2. Build and verify

---

### T-0007: Add full Doxygen to proprietary_parsers.h

**Priority:** High
**Files:** include/ble_sniffer/proprietary_parsers.h:1-58
**Specialist:** code-architect

**Issue Description:**
`proprietary_parsers.h` has near-zero Doxygen documentation. No `@file` block, no `@brief` on `ParseResult`, no `@param`/`@return`/`@example` on any of the 8 vendor `parse()` functions, and `decode_proprietary`/`decode_proprietary_parts` use only `///` one-liners. This violates AGENTS.md: "Every module, struct, and public function must have an `@example` block."

**Issue Analysis:**
```cpp
// Current state — almost no documentation:
struct ParseResult {
    std::string description;  // no @brief
    std::string details;      // no @brief
};

namespace apple {
std::optional<ParseResult> parse(const std::vector<uint8_t>& mfr_data);  // no docs at all
};
// ... same for samsung, microsoft, sony, sonos, garmin, razer, furbo

/// @overload Structured decode from raw manufacturer-specific AD data
std::optional<ParseResult> decode_proprietary_parts(const std::vector<uint8_t>& mfr_ad_data);
// Missing: @brief, @param, @return, @example
```

**Change Suggested:**
Add full Doxygen per doxygen-cpp standard:
- `@file` / `@brief` for the module
- `@brief` for `ParseResult` and its fields
- `@brief`, `@param`, `@return`, `@example` for all `parse()` functions
- Full blocks for `decode_proprietary` and `decode_proprietary_parts`
- `@note` indicating vendor parsers are reverse-engineered, not officially documented

**Acceptance Criteria:**
- [ ] File has `@file` / `@brief` block with description and `@see` references
- [ ] `ParseResult` has `@brief` and field docs
- [ ] All 8 vendor `parse()` functions have `@brief`, `@param`, `@return`
- [ ] `decode_proprietary` and `decode_proprietary_parts` have full blocks with `@example`
- [ ] Each vendor namespace has `@note` about reverse-engineered status
- [ ] Build passes

**Execution Plan:**
1. Add `@file` block at top of file
2. Add `ParseResult` struct documentation
3. Add vendor namespace documentation and `@note`
4. Add `parse()` function documentation for each vendor
5. Add `decode_proprietary` / `decode_proprietary_parts` full blocks
6. Build and verify

---

### T-0008: Fix Doxygen gaps in stat_view.h, messages.h, bluetooth_at_driver.h

**Priority:** High
**Files:** src/stat_view.h, include/ble_sniffer/messages.h, include/ble_sniffer/bluetooth_at_driver.h
**Specialist:** code-architect

**Issue Description:**
Multiple documentation gaps:
- `stat_view.h`: No `@file` block, structs use `///` one-liners, missing `@param`/`@return`/`@example`
- `messages.h`: Missing `@return` tags on 5+ `from()` factory methods and getters, `no_data()`/`error()` use `///` one-liners
- `bluetooth_at_driver.h`: Destructor has no Doxygen, `start_scan()`/`stop_scan()`/`reset_device()` missing `@example`, `sniffer_baud_rate`/`sniffer_timeout` use `///` one-liners

**Issue Analysis:**
```cpp
// messages.h — missing @return on factory methods
static AddressMessage from(const RawMessage& raw);  // no @return
static VersionMessage from(const RawMessage& raw);  // no @return

// messages.h — getters use /// without @return
const std::string& prefix() const { return m_prefix; }  /// The response prefix

// bluetooth_at_driver.h — undocumented destructor
~BluetoothATDriver();  // no @brief at all

// stat_view.h — /// one-liners everywhere
class DeviceTracker {  /// LRU tracker bounded by a fixed capacity.
```

**Change Suggested:**
- Add `@file` block to `stat_view.h`
- Upgrade all `///` one-liners to full `/** @brief ... @param ... @return ... @example */` blocks
- Add `@return` tags to all `from()` factories and getters in `messages.h`
- Add destructor Doxygen to `bluetooth_at_driver.h`
- Add `@example` to `start_scan()`, `stop_scan()`, `reset_device()`

**Acceptance Criteria:**
- [ ] `stat_view.h` has `@file` block
- [ ] All public methods in `stat_view.h` have full Doxygen blocks
- [ ] All `from()` factories in `messages.h` have `@return` tags
- [ ] All getters in `messages.h` have `@return` tags
- [ ] `~BluetoothATDriver()` has Doxygen documenting RAII behavior
- [ ] `start_scan()`, `stop_scan()`, `reset_device()` have `@example` blocks
- [ ] Build passes

**Execution Plan:**
1. Fix stat_view.h documentation
2. Fix messages.h @return tags
3. Fix bluetooth_at_driver.h gaps
4. Build and verify

---

### T-0009: Fix README project structure + conanfile.py metadata + exports_sources

**Priority:** High
**Files:** README.md:228-241, conanfile.py:11-15,21
**Specialist:** code-architect

**Issue Description:**
1. README "Project Structure" section is severely outdated — lists `src/bluetooth-at-driver.h` (doesn't exist), missing 8+ actual files (ad_parser, proprietary_parsers, assigned_numbers, stat_view)
2. `conanfile.py` has placeholder values for license, author, url, description, topics
3. `conanfile.py` `exports_sources` is missing `include/*` — library headers won't be packaged

**Issue Analysis:**
```python
# conanfile.py — placeholders
license = "<Put the package license here>"       # ← placeholder
author = "<Put your name here> <And your email here>"  # ← placeholder
url = "<Package recipe repository url here>"     # ← placeholder
description = "<Description of bluetooth-at-driver package here>"  # ← placeholder
topics = ("<Put some tag here>", "<here>", "<and here>")  # ← placeholder

# conanfile.py — missing include/ in exports
exports_sources = "CMakeLists.txt", "src/*"  # ← missing "include/*"
```

**Change Suggested:**
1. Update README project structure to match actual file tree
2. Fill in conanfile.py metadata: license = "MIT", author = "Jean Boutros", url = "https://github.com/jeanboutros/april-brother-at-absniffer", description = "C++17 library and CLI tool for communicating with an ABSniffer 528 BLE sniffer over serial using AT commands", topics = ("bluetooth", "ble", "sniffer", "at-commands", "serial")
3. Add `"include/*"` to `exports_sources`

**Acceptance Criteria:**
- [ ] README project structure matches actual directory tree
- [ ] conanfile.py has no placeholder values (license = "MIT", author = "Jean Boutros", url = "https://github.com/jeanboutros/april-brother-at-absniffer", description = "C++17 library and CLI tool for communicating with an ABSniffer 528 BLE sniffer over serial using AT commands", topics = ("bluetooth", "ble", "sniffer", "at-commands", "serial"))
- [ ] `exports_sources` includes `"include/*"`
- [ ] Build passes

**Execution Plan:**
1. Update README project structure section
2. Fill in conanfile.py metadata (license = "MIT", author = "Jean Boutros", url = "https://github.com/jeanboutros/april-brother-at-absniffer", description = "C++17 library and CLI tool for communicating with an ABSniffer 528 BLE sniffer over serial using AT commands", topics = ("bluetooth", "ble", "sniffer", "at-commands", "serial"))
3. Add `"include/*"` to `exports_sources`
4. Build and verify

---

### T-0010: Add JSON escaping + terminal sanitization + signal handler fix

**Priority:** High
**Files:** src/main.cpp:29,41-47,158-159; src/stat_view.cpp; src/proprietary_parsers.cpp:20-29
**Specialist:** code-architect

**Issue Description:**
1. JSONL output constructs JSON via string concatenation without escaping — MAC field not JSON-escaped, injection risk for downstream consumers
2. BLE Local Name from device printed directly to terminal — ANSI escape codes could be injected (e.g. `\033[2J` clears screen)
3. `bytes_to_ascii()` in proprietary_parsers.cpp doesn't filter ESC (0x1B) — terminal injection path
4. Signal handler uses `std::atomic<bool>` with `memory_order_relaxed` — technically not async-signal-safe per C++17

**Issue Analysis:**
```cpp
// main.cpp:41-47 — JSONL without escaping
m_out << "{\"ts\":\"" << iso8601_now() << "\""
      << ",\"mac\":\"" << ble_sniffer::address_to_mac_address(msg.mac_address()) << "\""
      // ← if MAC contains " or \, JSON is broken

// main.cpp:158-159 — raw device data printed to terminal
if (local_name) std::cout << "[Local Name: " << *local_name << "] ";
// ← could contain \033[2J (ANSI clear screen)

// proprietary_parsers.cpp:20-29 — bytes_to_ascii doesn't filter ESC
std::string bytes_to_ascii(const std::vector<uint8_t>& data, size_t offset = 0) {
    // ...
    if (c >= 0x20 && c <= 0x7E) result += c;
    else result += '.';
    // ← ESC (0x1B) is < 0x20, so it's replaced by '.', which is fine
    // But wait — 0x1B IS < 0x20, so it IS filtered. However, some ANSI
    // sequences use bytes in 0x40-0x7E after ESC, which would pass through.
}

// main.cpp:27-29 — signal handler safety
std::atomic<bool> g_running{true};
void on_sigint(int) { g_running.store(false, std::memory_order_relaxed); }
// ← std::atomic operations are NOT guaranteed async-signal-safe per C++17
```

**Change Suggested:**
1. Add `json_escape()` utility function — escapes `"`, `\`, control chars per RFC 8259
2. Add `sanitise_for_terminal()` function — strips ESC (0x1B) and control sequences
3. Apply `sanitise_for_terminal()` to Local Name output and all device-sourced strings in TUI
4. Change signal handler to `volatile std::sig_atomic_t`

**Acceptance Criteria:**
- [ ] `json_escape()` function exists and escapes `"`, `\`, `\n`, `\r`, `\t`, and control chars
- [ ] All JSONL string fields use `json_escape()`
- [ ] `sanitise_for_terminal()` function strips ESC (0x1B) and control chars
- [ ] Local Name output and TUI device-sourced strings are sanitized
- [ ] Signal handler uses `volatile std::sig_atomic_t`
- [ ] Build passes

**Execution Plan:**
1. Create `json_escape()` utility in main.cpp (private)
2. Create `sanitise_for_terminal()` utility
3. Apply to JSONL writer and stdout output
4. Fix signal handler type
5. Build and verify

---

### T-0011: Fix code quality — naming, includes, member convention

**Priority:** Medium
**Files:** include/ble_sniffer/bluetooth_at_driver.h:179-180; src/proprietary_parsers.cpp:1; include/ble_sniffer/bluetooth_at_driver.h:126
**Specialist:** code-architect

**Issue Description:**
1. Private members `file_descriptor` and `read_buffer` don't follow `m_` prefix convention per AGENTS.md
2. `proprietary_parsers.cpp` uses `#include "ble_sniffer/..."` instead of `<ble_sniffer/...>` — inconsistent with all other source files
3. `send_command()` `@example` in bluetooth_at_driver.h uses raw strings ("AT+BAUD", "4") instead of library constants

**Issue Analysis:**
```cpp
// bluetooth_at_driver.h:179-180 — missing m_ prefix
int file_descriptor;     // should be m_file_descriptor
std::string read_buffer; // should be m_read_buffer

// proprietary_parsers.cpp:1 — wrong include style
#include "ble_sniffer/proprietary_parsers.h"  // should be <ble_sniffer/...>

// bluetooth_at_driver.h:126 — example uses raw strings
* driver.send_command("AT+BAUD", "4"); // Sends "AT+BAUD4\r\n"
// Should use: driver.send_command(ble_sniffer::AT_BAUD.data(), ...)
```

**Change Suggested:**
1. Rename `file_descriptor` → `m_file_descriptor`, `read_buffer` → `m_read_buffer` (update all references)
2. Change include to `<ble_sniffer/proprietary_parsers.h>`
3. Fix `send_command()` example to use library vocabulary

**Acceptance Criteria:**
- [ ] Private members use `m_` prefix in `BluetoothATDriver`
- [ ] `proprietary_parsers.cpp` uses angle-bracket include
- [ ] `send_command()` `@example` uses library constants
- [ ] Build passes

**Execution Plan:**
1. Rename private members and update all references in .cpp
2. Fix include path
3. Fix Doxygen example
4. Build and verify

---

### T-0012: Add BRKINT/PARMRK to termios mask + use TCSAFLUSH

**Priority:** Low
**Files:** src/bluetooth_at_driver.cpp:62,72
**Specialist:** code-architect

**Issue Description:**
1. `tty.c_iflag` mask doesn't clear `BRKINT` (generates SIGINT on break) and `PARMRK` (marks parity errors with 3-byte sequences) — for raw binary data these should be cleared
2. `tcsetattr` uses `TCSANOW` (immediate) instead of `TCSAFLUSH` (drain pending output first) — defense-in-depth

**Issue Analysis:**
```cpp
// Line 62 — missing BRKINT and PARMRK
tty.c_iflag &= ~(IGNBRK | IXON | IXOFF | IXANY | ICRNL | INLCR | IGNCR);
// Should add: BRKINT | PARMRK

// Line 72 — TCSANOW vs TCSAFLUSH
if (tcsetattr(file_descriptor, TCSANOW, &tty) != 0) {
// TCSAFLUSH drains pending output before applying, more robust
```

**Change Suggested:**
1. Add `BRKINT` and `PARMK` to the input flag mask
2. Consider changing to `TCSAFLUSH` (though `tcflush` on line 80 already handles this)

**Acceptance Criteria:**
- [ ] `BRKINT` and `PARMRK` are cleared in `c_iflag` mask
- [ ] Build passes

**Execution Plan:**
1. Add `BRKINT | PARMRK` to input flag mask
2. Optionally change `TCSANOW` to `TCSAFLUSH`
3. Build and verify

---

### T-0013: Add test framework + initial unit tests

**Priority:** Critical
**Files:** conanfile.py, CMakeLists.txt, new test/ directory
**Specialist:** code-architect + test-engineer
**Framework:** Catch2 v3

**Issue Description:**
The project has ZERO test infrastructure — no test directory, no test framework dependency, no CMake test target, no unit tests. All message parsing, AD structure parsing, and proprietary decoders are untestable without hardware. Per AGENTS.md testing section, the project only tests via manual CLI invocation.

**Change Suggested:**
- Add Catch2 v3 to conanfile.py requirements
- Add test target to CMakeLists.txt with `enable_testing()` + `ctest`
- Create `test/` directory with initial test files:
  - `test/test_types.cpp` — message_type_from_prefix, advertisement_type_to_string, address_to_mac_address
  - `test/test_messages.cpp` — RawMessage::parse, ScanResultMessage::from (valid + malformed), sentinels
  - `test/test_ad_parser.cpp` — parse_ad_structures, decode_manufacturer, decode_local_name, decode_ad_data
  - `test/test_proprietary_parsers.cpp` — Apple iBeacon/AirPods/Find My, Samsung, Microsoft, unknown company
  - `test/test_assigned_numbers.cpp` — company_name_from_id (Apple, Samsung, unknown), ad_type_name
- Add libFuzzer fuzz targets for `RawMessage::parse()`, `parse_ad_structures()`, and `decode_proprietary()` (Clang-only, compiled conditionally)
- Add manual corpus tests in Catch2 for known-bad inputs (malformed scan lines, truncated AD data, invalid hex)

**Acceptance Criteria:**
- [ ] Test framework installed via Conan
- [ ] `CMakeLists.txt` has test target + `enable_testing()` + `add_test()`
- [ ] `ctest` runs and all tests pass
- [ ] Tests cover: types, messages (valid + malformed), ad_parser, proprietary parsers, assigned_numbers
- [ ] At least 1 test verifies that malformed ScanResultMessage data does NOT crash
- [ ] libFuzzer fuzz target exists for `RawMessage::parse()` (compiles with `-fsanitize=fuzzer`)
- [ ] Manual corpus tests exist for malformed input edge cases
- [ ] Build passes

**Execution Plan:**
1. Add test framework to conanfile.py
2. Create test/CMakeLists.txt or add test target to main CMakeLists.txt
3. Create test/test_types.cpp with basic enum/utility tests
4. Create test/test_messages.cpp with valid + malformed input tests
5. Create test/test_ad_parser.cpp with AD structure parsing tests
6. Create test/test_proprietary_parsers.cpp with vendor parser tests
7. Create test/test_assigned_numbers.cpp with lookup tests
8. Build tests, run ctest, verify all pass
9. Create `test/fuzz_raw_message.cpp` with LLVMFuzzerTestOneInput entry point
10. Create `test/fuzz_ad_parser.cpp` with LLVMFuzzerTestOneInput entry point
11. Add CMake conditional for fuzz targets (only when CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
12. Add manual corpus test cases in test/test_messages.cpp for malformed inputs

---

### T-0014: Create SerialHal abstraction for mock-based testing

**Priority:** High
**Files:** New include/ble_sniffer/serial_hal.h, src/posix_serial_port.cpp, test/mock_serial_port.h
**Specialist:** code-architect
**Pattern:** Virtual interface (SerialPort base class)

**Issue Description:**
`BluetoothATDriver` directly opens POSIX serial ports — no way to test AT command construction, response parsing, or error handling without physical hardware. A `SerialPort` abstraction enables mock-based testing of the entire driver.

**Change Suggested:**
- Create `SerialPort` abstract interface in `include/ble_sniffer/serial_hal.h`
- Create `PosixSerialPort` in `src/posix_serial_port.cpp` (wraps current termios code)
- Refactor `BluetoothATDriver` to accept `SerialPort&` (dependency injection)
- Create `MockSerialPort` in `test/mock_serial_port.h` for unit testing

**Acceptance Criteria:**
- [ ] `SerialPort` interface exists with `open()`, `close()`, `read()`, `write()` methods
- [ ] `PosixSerialPort` implements all `SerialPort` methods using termios
- [ ] `BluetoothATDriver` accepts `SerialPort&` via constructor injection
- [ ] Existing CLI behavior is unchanged when using `PosixSerialPort`
- [ ] `MockSerialPort` exists for unit testing
- [ ] Build passes on Linux and macOS

**Execution Plan:**
1. Define `SerialPort` interface
2. Extract termios code into `PosixSerialPort`
3. Refactor `BluetoothATDriver` to use `SerialPort&`
4. Update CLI main.cpp to create `PosixSerialPort`
5. Create `MockSerialPort` for tests
6. Build and verify CLI still works
7. Add driver unit tests using MockSerialPort

---

### T-0015: Create docs/adr/ + docs/learning/ + Doxyfile

**Priority:** Medium
**Files:** New docs/adr/, docs/learning/, Doxyfile
**Specialist:** docs-writer

**Issue Description:**
No ADR directory exists — no design decisions are documented. No learning docs directory — non-trivial protocol knowledge lives only in code comments. No Doxyfile — no API documentation generation pipeline.

**Change Suggested:**
1. Create `docs/adr/` with seed ADRs:
   - ADR-0001: Immutable message struct pattern with static factory methods
   - ADR-0002: AT command constants as `inline constexpr string_view`
   - ADR-0003: Vendor-specific parser namespace architecture
   - ADR-0004: POSIX-only serial driver (termios)
   - ADR-0005: Auto-generated assigned_numbers via Python script
2. Create `docs/learning/` with INDEX.md and initial entries:
   - BLE AD structure TLV format
   - Apple Continuity protocol (reverse-engineered)
   - Serial port configuration on Linux/macOS (termios)
3. Create `Doxyfile` targeting `include/ble_sniffer/` and `src/`

**Acceptance Criteria:**
- [ ] `docs/adr/` directory exists with 5 seed ADR files
- [ ] `docs/learning/INDEX.md` exists with 3 entries
- [ ] `Doxyfile` exists and generates HTML when `doxygen` is run
- [ ] Generated HTML includes all public API symbols

**Execution Plan:**
1. Create `docs/adr/` directory
2. Write 5 seed ADRs
3. Create `docs/learning/` directory
4. Write INDEX.md + 3 learning doc entries
5. Create Doxyfile
6. Run doxygen and verify HTML output