#include "stat_view.h"

#include <ble_sniffer/ad_parser.h>
#include <ble_sniffer/assigned_numbers.h>
#include <ble_sniffer/proprietary_parsers.h>
#include <ble_sniffer/types.h>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <sys/ioctl.h>
#include <unistd.h>

namespace ble_sniffer::stat {

namespace {

// Refresh interval. Slow enough that the eye doesn't see jitter even when the
// tracker is updated dozens of times per second by incoming advertisements.
constexpr std::chrono::milliseconds kRenderInterval{500};

// Filled and empty signal-bar glyphs (UTF-8 ▰ U+25B0 / ▱ U+25B1). 3 bytes each, 1 display column.
constexpr const char* kBarFilled = "\xE2\x96\xB0";
constexpr const char* kBarEmpty  = "\xE2\x96\xB1";
constexpr int         kBarCount  = 5;

// ANSI palette
constexpr const char* kReset      = "\033[0m";
constexpr const char* kBold       = "\033[1m";
constexpr const char* kDim        = "\033[2m";
constexpr const char* kCyan       = "\033[1;36m";
constexpr const char* kFgGreen    = "\033[32m";
constexpr const char* kFgYellow   = "\033[33m";  // used as "orange"
constexpr const char* kFgRed      = "\033[31m";

/// Pad/truncate s to exactly `width` display columns (byte-based; OK for ASCII content).
std::string clip(const std::string& s, std::size_t width) {
    if (s.size() == width) return s;
    if (s.size() < width)  return s + std::string(width - s.size(), ' ');
    if (width <= 1)        return std::string(width, '.');
    return s.substr(0, width - 1) + "\xE2\x80\xA6"; // ellipsis U+2026
}

/// Map RSSI (dBm) to bar count 0..5. Boundaries match common Wi-Fi/BT signal-strength heuristics.
int rssi_to_bars(int rssi) {
    if (rssi >= -50) return 5;
    if (rssi >= -60) return 4;
    if (rssi >= -70) return 3;
    if (rssi >= -80) return 2;
    if (rssi >= -90) return 1;
    return 0;
}

/// Returns the ANSI color for a given bar count: green (strong), yellow (medium), red (weak).
const char* bar_color(int bars) {
    if (bars >= 4) return kFgGreen;
    if (bars == 3) return kFgYellow;
    return kFgRed;
}

/// Render "<rssi>  ▰▰▰▱▱" with color on the bars. Display width is always 10.
std::string format_signal(int rssi) {
    int bars = rssi_to_bars(rssi);
    std::ostringstream os;
    os << std::setw(4) << rssi << "  " << bar_color(bars);
    for (int i = 0; i < kBarCount; ++i) os << (i < bars ? kBarFilled : kBarEmpty);
    os << kReset;
    return os.str();
}

/// Format an age duration. Uses ms for very recent packets so the column doesn't
/// stick at "0s" — BLE devices typically advertise every 100-1000 ms.
std::string format_age(std::chrono::steady_clock::duration age) {
    using namespace std::chrono;
    auto ms = duration_cast<milliseconds>(age).count();
    if (ms < 0) ms = 0;
    std::ostringstream os;
    if      (ms < 1000)     os << ms << "ms";
    else if (ms < 60'000)   os << (ms / 1000) << "s";
    else if (ms < 3'600'000) os << (ms / 60'000) << "m";
    else                    os << (ms / 3'600'000) << "h";
    return os.str();
}

} // anonymous namespace

DeviceEntry summarize(const ScanResultMessage& msg) {
    DeviceEntry e;
    e.mac = address_to_mac_address(msg.mac_address());
    e.rssi = msg.rssi();
    e.last_seen = std::chrono::steady_clock::now();
    e.manufacturer = "-";
    e.type = "Unknown";

    auto structures = parse_ad_structures(msg.adv_data());

    // Prefer manufacturer-specific data (0xFF) for type + manufacturer.
    for (const auto& ad : structures) {
        if (ad.type == 0xFF) {
            if (ad.data.size() >= 2) {
                uint16_t cid = static_cast<uint16_t>(ad.data[0]) |
                               (static_cast<uint16_t>(ad.data[1]) << 8);
                e.manufacturer = company_name_from_id(cid);
            }
            auto parts = proprietary::decode_proprietary_parts(ad.data);
            if (parts) {
                e.type = parts->description;
                e.last_decoded = parts->details;
            }
            break;
        }
    }

    if (e.last_decoded.empty()) {
        auto ln = decode_local_name(structures);
        if (ln) {
            if (e.type == "Unknown") e.type = "Local Name";
            e.last_decoded = "name=\"" + *ln + "\"";
        }
    }

    if (e.last_decoded.empty()) {
        for (const auto& ad : structures) {
            auto d = decode_ad_data(ad);
            if (d) {
                if (e.type == "Unknown") e.type = ad_type_name(ad.type);
                e.last_decoded = *d;
                break;
            }
        }
    }

    if (e.last_decoded.empty()) {
        std::ostringstream os;
        os << std::hex << std::setfill('0');
        std::size_t n = std::min<std::size_t>(8, msg.adv_data().size());
        for (std::size_t i = 0; i < n; ++i) os << std::setw(2) << static_cast<int>(msg.adv_data()[i]);
        if (msg.adv_data().size() > n) os << "...";
        e.last_decoded = os.str();
    }

    return e;
}

DeviceTracker::DeviceTracker(std::size_t capacity) : m_capacity(capacity) {}

void DeviceTracker::update(const ScanResultMessage& msg) {
    auto entry = summarize(msg);
    auto it = m_index.find(entry.mac);
    if (it != m_index.end()) {
        *(it->second) = entry;
        m_entries.splice(m_entries.begin(), m_entries, it->second);
        return;
    }
    if (m_entries.size() >= m_capacity) {
        m_index.erase(m_entries.back().mac);
        m_entries.pop_back();
    }
    m_entries.push_front(std::move(entry));
    m_index[m_entries.front().mac] = m_entries.begin();
}

std::vector<DeviceEntry> DeviceTracker::snapshot() const {
    std::vector<DeviceEntry> out(m_entries.begin(), m_entries.end());
    std::sort(out.begin(), out.end(), [](const DeviceEntry& a, const DeviceEntry& b) {
        if (a.rssi != b.rssi) return a.rssi > b.rssi;
        return a.last_seen > b.last_seen;
    });
    return out;
}

StatRenderer::StatRenderer() : m_started(std::chrono::steady_clock::now()) {
    std::cout << "\033[?25l\033[2J\033[H" << std::flush; // hide cursor + clear
}

StatRenderer::~StatRenderer() {
    std::cout << "\033[?25h" << kReset << '\n' << std::flush;
}

int StatRenderer::terminal_width() {
    struct winsize ws{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) return ws.ws_col;
    return 100;
}

void StatRenderer::render(const DeviceTracker& tracker) {
    auto now = std::chrono::steady_clock::now();
    if (now - m_last_render < kRenderInterval) return;
    m_last_render = now;

    int term_w = terminal_width();
    if (term_w < 90) term_w = 90;

    // Fixed columns:  # (3) | MAC (17) | <flex Type> | Signal (10) | <flex Mfr> | <flex Last> | Age (5)
    // Separators: " | " (3) × 6 = 18.  Padding: leading "  " + trailing "  " = 4.
    constexpr int kIdxW    = 3;
    constexpr int kMacW    = 17;
    constexpr int kSignalW = 10;
    constexpr int kAgeW    = 5;
    const int fixed = 4 + 18 + kIdxW + kMacW + kSignalW + kAgeW;

    int flex = term_w - fixed;
    if (flex < 30) flex = 30;
    int w_type = std::max(10, flex / 4);
    int w_mfr  = std::max(12, flex / 4);
    int w_last = flex - w_type - w_mfr;
    if (w_last < 12) w_last = 12;

    std::ostringstream out;
    out << "\033[H"; // home

    // Title bar
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - m_started).count();
    std::ostringstream title;
    title << "BLE Sniffer - Live Stats   "
          << tracker.size() << "/" << tracker.capacity() << " devices  |  uptime "
          << std::setw(2) << std::setfill('0') << (uptime / 3600) << ':'
          << std::setw(2) << std::setfill('0') << ((uptime / 60) % 60) << ':'
          << std::setw(2) << std::setfill('0') << (uptime % 60);
    out << kCyan << clip(title.str(), term_w) << kReset << "\033[K\n";

    // Header row
    out << kBold;
    out << "  " << clip("#", kIdxW)
        << " | " << clip("MAC", kMacW)
        << " | " << clip("Type", w_type)
        << " | " << clip("Signal", kSignalW)
        << " | " << clip("Manufacturer", w_mfr)
        << " | " << clip("Last Decoded", w_last)
        << " | " << clip("Age", kAgeW)
        << "  ";
    out << kReset << "\033[K\n";

    // Separator
    out << std::string(static_cast<std::size_t>(term_w), '-') << "\033[K\n";

    // Rows
    auto rows = tracker.snapshot();
    std::size_t idx = 1;
    for (const auto& d : rows) {
        std::ostringstream idx_s; idx_s << idx++;

        out << "  " << clip(idx_s.str(), kIdxW)
            << " | " << clip(d.mac, kMacW)
            << " | " << clip(d.type, static_cast<std::size_t>(w_type))
            << " | " << format_signal(d.rssi)                  // 10 display columns + ANSI codes
            << " | " << clip(d.manufacturer, static_cast<std::size_t>(w_mfr))
            << " | " << clip(d.last_decoded, static_cast<std::size_t>(w_last))
            << " | " << clip(format_age(now - d.last_seen), kAgeW)
            << "  \033[K\n";
    }

    // Wipe stale rows below.
    out << "\033[J";
    out << kDim << clip("press Ctrl+C to quit  -  sorted by RSSI then recency", term_w)
        << kReset << "\033[K";

    std::cout << out.str() << std::flush;
}

} // namespace ble_sniffer::stat
