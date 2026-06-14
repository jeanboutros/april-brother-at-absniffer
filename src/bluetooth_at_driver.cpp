/**
 * @file bluetooth_at_driver.cpp
 * @brief Implementation of the serial driver for ABSniffer 528.
 */

#include <ble_sniffer/bluetooth_at_driver.h>

#include <stdexcept>
#include <iostream>
#include <ble_sniffer/SerialPort.h>

namespace ble_sniffer {

BluetoothATDriver::BluetoothATDriver(std::unique_ptr<serial::SerialPort> serial_port) : m_serial_port(std::move(serial_port)) {
    if (!m_serial_port) {
        throw std::invalid_argument("serial_port must not be null");
    }
    if (!m_serial_port->is_open()) {
        throw std::invalid_argument("serial_port must be open before constructing BluetoothATDriver (call init() first)");
    }
}

BluetoothATDriver::~BluetoothATDriver() {
    if (m_serial_port->is_open()) {
        stop_scan();
        reset_device();
        m_serial_port->close_connection();
    }
    std::cout << "Bluetooth AT Driver cleaned up." << std::endl;
}

void BluetoothATDriver::send_command(const std::string& command, const std::string& params) {
    if (!m_serial_port->is_open()) return;
    std::string full_command = command;
    if (!params.empty()) {
        full_command += params;
    }
    full_command += COMMAND_DELIMITER.data();
    try {
        std::size_t written = m_serial_port->write(full_command.c_str(), full_command.size());
        if (written != full_command.size()) {
            std::cerr << "Error writing command: partial write (" << written << "/" << full_command.size() << " bytes)" << std::endl;
        }
    } catch (const serial::SerialWriteException& e) {
        std::cerr << "Serial write exception: " << e.what() << std::endl;
    }
}

RawMessage BluetoothATDriver::read_line() {
    if (!m_serial_port->is_open()) return RawMessage::error();
    char buf[512];
    while (true) {
        try {
            std::size_t n = m_serial_port->read(buf, sizeof(buf));
            if (n > 0) {
                m_read_buffer.append(buf, n);
                if (m_read_buffer.size() > serial::SerialPort::MAX_READ_BUFFER) {
                    std::cerr << "Read buffer overflow, clearing" << std::endl;
                    m_read_buffer.clear();
                    return RawMessage::error();
                }
                // Check for a complete line
                auto pos = m_read_buffer.find(COMMAND_DELIMITER.data());
                if (pos != std::string::npos) {
                    std::string line = m_read_buffer.substr(0, pos);
                    m_read_buffer.erase(0, pos + COMMAND_DELIMITER.size());
                    return RawMessage::parse(line);
                }
            } else if (n == 0) {
                std::cerr << "Read timeout or no data available" << std::endl;
                return RawMessage::no_data();
            }
        } catch (const serial::SerialReadException& e) {
            std::cerr << "Serial read error: " << e.what() << std::endl;
            return RawMessage::error();
        }
    }
    return RawMessage::no_data();
}

RawMessage BluetoothATDriver::query_status() {
    if (!is_connected()) return RawMessage::error();
    send_command(AT.data());
    return read_line();
}

RawMessage BluetoothATDriver::query_address() {
    if (!is_connected()) return RawMessage::error();
    send_command(AT_ADDR.data());
    return read_line();
}

RawMessage BluetoothATDriver::query_version() {
    if (!is_connected()) return RawMessage::error();
    send_command(AT_VERS.data());
    return read_line();
}

std::string BluetoothATDriver::device_info() {
    if (!is_connected()) return "Device Info:\nPort not open\n";
    std::string info = "Device Info:\n";
    info += "Status: " + query_status().prefix() + "\n";
    info += "Address: " + address_to_mac_address(query_address().data()) + "\n";
    info += "Version: " + query_version().data() + "\n";
    return info;
}

void BluetoothATDriver::start_scan() {
    if (!is_connected()) return;
    send_command(AT_SCAN1.data());
}

void BluetoothATDriver::stop_scan() {
    if (!is_connected()) return;
    send_command(AT_SCAN0.data());
}

void BluetoothATDriver::set_baud_rate(AtBaudParam baud_rate) {
    if (!is_connected()) return;
    send_command(AT_BAUD.data(), std::to_string(static_cast<int>(baud_rate)));
    // Update the serial port's baud rate to match the device
    int sniffer_baud_rate = at_baud_param_to_num<int>(baud_rate);
    // Convert the baud rate to the corresponding serial::BaudRate enum
    serial::BaudRate serial_baud_rate = serial::baud_rate_from_num(sniffer_baud_rate);
    if (!m_serial_port->set_baud_rate(serial_baud_rate)) {
        std::cerr << "Failed to set serial port baud rate to " << sniffer_baud_rate << std::endl;
    }
}
    

void BluetoothATDriver::set_scan_mode(ScanMode scan_mode) {
    if (!is_connected()) return;
    send_command(AT_ACT.data(), std::to_string(static_cast<int>(scan_mode)));
}

void BluetoothATDriver::reset_device() {
    if (!is_connected()) return;
    send_command(AT_RST.data());
}

bool BluetoothATDriver::is_connected() {
    return m_serial_port->is_open();
}


} // namespace ble_sniffer