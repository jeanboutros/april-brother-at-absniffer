# Bluetooth AT Driver

A CLI tool for communicating with an ABSniffer 528 BLE sniffer device over serial using AT commands.

## Origin

I was in China and wanted to get a Bluetooth sniffer. I went on Taobao and found a model that seemed fine. With my clunky Chinese I missed a detail — it was a limited device that only captures advertisement packets and is controlled using AT commands.

The seller was kind enough to advise me on how to use it and to send me the wiki. So it worked, but I wanted to learn C++, so I thought: why not write a proper program for the sniffer rather than fiddling with the terminal every time I want to use it?

And that's how this project was born.

- **Device:** [ABSniffer 528 on Taobao](https://item.taobao.com/item.htm?id=586733843626)
- **Wiki:** [AT Commands for ABSniffer 528](https://wiki.aprbrother.com/en/AT_Commands_For_ABSniffer_528.html)

## Prerequisites

- CMake >= 3.15
- C++17 compiler
- [Conan 2](https://conan.io/) package manager

## Build Commands

### Debug (development)

```bash
conan install . --build=missing -s build_type=Debug
cmake --preset conan-debug
cmake --build --preset conan-debug
```

Binary output: `build/Debug/bluetooth-at-driver`

### Release

```bash
conan install . --build=missing -s build_type=Release
cmake --preset conan-release
cmake --build --preset conan-release
```

Binary output: `build/Release/bluetooth-at-driver`

### Clean

```bash
rm -rf build/Debug
rm -rf build/Release
```

### Rebuild (clean + build)

```bash
cmake --build --preset conan-debug --clean-first
```

## Usage

```
bluetooth-at-driver [OPTIONS] <device>

Arguments:
  device              Serial device path (e.g. /dev/cu.usbmodemXXX, /dev/ttyUSB0)  [required]

Options:
  -i, --info          Query device info (status, address, version)
  -s, --start-scan    Start scanning for BLE advertisements
  -p, --stop-scan     Stop scanning
  -v, --verbose       Verbosity: -v formatted MAC, -vv full AD decode
      --stat          Live TUI dashboard (sorted by RSSI)
      --stat-limit N  Max devices in dashboard (default: 30)
  -o, --output FILE   Capture all packets to a JSONL file
  -h, --help          Print this help message
```

### Find your device path

**macOS:**
```bash
ls /dev/cu.usbmodem*
```

**Linux:**
```bash
ls /dev/ttyUSB* /dev/ttyACM*
```

### Examples

```bash
# Query device info
./build/Debug/bluetooth-at-driver -i /dev/cu.usbmodemE5373320F96A1

# Scan with full AD decode
./build/Debug/bluetooth-at-driver -s -vv /dev/cu.usbmodemE5373320F96A1

# Live TUI dashboard (top 30 devices, sorted by signal strength)
./build/Debug/bluetooth-at-driver -s --stat /dev/cu.usbmodemE5373320F96A1

# TUI + simultaneous JSONL capture (pipe to jq in another terminal)
./build/Debug/bluetooth-at-driver -s --stat -o capture.jsonl /dev/cu.usbmodemE5373320F96A1

# Linux — same commands with ttyUSB device
./build/Debug/bluetooth-at-driver -s --stat /dev/ttyUSB0

# Query captured data with jq
cat capture.jsonl | jq -r '"\(.mac)  RSSI:\(.rssi)  \(.adv_data[:16])"'
cat capture.jsonl | jq 'select(.rssi > -60)'      # strong signals only
cat capture.jsonl | jq -s 'group_by(.mac) | map({mac: .[0].mac, count: length})'
```

## Contributor Guide

### First-Time Setup (macOS)

1. **Install Homebrew** (if not already installed):
   ```bash
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   ```

2. **Install dependencies:**
   ```bash
   brew install cmake
   pip install conan
   ```

3. **Detect your Conan profile** (one-time):
   ```bash
   conan profile detect
   ```

4. **Clone and build:**
   ```bash
   git clone <repo-url>
   cd bluetooth-at-driver
   conan install . --build=missing -s build_type=Debug
   cmake --preset conan-debug
   cmake --build --preset conan-debug
   ```

### First-Time Setup (Linux / Ubuntu/Debian)

1. **Install dependencies:**
   ```bash
   sudo apt update
   sudo apt install -y cmake g++ python3-pip
   pip install conan
   ```

2. **Detect your Conan profile** (one-time):
   ```bash
   conan profile detect
   ```

3. **Clone and build:**
   ```bash
   git clone <repo-url>
   cd bluetooth-at-driver
   conan install . --build=missing -s build_type=Debug
   cmake --preset conan-debug
   cmake --build --preset conan-debug
   ```

### VS Code Setup

Install the following extensions:
- [CodeLLDB](https://marketplace.visualstudio.com/items?itemName=vadimcn.vscode-lldb) — debugger
- [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools) — CMake integration
- [clangd](https://marketplace.visualstudio.com/items?itemName=llvm-vs-code-extensions.vscode-clangd) — C++ intellisense

Build tasks and launch configs are already provided in `.vscode/`.

### Adding a Library

1. **Find the package on [Conan Center](https://conan.io/center).**

2. **Add it to `conanfile.py`:**
   ```python
   def requirements(self):
       self.requires("cli11/2.4.2")
       self.requires("new-library/1.0.0")  # ← add here
   ```

3. **Add it to `CMakeLists.txt`:**
   ```cmake
   find_package(NewLibrary REQUIRED)
   target_link_libraries(bluetooth-at-driver PRIVATE NewLibrary::NewLibrary)
   ```
   > The exact target name is shown on the Conan Center page for each package.

4. **Re-run Conan install and rebuild:**
   ```bash
   conan install . --build=missing -s build_type=Debug
   cmake --preset conan-debug
   cmake --build --preset conan-debug
   ```

### Project Structure

```
├── CMakeLists.txt          # Build configuration
├── conanfile.py            # Dependency management
├── CMakeUserPresets.json   # Conan-generated presets
├── src/
│   ├── main.cpp            # CLI entry point
│   ├── bluetooth-at-driver.h   # Driver header (AT commands, enums, types)
│   └── bluetooth-at-driver.cpp # Driver implementation
├── .vscode/
│   ├── tasks.json          # Build/clean/run tasks
│   └── launch.json         # Debug/run configurations
└── test_package/
    └── conanfile.py        # Conan package test recipe
```

## TODO

- [ ] **JSONL to stdout** — add a flag (e.g. `--json`) to emit JSONL packets to stdout instead of a file, enabling `bluetooth-at-driver -s --json /dev/... | jq ...` pipes
- [ ] **Device disconnect handling** — detect serial port disconnect, retry/reconnect with configurable timeout (`--reconnect-timeout`), or wait indefinitely; exit cleanly when the timeout expires
- [ ] **JSONL replay input** — add `-i, --input FILE` to read a previously-captured JSONL file instead of a live device. Packets are decoded and displayed (or fed into `--stat`) exactly as if they were live. The positional `device` argument becomes optional when `-i` is supplied
- [ ] **Disable events flag** — add `--no-events` to suppress event generation (useful during replay with `-i` where timing-based events are meaningless)
- [ ] **Event stream** — add `--events FILE` (or `--events -` for stdout) to emit structured JSONL events in parallel with normal output:
  - `device_appeared` — first time a MAC is seen in this session
  - `device_returned` — a known MAC reappears after being absent for longer than a threshold (default: 1 hour, configurable with `--return-threshold`)
  - `device_lost` — a tracked MAC has not been seen for longer than a threshold (default: 5 minutes, configurable with `--lost-threshold`)
  - `rssi_changed` — significant RSSI jump (±10 dB) between consecutive advertisements from the same MAC
  - `new_payload` — a device starts broadcasting a different AD payload (e.g. name change, new service UUID)
  - `battery_low` — for decoded Apple devices (AirPods, Find My) when reported battery drops below a threshold
  - `ibeacon_region_enter` / `ibeacon_region_exit` — an iBeacon UUID+Major+Minor appears or disappears
  - `temperature_report` — a BLE sensor beacon (Ruuvi, Xiaomi, custom) broadcasts a temperature reading (parsed from manufacturer-specific AD payload)
  - `humidity_report` — same, for humidity (Ruuvi, Xiaomi, Govee, SwitchBot)
  - `pressure_report` — barometric pressure from Ruuvi or similar environmental sensor beacons
  - `exposure_notification` — detection of an Apple/Google Exposure Notification beacon (COVID-era contact-tracing protocol, service UUID `0xFD6F`)
  - `sensor_alert` — generic high/low threshold crossing on any telemetry value (e.g. temperature > 40°C, battery < 10%)


