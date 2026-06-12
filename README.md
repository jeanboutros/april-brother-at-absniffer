# Bluetooth AT Driver

A CLI tool for communicating with an ABSniffer 528 BLE sniffer device over serial using AT commands.

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

```bash
./build/Debug/bluetooth-at-driver --help
./build/Debug/bluetooth-at-driver --info
./build/Debug/bluetooth-at-driver --scan
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
