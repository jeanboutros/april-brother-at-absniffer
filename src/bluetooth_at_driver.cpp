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

BluetoothATDriver::BluetoothATDriver(const std::string& device) : m_device(device), file_descriptor(-1) {
    init();
}

BluetoothATDriver::~BluetoothATDriver() {
    if (file_descriptor >= 0) {
        close(file_descriptor);
        file_descriptor = -1;
    }
    BluetoothATDriver::stop_scan(); // Ensure scanning is stopped on exit
    BluetoothATDriver::reset_device(); // Reset device to clean state
    std::cout << "Bluetooth AT Driver cleaned up." << std::endl;
}

void BluetoothATDriver::init() {
    file_descriptor = open(m_device.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (file_descriptor < 0) {
        std::cerr << "Error opening device: " << m_device << std::endl;
        return;
    }

    struct termios tty;
    if (tcgetattr(file_descriptor, &tty) != 0) {
        std::cerr << "Error getting terminal attributes" << std::endl;
        close(file_descriptor);
        file_descriptor = -1;
        return;
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

    // No break processing
    tty.c_iflag &= ~IGNBRK;
    // Non-canonical mode: no echo, no line editing
    tty.c_lflag = 0;
    // No output processing
    tty.c_oflag = 0;

    // Non-blocking read with timeout
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = sniffer_timeout / 100; // tenths of a second

    if (tcsetattr(file_descriptor, TCSANOW, &tty) != 0) {
        std::cerr << "Error setting terminal attributes" << std::endl;
        close(file_descriptor);
        file_descriptor = -1;
        return;
    }

    // Discard any stale data in buffers
    tcflush(file_descriptor, TCIOFLUSH);
}

void BluetoothATDriver::send_command(const std::string& command, const std::string& params) {
    std::string full_command = command;
    if (!params.empty()) {
        full_command += params;
    }
    full_command += COMMAND_DELIMITER.data();
    write(file_descriptor, full_command.c_str(), full_command.size());
}

RawMessage BluetoothATDriver::read_line() {
    char buf[512];
    while (true) {
        ssize_t n = read(file_descriptor, buf, sizeof(buf));
        if (n > 0) {
            read_buffer.append(buf, n);
            // Check for a complete line
            auto pos = read_buffer.find("\r\n");
            if (pos != std::string::npos) {
                std::string line = read_buffer.substr(0, pos);
                read_buffer.erase(0, pos + 2);
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
    send_command(AT.data());
    return read_line();
}

RawMessage BluetoothATDriver::query_address() {
    send_command(AT_ADDR.data());
    return read_line();
}

RawMessage BluetoothATDriver::query_version() {
    send_command(AT_VERS.data());
    return read_line();
}

std::string BluetoothATDriver::device_info() {
    std::string info = "Device Info:\n";
    info += "Status: " + query_status().prefix() + "\n";
    info += "Address: " + address_to_mac_address(query_address().data()) + "\n";
    info += "Version: " + query_version().data() + "\n";
    return info;
}

void BluetoothATDriver::start_scan() {
    send_command(AT_SCAN1.data());
}

void BluetoothATDriver::stop_scan() {
    send_command(AT_SCAN0.data());
}

void BluetoothATDriver::set_baud_rate(BaudRate baud_rate) {
    send_command(AT_BAUD.data(), std::to_string(static_cast<int>(baud_rate)));
}

void BluetoothATDriver::set_scan_mode(ScanMode scan_mode) {
    send_command(AT_ACT.data(), std::to_string(static_cast<int>(scan_mode)));
}

void BluetoothATDriver::reset_device() {
    send_command(AT_RST.data());
}

} // namespace ble_sniffer
