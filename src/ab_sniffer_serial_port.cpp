/** 
 * @file ab_sniffer_serial_port.cpp
 * @brief POSIX serial port implementation for the ABSniffer 528 BLE sniffer.
 *
 * Uses the termios API to configure and manage the serial port.
 * The implementation supports reading and writing lines of text (AT commands),
 * with a configurable timeout.
 * @see https://wiki.aprbrother.com/en/AT_Commands_For_ABSniffer_528.html
 */
#include "ble_sniffer/ab_sniffer_serial_port.h"
#include <termios.h>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>

// Convert a serial::BaudRate enum to the corresponding termios speed_t value.
static speed_t baud_rate_to_speed_t(serial::BaudRate baud_rate) {
    switch (baud_rate) {
        case serial::BaudRate::BAUD_300: return B300;
        case serial::BaudRate::BAUD_1200: return B1200;
        case serial::BaudRate::BAUD_2400: return B2400;
        case serial::BaudRate::BAUD_4800: return B4800;
        case serial::BaudRate::BAUD_9600: return B9600;
        case serial::BaudRate::BAUD_19200: return B19200;
        case serial::BaudRate::BAUD_38400: return B38400;
        case serial::BaudRate::BAUD_57600: return B57600;
        case serial::BaudRate::BAUD_115200: return B115200;
        case serial::BaudRate::BAUD_230400: return B230400;
        default:
            throw std::invalid_argument("Unsupported baud rate");
    }
}

struct serial::ABSnifferSerialPort::Impl {
    // Device path (e.g., "/dev/ttyUSB0")
    std::string device;
    // The original terminal settings to restore on close (empty until init() succeeds)
    std::optional<struct termios> original_tty;
    // File descriptor for the opened serial port
    int file_descriptor = -1;
    /// Read timeout in milliseconds before returning NO_DATA.
    /// @note Range constraint: 100ms–25500ms (25.5s).
    const uint16_t timeout_ms;
    // Current baud rate (can change with AT+BAUD command)
    BaudRate current_baud_rate;
    // Serial port configuration defaults for ABSniffer 528
    static constexpr Parity PARITY = serial::Parity::NONE;
    static constexpr StopBits STOP_BITS = serial::StopBits::ONE;
    static constexpr DataBits DATA_BITS = serial::DataBits::EIGHT;
    static constexpr FlowControl FLOW_CONTROL = serial::FlowControl::NONE;

    explicit Impl(const std::string& dev, uint16_t timeout, BaudRate baud)
        : device(dev), timeout_ms(timeout), current_baud_rate(baud) {}
};

namespace serial {

   ABSnifferSerialPort::ABSnifferSerialPort(const std::string& device, 
    const uint16_t timeout_ms,
    const BaudRate baud_rate) 
    : m_impl(std::make_unique<Impl>(device, timeout_ms, baud_rate)) {
            if (m_impl->timeout_ms < 100 || m_impl->timeout_ms > 25500) {
                throw std::invalid_argument("Timeout must be between 100ms and 25500ms");
            }
        }
    

    ABSnifferSerialPort::~ABSnifferSerialPort() {
        close_connection();
    }   
     
