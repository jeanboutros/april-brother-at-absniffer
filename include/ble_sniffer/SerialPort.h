/**
 * @file SerialPort.h
 * @brief Platform-independent serial port interface and supporting types.
 *
 * Defines the serial::SerialPort abstract interface for reading and writing
 * data over a serial connection, along with configuration enums (BaudRate,
 * Parity, StopBits, DataBits, FlowControl) and exception types.
 *
 * @example
 * @code
 * #include <ble_sniffer/SerialPort.h>
 * #include <ble_sniffer/ab_sniffer_serial_port.h>
 *
 * auto port = std::make_unique<serial::ABSnifferSerialPort>("/dev/ttyUSB0");
 * if (!port->init()) {
 *     std::cerr << "Failed to open port" << std::endl;
 *     return 1;
 * }
 * port->write("AT\r\n", 4);
 * char buf[256];
 * std::size_t n = port->read(buf, sizeof(buf));
 * port->close_connection();
 * @endcode
 */

#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace serial {

/**
 * @brief Base exception for serial port errors.
 *
 * Thrown for general serial port failures that are not specifically
 * read or write related.
 */
struct SerialPortException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

/**
 * @brief Exception thrown when a serial read operation fails.
 *
 * Thrown by read() when the underlying read system call reports an error
 * (e.g. device disconnect, I/O failure). Timeouts and "not open" conditions
 * return 0 rather than throwing.
 */
struct SerialReadException : public SerialPortException {
    using SerialPortException::SerialPortException;
};

/**
 * @brief Exception thrown when a serial write operation fails.
 *
 * Thrown by write() when the underlying write system call reports an error.
 * A closed port returns 0 rather than throwing.
 */
struct SerialWriteException : public SerialPortException {
    using SerialPortException::SerialPortException;
};

/**
 * @brief Standard serial baud rates.
 * @see https://man7.org/linux/man-pages/man3/termios.3.html
 *
 * @example
 * @code
 * serial::BaudRate rate = serial::BaudRate::BAUD_115200;
 * serial::BaudRate rate2 = serial::baud_rate_from_num(115200);
 * @endcode
 */
enum class BaudRate: int {
    BAUD_300 = 300,
    BAUD_1200 = 1200,
    BAUD_2400 = 2400,
    BAUD_4800 = 4800,
    BAUD_9600 = 9600,
    BAUD_19200 = 19200,
    BAUD_38400 = 38400,
    BAUD_57600 = 57600,
    BAUD_115200 = 115200,
    BAUD_230400 = 230400,
    BAUD_460800 = 460800,
    BAUD_921600 = 921600
};

/**
 * @brief Convert an integer baud rate value to a BaudRate enum.
 *
 * Only standard rates listed in the BaudRate enum are accepted.
 * Throws std::invalid_argument for unsupported values.
 *
 * @tparam T Integer type of the baud rate value.
 * @param baud_rate Numeric baud rate (e.g. 115200).
 * @return Corresponding BaudRate enum value.
 * @throws std::invalid_argument if the value is not a standard baud rate.
 *
 * @example
 * @code
 * serial::BaudRate rate = serial::baud_rate_from_num(115200); // BaudRate::BAUD_115200
 * serial::BaudRate rate2 = serial::baud_rate_from_num(9600);  // BaudRate::BAUD_9600
 * @endcode
 */
template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
static constexpr BaudRate baud_rate_from_num(T baud_rate) {
    switch (static_cast<int>(baud_rate)) {
        case 300: return BaudRate::BAUD_300;
        case 1200: return BaudRate::BAUD_1200;
        case 2400: return BaudRate::BAUD_2400;
        case 4800: return BaudRate::BAUD_4800;
        case 9600: return BaudRate::BAUD_9600;
        case 19200: return BaudRate::BAUD_19200;
        case 38400: return BaudRate::BAUD_38400;
        case 57600: return BaudRate::BAUD_57600;
        case 115200: return BaudRate::BAUD_115200;
        case 230400: return BaudRate::BAUD_230400;
        case 460800: return BaudRate::BAUD_460800;
        case 921600: return BaudRate::BAUD_921600;
        default:
            throw std::invalid_argument("Unsupported baud rate");
    }
}



