#pragma once

/**
 * @file ab_sniffer_serial_port.h
 * @brief POSIX serial port implementation for the ABSniffer 528 BLE sniffer.
 *
 * Implements the serial::SerialPort interface using the POSIX termios API.
 * All platform-specific (termios) details are hidden in the implementation
 * file — this header exposes only platform-independent types.
 *
 * @example
 * @code
 * auto port = std::make_unique<serial::ABSnifferSerialPort>("/dev/ttyUSB0");
 * if (!port->init()) {
 *     std::cerr << "Failed to open serial port" << std::endl;
 *     return 1;
 * }
 * // port is ready for AT command communication
 * @endcode
 */

#include <ble_sniffer/SerialPort.h>
#include <ble_sniffer/types.h>
#include <memory>
#include <string>

namespace serial {  

/**
 * @brief POSIX serial port implementation for the ABSniffer 528.
 *
 * Uses termios for serial communication. Platform-specific details
 * are hidden behind a pimpl pointer so that including this header
 * does not require <termios.h>.
 */
struct ABSnifferSerialPort : public SerialPort {
        
    /// Default baud rate for the ABSniffer 528.
    /// @see ble_sniffer::sniffer_baud_rate_bps
    static constexpr int SNIFFER_DEFAULT_BAUD_RATE_BPS = ble_sniffer::sniffer_baud_rate_bps;
        
    /**
     * @brief Construct a serial port for the given device path.
     * @param device Serial device path (e.g. "/dev/ttyUSB0").
     * @param timeout_ms Read timeout in milliseconds (100–25500). Default: 1000.
     * @param baud_rate Initial baud rate for the serial connection. Default: 115200.
     *
     * @example
     * @code
     * auto port = std::make_unique<serial::ABSnifferSerialPort>("/dev/ttyUSB0");
     * @endcode
     */
    explicit ABSnifferSerialPort(const std::string& device, uint16_t timeout_ms = 1000, 
        BaudRate baud_rate = serial::baud_rate_from_num(SNIFFER_DEFAULT_BAUD_RATE_BPS));
    
    /// @brief Close the serial port and release resources.
    ~ABSnifferSerialPort() override;

    /**
     * @brief Open and configure the serial port.
     * @return true on success, false if the port could not be opened or configured.
     *
     * @example
     * @code
     * auto port = std::make_unique<serial::ABSnifferSerialPort>("/dev/ttyUSB0");
     * if (!port->init()) {
     *     std::cerr << "Failed to open serial port" << std::endl;
     *     return 1;
     * }
     * @endcode
     */
    bool init() override;

    /**
     * @brief Change the serial port baud rate.
     * @param baud_rate The new baud rate.
     * @return true on success, false if the baud rate could not be changed.
     *
     * @example
     * @code
     * port->set_baud_rate(serial::BaudRate::BAUD_230400);
     * @endcode
     */
    bool set_baud_rate(BaudRate baud_rate) override;

    /**
     * @brief Get the current baud rate of the serial port.
     * @return The current baud rate.
     *
     * @example
     * @code
     * serial::BaudRate baud = port->get_baud_rate();
     * @endcode
     */
    BaudRate get_baud_rate() override;

    /// @brief Check whether the serial port is open. @return true if open.
    bool is_open() const override;

    /**
     * @brief Read data from the serial port (blocking).
     * @param buffer Destination buffer for the data.
     * @param max_length Max bytes to read.
     * @return Bytes read, 0 on timeout or if port is not open.
     * @throws serial::SerialReadException on read failure (e.g. device disconnect).
     *
     * @example
     * @code
     * char buf[256];
     * std::size_t n = port->read(buf, sizeof(buf));
     * if (n > 0) {
     *     std::cout << "Read " << n << " bytes" << std::endl;
     * }
     * @endcode
     */
    std::size_t read(char* buffer, std::size_t max_length) override;

    /**
     * @brief Write data to the serial port (blocking).
     * @param data Source buffer.
     * @param length Number of bytes to write.
     * @return Bytes written, 0 if port is not open.
     * @throws serial::SerialWriteException on write failure.
     *
     * @example
     * @code
     * const char* cmd = "AT+SCAN1\r\n";
     * std::size_t n = port->write(cmd, std::strlen(cmd));
     * @endcode
     */
    std::size_t write(const char* data, std::size_t length) override;

    /**
     * @brief Close the serial port and restore original terminal settings.
     *
     * This is a best-effort teardown operation; it returns void because
     * close failures cannot meaningfully be acted upon. Callers who need
     * to verify the port was closed can check is_open() afterwards.
     *
     * @example
     * @code
     * port->close_connection();
     * assert(!port->is_open());
     * @endcode
     */
    void close_connection() override;

private:
    /// @brief Opaque implementation pointer (hides termios from this header).
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace serial