    bool ABSnifferSerialPort::init() {
        m_impl->file_descriptor = open(m_impl->device.c_str(), O_RDWR | O_NOCTTY | O_SYNC);

        if (m_impl->file_descriptor < 0) {
            std::cerr << "Error opening device: " << m_impl->device << std::endl;
            throw serial::SerialPortException("Failed to open serial port: " + m_impl->device);
        }

        struct termios tty;
        if (tcgetattr(m_impl->file_descriptor, &tty) != 0) {
            throw serial::SerialPortException("Failed to get terminal attributes: " + m_impl->device);
        }
        // Save the original terminal settings to restore on close
        m_impl->original_tty = tty;

        // Baud rate: 115200 (device default)
        cfsetospeed(&tty, baud_rate_to_speed_t(m_impl->current_baud_rate));
        cfsetispeed(&tty, baud_rate_to_speed_t(m_impl->current_baud_rate));

        // 8 data bits
        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
        // No parity
        tty.c_cflag &= ~PARENB;
        // 1 stop bit (CSTOPB set = 2 stop bits)
        tty.c_cflag &= ~CSTOPB;
        // No hardware flow control
        tty.c_cflag &= ~CRTSCTS;
        // Enable receiver; ignore modem control lines (required for Linux USB serial)
        tty.c_cflag |= (CLOCAL | CREAD);

        // Disable software flow control and input byte translation
        tty.c_iflag &= ~(IGNBRK | IXON | IXOFF | IXANY | ICRNL | INLCR | IGNCR);
        // Non-canonical mode: no echo, no line editing
        tty.c_lflag = 0;
        // No output processing
        tty.c_oflag = 0;

        // Non-blocking read with timeout
        tty.c_cc[VMIN]  = 0;
        tty.c_cc[VTIME] = static_cast<cc_t>(m_impl->timeout_ms / 100);

        if (tcsetattr(m_impl->file_descriptor, TCSANOW, &tty) != 0) {
            throw serial::SerialPortException("Failed to set terminal attributes: " + m_impl->device);
        }

        // Discard any stale data in buffers
        tcflush(m_impl->file_descriptor, TCIOFLUSH);
        return true;
    }

    bool ABSnifferSerialPort::set_baud_rate(BaudRate baud_rate) {
        if (!is_open()) return false;
        struct termios tty;
        if (tcgetattr(m_impl->file_descriptor, &tty) != 0) {
            throw serial::SerialPortException("Failed to get terminal attributes: " + m_impl->device);
        }
        cfsetospeed(&tty, baud_rate_to_speed_t(baud_rate));
        cfsetispeed(&tty, baud_rate_to_speed_t(baud_rate));
        if (tcsetattr(m_impl->file_descriptor, TCSANOW, &tty) != 0) {
            throw serial::SerialPortException("Failed to set terminal attributes: " + m_impl->device);
        }
        m_impl->current_baud_rate = baud_rate;
        return true;
    }

    BaudRate ABSnifferSerialPort::get_baud_rate() {
        return m_impl->current_baud_rate;
    }

    void ABSnifferSerialPort::close_connection() {
        if (is_open()) {
            // Best-effort cleanup: failures are ignored to ensure the fd is always closed.
            // tcdrain is skipped during close to avoid blocking in destructor paths.
            tcflush(m_impl->file_descriptor, TCIOFLUSH);
            if (m_impl->original_tty.has_value()) {
                tcsetattr(m_impl->file_descriptor, TCSANOW, &m_impl->original_tty.value());
            }
            ::close(m_impl->file_descriptor);
            m_impl->file_descriptor = -1;
        }
    }

    bool ABSnifferSerialPort::is_open() const {
        return m_impl->file_descriptor >= 0;
    }

    /**
     * @brief Read data from the serial port (blocking).
     * @param buffer Destination buffer for the data. Not null-terminated; use the return value as the byte count.
     * @param max_length Maximum number of bytes to read.
     * @return Number of bytes read, or 0 on error/timeout.
     */
    std::size_t ABSnifferSerialPort::read(char* buffer, std::size_t max_length) {
        if (!is_open()) return 0;
        ssize_t bytes_read = ::read(m_impl->file_descriptor, buffer, max_length);
        if (bytes_read < 0) {
            std::cerr << "Read error: " << strerror(errno) << std::endl;
            throw serial::SerialReadException("Failed to read from serial port: " + m_impl->device);
        }
        return static_cast<std::size_t>(bytes_read);
    }

    std::size_t ABSnifferSerialPort::write(const char* data, std::size_t length) {
        if (!is_open()) return 0;
        ssize_t total_written = ::write(m_impl->file_descriptor, data, length);
        if (total_written < 0) {
            std::cerr << "Write error: " << strerror(errno) << std::endl;
            throw serial::SerialWriteException("Failed to write to serial port: " + m_impl->device);
        }
        return static_cast<std::size_t>(total_written);
    }

}