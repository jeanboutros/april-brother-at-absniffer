#include <ble_sniffer/bluetooth_at_driver.h>
#include <ble_sniffer/messages.h>
#include <ble_sniffer/SerialPort.h>
#include <ble_sniffer/ab_sniffer_serial_port.h>
#include <ble_sniffer/ad_parser.h>
#include <ble_sniffer/assigned_numbers.h>
#include <ble_sniffer/proprietary_parsers.h>

#include "stat_view.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <CLI/CLI.hpp>

namespace {

// SIGINT-driven shutdown so the TUI can restore the cursor and the file writer can flush.
std::atomic<bool> g_running{true};

void on_sigint(int) { g_running.store(false, std::memory_order_relaxed); }

/// JSONL capture writer. One JSON object per line; safe to pipe into jq.
class JsonlCaptureWriter {
public:
    explicit JsonlCaptureWriter(const std::string& path) : m_out(path, std::ios::out | std::ios::app) {
        if (!m_out) {
            throw std::runtime_error("failed to open output file: " + path);
        }
    }

    void write(const ble_sniffer::ScanResultMessage& msg) {
        m_out << "{\"ts\":\"" << iso8601_now() << "\""
              << ",\"mac\":\"" << ble_sniffer::address_to_mac_address(msg.mac_address()) << "\""
              << ",\"rssi\":" << msg.rssi()
              << ",\"adv_type\":" << static_cast<int>(msg.adv_type())
              << ",\"data_length\":" << msg.data_length()
              << ",\"adv_data\":\"" << to_hex(msg.adv_data()) << "\""
              << "}\n";
        m_out.flush();
    }

private:
    static std::string iso8601_now() {
        using namespace std::chrono;
        auto now = system_clock::now();
        auto secs = time_point_cast<seconds>(now);
        auto ms = duration_cast<milliseconds>(now - secs).count();
        std::time_t t = system_clock::to_time_t(secs);
        std::tm tm_utc{};
        gmtime_r(&t, &tm_utc);
        std::ostringstream os;
        os << std::put_time(&tm_utc, "%Y-%m-%dT%H:%M:%S")
           << '.' << std::setw(3) << std::setfill('0') << ms << 'Z';
        return os.str();
    }

    static std::string to_hex(const std::vector<uint8_t>& bytes) {
        std::ostringstream os;
        os << std::hex << std::setfill('0');
        for (auto b : bytes) os << std::setw(2) << static_cast<int>(b);
        return os.str();
    }

    std::ofstream m_out;
};

} // namespace

