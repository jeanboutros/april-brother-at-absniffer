#pragma once

/**
 * @file bluetooth_at_driver.h
 * @brief Serial driver for the ABSniffer 528 BLE sniffer device.
 *
 * Provides a high-level interface to open the serial port, send AT commands,
 * and receive parsed responses. The driver manages the serial connection
 * lifecycle and provides convenience methods for common operations.
 *
 * @example
 * @code
 * #include <ble_sniffer/bluetooth_at_driver.h>
 * #include <iostream>
 *
 * int main() {
 *     ble_sniffer::BluetoothATDriver driver;
 *
 *     // Query device information
 *     std::cout << driver.device_info() << std::endl;
 *
 *     // Start scanning and read results
 *     driver.start_scan();
 *     while (true) {
 *         auto msg = driver.read_line();
 *         if (msg.type() == ble_sniffer::MessageType::SCAN_RESULT) {
 *             auto scan = ble_sniffer::ScanResultMessage::from(msg);
 *             std::cout << scan.mac_address() << "," << scan.rssi() << std::endl;
 *         }
 *     }
 *     driver.stop_scan();
 * }
 * @endcode
 */

#include <ble_sniffer/messages.h>
#include <string>
#include <termios.h>

namespace ble_sniffer {

// --- Device configuration ---

/// Default baud rate matching the device factory setting.
constexpr speed_t sniffer_baud_rate = B115200;

/// Read timeout in milliseconds before returning NO_DATA.
/// @note Range constraint: 100ms–25500ms (25.5s). VTIME is an unsigned char
///       representing tenths of a second (0–255). Values below 100ms would
///       yield VTIME=0 (immediate return), and values above 25500ms would
///       overflow cc_t.
constexpr int sniffer_timeout = 1000;
static_assert(sniffer_timeout >= 100, "VTIME requires timeout >= 100ms");
static_assert(sniffer_timeout / 100 <= 255, "VTIME exceeds cc_t max (25.5s)");

/**
 * @brief Serial driver for the ABSniffer 528 BLE sniffer.
 *
 * Opens the serial port on construction and closes it on destruction (RAII).
 * All communication is synchronous and blocking up to the configured timeout.
 *
 * @example
 * @code
 * ble_sniffer::BluetoothATDriver driver;
 *
 * // Check if device is responsive
 * auto status = driver.query_status();
 * if (status.type() == ble_sniffer::MessageType::STATUS) {
 *     std::cout << "Device OK" << std::endl;
 * }
 *
 * // Change baud rate
 * driver.set_baud_rate(ble_sniffer::BaudRate::BAUD_230400);
 *
 * // Enable active scanning mode
 * driver.set_scan_mode(ble_sniffer::ScanMode::ACTIVE);
 *
 * // Restart the device
 * driver.reset_device();
 * @endcode
 */
struct BluetoothATDriver {
    /**
     * @brief Construct and open the serial port.
     * @param device Serial device path (e.g. "/dev/cu.usbmodemXXX" on macOS,
     *               "/dev/ttyUSB0" on Linux). Defaults to the factory device path.
     *
     * @example
     * @code
     * // Use default device path
     * ble_sniffer::BluetoothATDriver driver;
     *
     * // Specify a custom device path
     * ble_sniffer::BluetoothATDriver driver("/dev/ttyUSB0");
     * @endcode
     */
    explicit BluetoothATDriver(const std::string& device);
    ~BluetoothATDriver();

    /**
     * @brief Open and configure the serial port.
     * @return true if the port was opened and configured successfully, false otherwise.
     *
     * Configures: 115200 baud, 8N1, no flow control, non-canonical mode.
     * Called automatically by the constructor. Check is_open() after construction
     * to verify success.
     *
     * @example
     * @code
     * ble_sniffer::BluetoothATDriver driver("/dev/ttyUSB0");
     * if (!driver.is_open()) {
     *     std::cerr << "Failed to open device" << std::endl;
     * }
     * @endcode
     */
    bool init();

    /**
     * @brief Check whether the serial port is open and ready for communication.
     * @return true if the port is open, false if init() failed or the port was closed.
     *
     * @example
     * @code
     * ble_sniffer::BluetoothATDriver driver("/dev/ttyUSB0");
     * if (driver.is_open()) {
     *     driver.start_scan();
     * }
     * @endcode
     */
    bool is_open() const { return m_file_descriptor >= 0; }

    /**
     * @brief Read one complete response line from the device.
     * @return A RawMessage parsed from the line, or a sentinel
     *         (NO_DATA on timeout, ERROR on read failure).
     *
     * @example
     * @code
     * auto msg = driver.read_line();
     * if (msg.type() != ble_sniffer::MessageType::NO_DATA) {
     *     std::cout << msg.prefix() << ": " << msg.data() << std::endl;
     * }
     * @endcode
     */
    RawMessage read_line();

    /**
     * @brief Send an AT command to the device.
     * @param command The command string (e.g. "AT+SCAN1").
     * @param params  Optional parameter appended directly (e.g. "4" for AT+BAUD4).
     *
     * The \\r\\n terminator is added automatically.
     *
     * @example
     * @code
     * driver.send_command("AT+BAUD", "4"); // Sends "AT+BAUD4\r\n"
     * driver.send_command("AT");           // Sends "AT\r\n"
     * @endcode
     */
    void send_command(const std::string& command, const std::string& params = "");

    /**
     * @brief Query device status (AT command).
     * @return RawMessage with type STATUS if device is responsive.
     */
    RawMessage query_status();

    /**
     * @brief Query the device MAC address (AT+ADDR?).
     * @return RawMessage with type ADDRESS containing the hex address.
     */
    RawMessage query_address();

    /**
     * @brief Query firmware version (AT+VERS?).
     * @return RawMessage with type VERSION containing the version string.
     */
    RawMessage query_version();

    /**
     * @brief Get a formatted summary of device status, address, and version.
     * @return Multi-line string with device information.
     */
    std::string device_info();

    /// Start BLE advertisement scanning (AT+SCAN1).
    void start_scan();

    /// Stop BLE advertisement scanning (AT+SCAN0).
    void stop_scan();

    /**
     * @brief Set the device baud rate (AT+BAUD).
     * @param baud_rate The desired baud rate.
     */
    void set_baud_rate(BaudRate baud_rate);

    /**
     * @brief Set active or passive scan mode (AT+ACT).
     * @param scan_mode The desired scan mode.
     */
    void set_scan_mode(ScanMode scan_mode);

    /// Restart the device module (AT+RST).
    void reset_device();

private:
    std::string m_device;
    int m_file_descriptor;
    std::string m_read_buffer;

    /// Maximum read buffer size (64KB). Overflow clears the buffer and returns error.
    static constexpr size_t MAX_READ_BUFFER = 65536;
};

} // namespace ble_sniffer
