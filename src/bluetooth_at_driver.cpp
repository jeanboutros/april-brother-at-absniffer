/**
 * @file bluetooth_at_driver.cpp
 * @brief Implementation of the serial driver for ABSniffer 528.
 */

#include <ble_sniffer/bluetooth_at_driver.h>

#include <iostream>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

namespace ble_sniffer {

BluetoothATDriver::BluetoothATDriver(const std::string& device) : m_device(device), m_file_descriptor(-1) {
    init();
}

BluetoothATDriver::~BluetoothATDriver() {
    if (m_file_descriptor >= 0) {
        stop_scan();
        reset_device();
        close(m_file_descriptor);
        m_file_descriptor = -1;
    }
    std::cout << "Bluetooth AT Driver cleaned up." << std::endl;
}

bool BluetoothATDriver::init() {
    m_file_descriptor = open(m_device.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (m_file_descriptor < 0) {
        std::cerr << "Error opening device: " << m_device << std::endl;
        return false;
    }

    struct termios tty;
    if (tcgetattr(m_file_descriptor, &tty) != 0) {
        std::cerr << "Error getting terminal attributes" << std::endl;
        close(m_file_descriptor);
        m_file_descriptor = -1;
        return false;
    }

    // Baud rate: 115200 (device default)
    cfsetospeed(&tty, sniffer_baud_rate);
    cfsetispeed(&tty, sniffer_baud_rate);

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
    tty.c_cc[VTIME] = static_cast<cc_t>(sniffer_timeout / 100);

    if (tcsetattr(m_file_descriptor, TCSANOW, &tty) != 0) {
        std::cerr << "Error setting terminal attributes" << std::endl;
        close(m_file_descriptor);
        m_file_descriptor = -1;
        return false;
    }

    // Discard any stale data in buffers
    tcflush(m_file_descriptor, TCIOFLUSH);
    return true;
}

void BluetoothATDriver::send_command(const std::string& command, const std::string& params) {
    if (!is_open()) return;
    std::string full_command = command;
    if (!params.empty()) {
        full_command += params;
    }
    full_command += COMMAND_DELIMITER.data();
    ssize_t written = write(m_file_descriptor, full_command.c_str(), full_command.size());
    if (written < 0 || static_cast<size_t>(written) != full_command.size()) {
        std::cerr << "Error writing command: " << strerror(errno) << std::endl;
    }
}

RawMessage BluetoothATDriver::read_line() {
    if (!is_open()) return RawMessage::error();
    char buf[512];
    while (true) {
        ssize_t n = read(m_file_descriptor, buf, sizeof(buf));
        if (n > 0) {
            m_read_buffer.append(buf, n);
            if (m_read_buffer.size() > MAX_READ_BUFFER) {
                std::cerr << "Read buffer overflow, clearing" << std::endl;
                m_read_buffer.clear();
                return RawMessage::error();
            }
            // Check for a complete line
            auto pos = m_read_buffer.find("\r\n");
            if (pos != std::string::npos) {
                std::string line = m_read_buffer.substr(0, pos);
                m_read_buffer.erase(0, pos + 2);
                return RawMessage::parse(line);
            }
        } else if (n == 0) {
            break; // timeout
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "read error: " << strerror(errno) << std::endl;
            return RawMessage::error();
        }
    }
    return RawMessage::no_data();
}

RawMessage BluetoothATDriver::query_status() {
    if (!is_open()) return RawMessage::error();
    send_command(AT.data());
    return read_line();
}

RawMessage BluetoothATDriver::query_address() {
    if (!is_open()) return RawMessage::error();
    send_command(AT_ADDR.data());
    return read_line();
}

RawMessage BluetoothATDriver::query_version() {
    if (!is_open()) return RawMessage::error();
    send_command(AT_VERS.data());
    return read_line();
}

std::string BluetoothATDriver::device_info() {
    if (!is_open()) return "Device Info:\nPort not open\n";
    std::string info = "Device Info:\n";
    info += "Status: " + query_status().prefix() + "\n";
    info += "Address: " + address_to_mac_address(query_address().data()) + "\n";
    info += "Version: " + query_version().data() + "\n";
    return info;
}

void BluetoothATDriver::start_scan() {
    if (!is_open()) return;
    send_command(AT_SCAN1.data());
}

void BluetoothATDriver::stop_scan() {
    if (!is_open()) return;
    send_command(AT_SCAN0.data());
}

void BluetoothATDriver::set_baud_rate(BaudRate baud_rate) {
    if (!is_open()) return;
    send_command(AT_BAUD.data(), std::to_string(static_cast<int>(baud_rate)));
}

void BluetoothATDriver::set_scan_mode(ScanMode scan_mode) {
    if (!is_open()) return;
    send_command(AT_ACT.data(), std::to_string(static_cast<int>(scan_mode)));
}

void BluetoothATDriver::reset_device() {
    if (!is_open()) return;
    send_command(AT_RST.data());
}

} // namespace ble_sniffer