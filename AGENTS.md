# AGENTS.md

## Project Overview

C++17 library + CLI tool for communicating with an ABSniffer 528 BLE sniffer over serial using AT commands. The library (`libble_sniffer`) is separate from the CLI executable.

## Build System

- **Package manager:** Conan 2 (`conanfile.py`)
- **Build tool:** CMake with Conan presets
- **C++ standard:** C++17

### Commands

```bash
# Install dependencies
conan install . --build=missing -s build_type=Debug

# Configure + build
cmake --preset conan-debug
cmake --build --preset conan-debug

# Release
conan install . --build=missing -s build_type=Release
cmake --preset conan-release
cmake --build --preset conan-release
```

**Important:** Never use bare `cmake ..` — always use presets or pass `-DCMAKE_BUILD_TYPE=<type>` with the Conan toolchain file.

### Adding a dependency

1. Add `self.requires("pkg/version")` to `conanfile.py` `requirements()` method
2. Add `find_package(Pkg REQUIRED)` and `target_link_libraries(ble_sniffer ...)` to `CMakeLists.txt`
3. Re-run `conan install` then rebuild

## Project Structure

```
include/ble_sniffer/           # Public API headers (installed with library)
├── types.h                    # Enums, constants, AT command strings, utilities
├── messages.h                 # Message structs (RawMessage, ScanResultMessage, etc.)
└── bluetooth_at_driver.h      # BluetoothATDriver class (serial I/O)

src/
├── messages.cpp               # Message parsing implementation
├── bluetooth_at_driver.cpp    # Serial driver implementation
└── main.cpp                   # CLI executable (links library + CLI11)
```

- `ble_sniffer` is a static library with public includes in `include/`
- `bluetooth-at-driver` is the CLI executable, links `ble_sniffer` + `CLI11`
- CLI11 is only a dependency of the executable, not the library

## Code Conventions

### Namespace
All library code lives in `namespace ble_sniffer`.

### Immutable message structs
- Fields are `protected` (accessible to derived types) with `const` getters
- Construction only through static factory methods (`parse()`, `from()`, `no_data()`, `error()`)
- Derived message types (e.g. `ScanResultMessage`) inherit from `RawMessage` and add typed fields

### Enums
- Use `enum class` with explicit underlying type (`int`)
- Parameter enums map directly to AT command integer values
- Provide a `*_to_string()` free function for human-readable conversion

### AT commands
- Stored as `inline constexpr std::string_view`
- Parameters are appended directly (no separator): `"AT+BAUD" + "4"` → `"AT+BAUD4"`
- All commands terminated with `\r\n` (handled by `send_command`)

### Documentation
- Use Doxygen-style comments (`/** */`, `@brief`, `@param`, `@return`)
- Every module, struct, and public function must have an `@example` block
- Comments describe **what**, not **how**
- Reference the AT command wiki: https://wiki.aprbrother.com/en/AT_Commands_For_ABSniffer_528.html

### Naming
- Files: `snake_case` (e.g. `bluetooth_at_driver.cpp`)
- Classes/structs: `PascalCase` (e.g. `BluetoothATDriver`, `ScanResultMessage`)
- Functions/methods: `snake_case` (e.g. `read_line`, `query_status`)
- Private members: `m_` prefix (e.g. `m_mac_address`)
- Constants: `UPPER_SNAKE_CASE` for delimiters, `PascalCase` for AT commands

### Error handling
- Serial errors return sentinel messages (`RawMessage::error()`, `RawMessage::no_data()`)
- Check `msg.type()` before accessing data
- Failed `init()` sets `file_descriptor = -1`

## Testing

Run from project root:
```bash
./build/Debug/bluetooth-at-driver --help
./build/Debug/bluetooth-at-driver -i          # device info
./build/Debug/bluetooth-at-driver -s          # scan (Ctrl+C to stop)
./build/Debug/bluetooth-at-driver -s -vv      # scan with full detail
./build/Debug/bluetooth-at-driver --stop-scan # stop scanning
```
