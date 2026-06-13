# B1: PLAN

| Field | Value |
|-------|-------|
| Agent | code-architect |
| Timestamp | 2026-06-13T12:00Z |
| Step | B1 |

### Acceptance Criteria
1. `write()` return value is checked in `send_command()`; error is logged
2. `init()` returns `bool` indicating success/failure
3. `is_open()` public accessor exists and returns `file_descriptor >= 0`
4. All public methods check `is_open()` and return error sentinels when closed
5. `read_buffer` has a maximum size cap (64KB); overflow returns `RawMessage::error()`
6. Destructor only calls `stop_scan()`/`reset_device()` if fd is valid
7. Build passes with zero warnings

### Logical Units
| # | Unit | Files | AC Satisfied |
|---|------|-------|-------------|
| 1 | `init()`→bool, `is_open()`, rename private members, `MAX_READ_BUFFER`, `is_open()` guards | bluetooth_at_driver.h, bluetooth_at_driver.cpp | AC 2, 3, 4 |
| 2 | `write()` return check, `read_buffer` cap, destructor guard | bluetooth_at_driver.cpp | AC 1, 5, 6 |

### Approach
Unit 1 restructures the header and .cpp: rename private members per convention, change init() signature, add is_open() accessor, add MAX_READ_BUFFER constant, add is_open() guards to all public methods. Unit 2 adds the remaining safety checks.

---

# B2-1: APPLY Unit 1

| Field | Value |
|-------|-------|
| Agent | code-architect |
| Timestamp | 2026-06-13T12:05Z |
| Step | B2-1 |

| Unit | 1 |
| Build result | PASS — exit 0, 0 warnings |
| Files changed | bluetooth_at_driver.h (189→216 lines), bluetooth_at_driver.cpp (158→170 lines) |

Changes:
- `bluetooth_at_driver.h`: Changed `init()` return type `void` → `bool`, added `is_open()` inline accessor with Doxygen docs and @example, renamed `file_descriptor` → `m_file_descriptor`, `read_buffer` → `m_read_buffer`, added `MAX_READ_BUFFER = 65536` constexpr constant
- `bluetooth_at_driver.cpp`: Renamed all `file_descriptor` → `m_file_descriptor` and `read_buffer` → `m_read_buffer`, changed `init()` to return `bool` (return false on each error path, return true at end), added `is_open()` guards to all public methods, `device_info()` returns "Port not open" string when closed

---

# B2-2: APPLY Unit 2

| Field | Value |
|-------|-------|
| Agent | code-architect |
| Timestamp | 2026-06-13T12:10Z |
| Step | B2-2 |

| Unit | 2 |
| Build result | PASS — exit 0, 0 warnings |
| Files changed | bluetooth_at_driver.cpp (170→178 lines) |

Changes:
- `send_command()`: Captured `ssize_t written` from `write()`, added check `written < 0 || static_cast<size_t>(written) != full_command.size()`, logs error via `std::cerr << strerror(errno)`
- `read_line()`: After `m_read_buffer.append(buf, n)`, added check `if (m_read_buffer.size() > MAX_READ_BUFFER)` → clears buffer and returns `RawMessage::error()`
- Destructor: Already guarded from Unit 1 with `if (m_file_descriptor >= 0)` — no additional change needed

---

# B3: VALIDATE

| Field | Value |
|-------|-------|
| Agent | code-architect |
| Timestamp | 2026-06-13T12:15Z |
| Step | B3 |

| Full build | PASS — exit 0, 0 warnings (clean rebuild) |
| AC coverage | 7/7 acceptance criteria satisfied |

### AC Evidence

| AC | Status | Evidence |
|----|--------|----------|
| AC1: write() return checked | ✅ PASS | bluetooth_at_driver.cpp:91-93 — `ssize_t written = write(...)` then `if (written < 0 \|\| static_cast<size_t>(written) != full_command.size())` with `std::cerr` log |
| AC2: init() returns bool | ✅ PASS | bluetooth_at_driver.h:116 `bool init();`, bluetooth_at_driver.cpp:31 `bool BluetoothATDriver::init()`, returns `false` on error, `true` on success |
| AC3: is_open() accessor | ✅ PASS | bluetooth_at_driver.h:130 `bool is_open() const { return m_file_descriptor >= 0; }` with Doxygen + @example |
| AC4: All public methods check is_open() | ✅ PASS | read_line (L98), send_command (L85), query_status (L127), query_address (L133), query_version (L139), device_info (L145), start_scan (L154), stop_scan (L159), set_baud_rate (L163), set_scan_mode (L169), reset_device (L174) — all guard with `if (!is_open())` |
| AC5: read_buffer 64KB cap | ✅ PASS | bluetooth_at_driver.h:213 `MAX_READ_BUFFER = 65536`, bluetooth_at_driver.cpp:104-107 overflow check clears buffer and returns `RawMessage::error()` |
| AC6: Destructor guards fd | ✅ PASS | bluetooth_at_driver.cpp:22 `if (m_file_descriptor >= 0)` wraps stop_scan/reset_device/close |
| AC7: Build zero warnings | ✅ PASS | Clean rebuild output shows 0 warnings, exit 0 |

### T1 Mechanical Checks (Self-Assessment)

| # | Check | Result |
|---|-------|--------|
| T1.1 | All new public symbols have doc comments | ✅ `is_open()` has Doxygen with @brief, @return, @example; `init()` return type change documented with @return and @example |
| T1.2 | No raw integers in public API | ✅ `MAX_READ_BUFFER` is named constant (65536), not magic number |
| T1.3 | Typed enums used | ✅ No new enums needed; existing BaudRate/ScanMode unchanged |
| T1.4 | No platform headers in public API | ⚠️ termios.h still in public header — out of scope (psc-0005) |
| T1.5 | Named constants, not magic numbers | ✅ `MAX_READ_BUFFER` named, no magic numbers introduced |
| T1.6 | Private members use m_ prefix | ✅ `m_file_descriptor`, `m_read_buffer`, `m_device` |
| T1.7 | Build passes with -Werror equivalent | ✅ Zero warnings with GCC 14.2.0 |
| T1.8 | Doc-standard compliance (Doxygen) | ✅ All new public methods have @brief, @return, @example |
| T1.9 | Reserved bits handled | N/A — no register/bitfield changes |