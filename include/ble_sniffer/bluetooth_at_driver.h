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
 * #include <ble_sniffer/ab_sniffer_serial_port.h>
 * #include <iostream>
 *
 * int main() {
 *     auto port = std::make_unique<serial::ABSnifferSerialPort>("/dev/ttyUSB0");
 *     port->init();
 *     ble_sniffer::BluetoothATDriver driver(std::move(port));
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
#include <ble_sniffer/SerialPort.h>
#include <memory>
#include <string>

namespace ble_sniffer {


/**
 * @brief Serial driver for the ABSniffer 528 BLE sniffer.
 *
 * Takes ownership of an already-initialized serial port on construction and closes it on destruction (RAII).
 * The port must be open (is_open() returns true) before construction; otherwise std::invalid_argument is thrown.
 * All communication is synchronous and blocking up to the configured timeout.
 *
 * @example
 * @code
 * auto port = std::make_unique<serial::ABSnifferSerialPort>("/dev/ttyUSB0");
 * port->init();
 * ble_sniffer::BluetoothATDriver driver(std::move(port));
 *
 * // Check if device is responsive
 * auto status = driver.query_status();
 * if (status.type() == ble_sniffer::MessageType::STATUS) {
 *     std::cout << "Device OK" << std::endl;
 * }
 *
 * // Change baud rate
 * driver.set_baud_rate(ble_sniffer::AtBaudParam::BAUD_230400);
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
     * @brief Construct a driver with the given serial port.
     * @param serial_port Ownership of a SerialPort implementation (moved in).
     *
     * @example
     * @code
     * auto port = std::make_unique<serial::ABSnifferSerialPort>("/dev/ttyUSB0");
     * port->init();
     * ble_sniffer::BluetoothATDriver driver(std::move(port));
     * @endcode
     */
    explicit BluetoothATDriver(std::unique_ptr<serial::SerialPort> serial_port);
    ~BluetoothATDriver();

    BluetoothATDriver(const BluetoothATDriver&) = delete;
    BluetoothATDriver& operator=(const BluetoothATDriver&) = delete;
    BluetoothATDriver(BluetoothATDriver&&) = delete;
    BluetoothATDriver& operator=(BluetoothATDriver&&) = delete;


    /**
     * @brief Check whether the serial port is open and ready for communication.
     * @return true if the port is open, false if init() failed or the port was closed.
     *
     * @example
     * @code
     * if (driver.is_connected()) {
     *     driver.start_scan();
     * }
     * @endcode
     */
    bool is_connected();

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
     *
     * @example
     * @code
     * auto status = driver.query_status();
     * if (status.type() == ble_sniffer::MessageType::STATUS) {
     *     std::cout << "Device responsive: " << status.data() << std::endl;
     * }
     * @endcode
     */
    RawMessage query_status();

    /**
     * @brief Query the device MAC address (AT+ADDR?).
     * @return RawMessage with type ADDRESS containing the hex address.
     *
     * @example
     * @code
     * auto addr = driver.query_address();
     * if (addr.type() == ble_sniffer::MessageType::ADDRESS) {
     *     std::cout << "MAC: " << addr.data() << std::endl;
     * }
     * @endcode
     */
    RawMessage query_address();

    /**
     * @brief Query firmware version (AT+VERS?).
     * @return RawMessage with type VERSION containing the version string.
     *
     * @example
     * @code
     * auto ver = driver.query_version();
     * if (ver.type() == ble_sniffer::MessageType::VERSION) {
     *     std::cout << "Firmware: " << ver.data() << std::endl;
     * }
     * @endcode
     */
    RawMessage query_version();

    /**
     * @brief Get a formatted summary of device status, address, and version.
     * @return Multi-line string with device information.
     *
     * @example
     * @code
     * std::cout << driver.device_info() << std::endl;
     * // Output:
     * // Device Info:
     * // Status: OK
     * // Address: AA:BB:CC:DD:EE:FF
     * // Version: V1.0.0
     * @endcode
     */
    std::string device_info();

    /**
     * @brief Start BLE advertisement scanning (AT+SCAN1).
     *
     * @example
     * @code
     * driver.start_scan();
     * while (true) {
     *     auto msg = driver.read_line();
     *     if (msg.type() == ble_sniffer::MessageType::SCAN_RESULT) {
     *         auto scan = ble_sniffer::ScanResultMessage::from(msg);
     *         std::cout << scan.mac_address() << std::endl;
     *     }
     * }
     * @endcode
     */
    void start_scan();

    /**
     * @brief Stop BLE advertisement scanning (AT+SCAN0).
     *
     * @example
     * @code
     * driver.stop_scan();
     * @endcode
     */
    void stop_scan();

    /**
     * @brief Set the device baud rate (AT+BAUD).
     * @param baud_rate The desired AT baud rate parameter.
     *
     * @example
     * @code
     * driver.set_baud_rate(ble_sniffer::AtBaudParam::BAUD_230400);
     * @endcode
     */
    void set_baud_rate(AtBaudParam baud_rate);

    /**
     * @brief Get the current serial port baud rate as an integer.
     * @return The baud rate in bps (e.g. 115200).
     * @throws std::runtime_error if the port is not connected.
     *
     * @example
     * @code
     * int rate = driver.get_baud_rate();
     * // Returns: 115200 (or throws if not connected)
     * @endcode
     */
    int get_baud_rate();

    /**
     * @brief Get the current baud rate as an AT command parameter.
     *
     * Converts the current serial port baud rate to the corresponding
     * AtBaudParam enum value for use with set_baud_rate().
     *
     * @return The AT baud rate parameter matching the current port speed.
     * @throws std::runtime_error if the port is not connected.
     * @throws std::invalid_argument if the current baud rate is unsupported.
     *
     * @example
     * @code
     * ble_sniffer::AtBaudParam param = driver.get_at_baud_rate();
     * // Returns: AtBaudParam::BAUD_115200
     * @endcode
     */
    AtBaudParam get_at_baud_rate();

    /**
     * @brief Set active or passive scan mode (AT+ACT).
     * @param scan_mode The desired scan mode.
     *
     * @example
     * @code
     * driver.set_scan_mode(ble_sniffer::ScanMode::ACTIVE);
     * @endcode
     */
    void set_scan_mode(ScanMode scan_mode);

    /**
     * @brief Restart the device module (AT+RST).
     *
     * @example
     * @code
     * driver.reset_device();
     * @endcode
     */
    void reset_device();

private:
    std::unique_ptr<serial::SerialPort> m_serial_port;
    std::string m_read_buffer;


};

} // namespace ble_sniffer