int main(int argc, char** argv) {
    bool info_flag = false;
    bool scan_flag = false;
    bool stop_scan_flag = false;
    bool stat_flag = false;
    int  stat_limit = 30;
    int  verbosity = 0;
    std::string output_path;
    std::string device_path;

    CLI::App app{"Bluetooth AT Driver CLI"};
    app.add_flag("-i,--info",  info_flag, "Query device info (status, address, version)");
    app.add_flag("-s,--start-scan", scan_flag, "Start scanning for BLE advertisements");
    app.add_flag("-p,--stop-scan", stop_scan_flag, "Stop scanning for BLE advertisements");
    app.add_flag("-v,--verbose", verbosity, "Verbosity level (-v: formatted MAC, -vv: full detail)");
    app.add_flag("--stat", stat_flag, "Show live device dashboard (TUI) instead of raw packet stream");
    app.add_option("--stat-limit", stat_limit, "Max devices to track in --stat mode")->default_val(30);
    app.add_option("-o,--output", output_path, "Write captured packets to a JSONL file (one packet per line)");
    app.add_option("device", device_path, "Serial device path (e.g. /dev/cu.usbmodemXXX or /dev/ttyUSB0)")
        ->required();
    CLI11_PARSE(app, argc, argv);

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

    if (info_flag) {
        std::cout << "Reading from Bluetooth AT Driver..." << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        std::cout << driver.device_info() << std::endl;
        std::cout << "----------------------------------------" << std::endl;
    }

    if (scan_flag) {
        std::unique_ptr<JsonlCaptureWriter> writer;
        if (!output_path.empty()) {
            try {
                writer = std::make_unique<JsonlCaptureWriter>(output_path);
            } catch (const std::exception& ex) {
                std::cerr << "Output: " << ex.what() << std::endl;
                return 1;
            }
        }

        std::signal(SIGINT, on_sigint);
        driver.start_scan();

        std::unique_ptr<ble_sniffer::stat::DeviceTracker> tracker;
        std::unique_ptr<ble_sniffer::stat::StatRenderer>  renderer;
        if (stat_flag) {
            tracker  = std::make_unique<ble_sniffer::stat::DeviceTracker>(
                static_cast<std::size_t>(std::max(1, stat_limit)));
            renderer = std::make_unique<ble_sniffer::stat::StatRenderer>();
        } else {
            std::cout << "Started scanning for BLE advertisements. Press Ctrl+C to stop." << std::endl;
        }

        while (g_running.load(std::memory_order_relaxed)) {
            auto msg = driver.read_line();
            if (msg.type() == ble_sniffer::MessageType::SCAN_RESULT) {
                auto scan_msg = ble_sniffer::ScanResultMessage::from(msg);

                if (writer) writer->write(scan_msg);

                if (stat_flag) {
                    tracker->update(scan_msg);
                    renderer->render(*tracker);
                    continue;
                }

                std::string mac = scan_msg.mac_address();
                const std::vector<uint8_t>& adv_data = scan_msg.adv_data();

                if (verbosity >= 2) {
                    std::cout << "Advertisement from " << ble_sniffer::address_to_mac_address(mac)
                              << " | RSSI: " << scan_msg.rssi()
                              << " | Type: " << advertisement_type_to_string(scan_msg.adv_type())
                              << " | Data Length: " << scan_msg.data_length()
                              << " | Data: \n";
                    auto ad_structures = ble_sniffer::parse_ad_structures(adv_data);
                    auto local_name = ble_sniffer::decode_local_name(ad_structures);
                    auto manufacturer_data = ble_sniffer::decode_manufacturer(ad_structures);
                    if (local_name) std::cout << "[Local Name: " << *local_name << "] ";
                    if (manufacturer_data) std::cout << "[Manufacturer: " << *manufacturer_data << "] ";
                    std::cout << std::endl;
                    for (const auto& ad : ad_structures) {
                        std::cout << "  [0x" << std::hex << std::setfill('0') << std::setw(2)
                                  << static_cast<int>(ad.type) << std::dec << "] "
                                  << ble_sniffer::ad_type_name(ad.type) << ": ";
                        for (uint8_t byte : ad.data) {
                            std::cout << std::hex << std::setfill('0') << std::setw(2)
                                      << static_cast<int>(byte) << " ";
                        }
                        std::cout << std::dec << std::endl;
                        if (ad.type == 0xFF) {
                            auto decoded = ble_sniffer::proprietary::decode_proprietary(ad.data);
                            if (decoded) std::cout << "    -> " << *decoded << std::endl;
                        } else {
                            auto decoded = ble_sniffer::decode_ad_data(ad);
                            if (decoded) std::cout << "    -> " << *decoded << std::endl;
                        }
                    }
                    std::cout << "----------------------------------------" << std::endl;
                } else if (verbosity == 1) {
                    std::cout << ble_sniffer::address_to_mac_address(mac) << ","
                              << scan_msg.rssi() << ","
                              << static_cast<int>(scan_msg.adv_type()) << ","
                              << scan_msg.data_length() << ","
                              << (scan_msg.adv_data().size() == static_cast<size_t>(scan_msg.data_length()) ? "SIZE_MATCH" : "SIZE_MISMATCH") << ",";
                    for (uint8_t byte : scan_msg.adv_data()) {
                        std::cout << std::hex << std::setfill('0') << std::setw(2)
                                  << static_cast<int>(byte);
                    }
                    std::cout << std::dec << std::endl;
                } else {
                    std::cout << mac << ","
                              << scan_msg.rssi() << ","
                              << static_cast<int>(scan_msg.adv_type()) << ","
                              << scan_msg.data_length() << ","
                              << (scan_msg.adv_data().size() == static_cast<size_t>(scan_msg.data_length()) ? "SIZE_MATCH" : "SIZE_MISMATCH") << ",";
                    for (uint8_t byte : scan_msg.adv_data()) {
                        std::cout << std::hex << std::setfill('0') << std::setw(2)
                                  << static_cast<int>(byte);
                    }
                    std::cout << std::dec << std::endl;
                }
            } else if (msg.type() != ble_sniffer::MessageType::NO_DATA) {
                if (!stat_flag) {
                    std::cout << "Received message: " << msg.prefix() << " | " << msg.data() << std::endl;
                }
            } else if (stat_flag) {
                // Periodically refresh the TUI even when no packets arrive (so ages keep advancing).
                renderer->render(*tracker);
            }
        }

        driver.stop_scan();
        renderer.reset(); // restores cursor before any final stdout
    }

    if (stop_scan_flag) {
        driver.stop_scan();
        std::cout << "Stopped scanning for BLE advertisements." << std::endl;
        return 0;
    }
    return 0;
}


