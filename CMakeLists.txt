cmake_minimum_required(VERSION 3.15)
project(bluetooth-at-driver CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(CLI11 REQUIRED)

# --- Library ---
add_library(ble_sniffer
    src/messages.cpp
    src/bluetooth_at_driver.cpp
    src/assigned_numbers.cpp
    src/ad_parser.cpp
    src/proprietary_parsers.cpp
    include/ble_sniffer/SerialPort.h
    src/ab_sniffer_serial_port.cpp
)
target_include_directories(ble_sniffer PUBLIC include)
target_compile_features(ble_sniffer PUBLIC cxx_std_17)

# --- CLI Executable ---
add_executable(bluetooth-at-driver
    src/main.cpp
    src/stat_view.cpp
)
target_link_libraries(bluetooth-at-driver PRIVATE ble_sniffer CLI11::CLI11)

# --- Install ---
install(TARGETS ble_sniffer bluetooth-at-driver
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
install(DIRECTORY include/ble_sniffer DESTINATION include)
