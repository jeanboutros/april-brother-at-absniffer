# B3: VALIDATE — psc-0029

| Field | Value |
|-------|-------|
| Agent | software-engineer |
| Timestamp | 2026-06-14T14:00:00Z |
| Step | B3 |
| Verdict | PASS |

## Scope

Full clean build and verification of all psc-0029 Phase B changes across both units.

---

## Build Verification

| Field | Value |
|-------|-------|
| Command | `cmake --build --preset conan-debug --clean-first` |
| Exit code | 0 |
| Warnings | 0 |
| Errors | 0 |

All targets built successfully:
- `libble_sniffer.a` — rebuilt (all `.cpp.o` objects recompiled)
- `bluetooth-at-driver` — rebuilt (`main.cpp.o` recompiled, relinked)

**Evidence:**
```
[ 30%] Building CXX object CMakeFiles/ble_sniffer.dir/src/bluetooth_at_driver.cpp.o
[ 30%] Building CXX object CMakeFiles/ble_sniffer.dir/src/assigned_numbers.cpp.o
[ 30%] Building CXX object CMakeFiles/ble_sniffer.dir/src/messages.cpp.o
[ 50%] Building CXX object CMakeFiles/ble_sniffer.dir/src/proprietary_parsers.cpp.o
[ 60%] Building CXX object CMakeFiles/ble_sniffer.dir/src/ad_parser.cpp.o
[ 60%] Building CXX object CMakeFiles/ble_sniffer.dir/src/ab_sniffer_serial_port.cpp.o
[ 70%] Linking CXX static library libble_sniffer.a
[ 70%] Built target ble_sniffer
[ 90%] Building CXX object CMakeFiles/bluetooth-at-driver.dir/src/main.cpp.o
[ 90%] Building CXX object CMakeFiles/bluetooth-at-driver.dir/src/stat_view.cpp.o
[100%] Linking CXX executable bluetooth-at-driver
[100%] Built target bluetooth-at-driver
```

---

## Verification of Specific Changes

### 1. `bluetooth_at_driver.h` Line 50 — Doxygen Fix (F1)

**Expected:** "Takes ownership of an already-initialized serial port" (not "Opens the serial port on construction")

**Actual (line 50):**
```
 * Takes ownership of an already-initialized serial port on construction and closes it on destruction (RAII).
```

**Additional (line 51):**
```
 * The port must be open (is_open() returns true) before construction; otherwise std::invalid_argument is thrown.
```

**Verdict:** ✅ PASS — Both the ownership description and the `is_open()` precondition are documented.

### 2. `bluetooth_at_driver.cpp` — Constructor Precondition Check (F4)

**Expected:** Both null check AND `is_open()` check

**Actual (lines 14-20):**
```cpp
BluetoothATDriver::BluetoothATDriver(std::unique_ptr<serial::SerialPort> serial_port) : m_serial_port(std::move(serial_port)) {
    if (!m_serial_port) {
        throw std::invalid_argument("serial_port must not be null");
    }
    if (!m_serial_port->is_open()) {
        throw std::invalid_argument("serial_port must be open before constructing BluetoothATDriver (call init() first)");
    }
}
```

**Verdict:** ✅ PASS — Both null check (lines 15-17) and `is_open()` check (lines 18-20) present. Error message mentions `init()`.

### 3. `SerialPort.h` — `[[nodiscard]]` on `init()` (psc-adr-0029-1)

**Expected:** `[[nodiscard]]` attribute on `init()` declaration

**Actual (line 197):**
```cpp
    [[nodiscard]] virtual bool init() = 0;
```

**Verdict:** ✅ PASS — `[[nodiscard]]` attribute is on the interface declaration.

### 4. `main.cpp` — Action Flag Validation + Init Call (lines 102-119)

**Expected:** Action flag validation + `init()` call with defensive comment

**Actual (lines 102-123):**
```cpp
    // Require at least one action flag so the program doesn't exit silently.
    if (!info_flag && !scan_flag && !stop_scan_flag) {
        std::cerr << "Error: No action specified. Use -i (info), -s (scan), or --stop-scan." << std::endl;
        return 1;
    }

    // Initialize the serial port before passing it to the driver.
    // init() must be called explicitly — the driver does not call it.
    auto port = std::make_unique<serial::ABSnifferSerialPort>(device_path);
    try {
        // Defensive check: ABSnifferSerialPort::init() always throws on failure (never returns false),
        // but the SerialPort interface contract permits implementations that return false.
        // This check handles alternative SerialPort implementations correctly.
        if (!port->init()) {
            std::cerr << "Failed to initialize serial port: " << device_path << std::endl;
            return 1;
        }
    } catch (const serial::SerialPortException& e) {
        std::cerr << "Error opening serial port: " << e.what() << std::endl;
        return 1;
    }
    ble_sniffer::BluetoothATDriver driver(std::move(port));
```

**Verdict:** ✅ PASS — All elements present:
- Action flag validation at lines 103-106
- `init()` call with error handling at lines 110-122
- Defensive comment explaining the dead code nature at lines 112-114

---

## Diff Verification (Unintended Changes Check)

`git diff HEAD --stat` shows:

| File | Lines Changed | Intended? |
|------|---------------|-----------|
| `include/ble_sniffer/SerialPort.h` | +1/-1 | ✅ Yes — `[[nodiscard]]` |
| `include/ble_sniffer/bluetooth_at_driver.h` | +2/-1 | ✅ Yes — Doxygen fix |
| `src/bluetooth_at_driver.cpp` | +3 | ✅ Yes — `is_open()` check |
| `src/main.cpp` | +23/-1 | ✅ Yes — action flags + init + defensive comment |
| `.opencode/agents/code-architect.md` | +1 | Not reviewed (pipeline config) |
| `docs/project-management/counters.json` | +1/-1 | Not reviewed (pipeline config) |

No unintended changes to source files.

---

## Overall Verdict: PASS

Clean build with zero errors, zero warnings. All four required changes verified present and correct. No unintended changes to source files.