/**
 * @brief Serial parity modes.
 * @see https://man7.org/linux/man-pages/man3/termios.3.html (PARENB, PARODD)
 *
 * @example
 * @code
 * serial::Parity p = serial::Parity::NONE;
 * @endcode
 */
enum class Parity {
    NONE,
    ODD,
    EVEN
};

/**
 * @brief Number of stop bits per character.
 * @see https://man7.org/linux/man-pages/man3/termios.3.html (CSTOPB)
 *
 * @example
 * @code
 * serial::StopBits sb = serial::StopBits::ONE;
 * @endcode
 */
enum class StopBits {
    ONE = 1,
    TWO = 2
};

/**
 * @brief Number of data bits per character.
 * @see https://man7.org/linux/man-pages/man3/termios.3.html (CS5–CS8)
 *
 * @example
 * @code
 * serial::DataBits db = serial::DataBits::EIGHT;
 * @endcode
 */
enum class DataBits {
    FIVE = 5,
    SIX = 6,
    SEVEN = 7,
    EIGHT = 8
};

/**
 * @brief Serial flow control modes.
 * @see https://man7.org/linux/man-pages/man3/termios.3.html (CRTSCTS, IXON/IXOFF)
 *
 * @example
 * @code
 * serial::FlowControl fc = serial::FlowControl::NONE;
 * @endcode
 */
enum class FlowControl {
    NONE,
    HARDWARE,  ///< RTS/CTS hardware flow control
    SOFTWARE   ///< XON/XOFF software flow control
};

struct SerialPort {
    virtual ~SerialPort() = default;

    /**
     * @brief Open and configure the serial port.
     * @return true if the port was opened and configured successfully, false otherwise.
     */
    virtual bool init() = 0;

    /**
     * @brief Check whether the serial port is open and ready for communication.
     * @return true if the port is open, false if init() failed or the port was closed.
     */
    virtual bool is_open() const = 0;

    /**
     * @brief Read data from the serial port (blocking).
     *
     * Blocks until data is available or the read timeout expires.
     * On timeout or if the port is not open, returns 0.
     * On a read failure (e.g. device disconnect), throws SerialReadException.
     *
     * @param buffer Destination buffer for the data.
     * @param max_length Maximum number of bytes to read (including null terminator).
     * @return Number of bytes read, 0 on timeout or if port is not open.
     * @throws serial::SerialReadException on read failure (e.g. device disconnect).
     */
    virtual std::size_t read(char* buffer, std::size_t max_length) = 0;

    /**
     * @brief Write data to the serial port (blocking).
     *
     * If the port is not open, returns 0.
     * On a write failure, throws SerialWriteException.
     *
     * @param data Pointer to the data to write.
     * @param length Number of bytes to write.
     * @return Number of bytes written, 0 if port is not open.
     * @throws serial::SerialWriteException on write failure.
     */
    virtual std::size_t write(const char* data, std::size_t length) = 0;

    /**
     * @brief Close the serial port and release resources.
     *
     * This is a best-effort teardown operation; it returns void because
     * close failures cannot meaningfully be acted upon. Callers who need
     * to verify the port was closed can check is_open() afterwards.
     */
    virtual void close_connection() = 0;

    /**
     * @brief Change the serial port baud rate.
     * @param baud_rate The desired baud rate.
     * @return true if the baud rate was changed successfully, false otherwise.
     */
    virtual bool set_baud_rate(BaudRate baud_rate) = 0;

    /**
     * @brief Maximum buffer size for a single read operation (64 KB).
     *
     * Used to guard against unbounded buffer growth in line-oriented
     * read loops. If the internal buffer exceeds this limit, it is
     * cleared and an error message is returned.
     */
    static constexpr std::size_t MAX_READ_BUFFER = 65536; // 64KB
};

} // namespace serial