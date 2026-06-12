#pragma once

#include <ble_sniffer/messages.h>

#include <chrono>
#include <cstddef>
#include <list>
#include <string>
#include <unordered_map>
#include <vector>

namespace ble_sniffer::stat {

/// One row in the live device dashboard.
struct DeviceEntry {
    std::string mac;          ///< "AA:BB:CC:DD:EE:FF"
    std::string type;         ///< e.g. "Find My", "Swift Pair", "Local Name"
    int rssi = 0;
    std::string manufacturer; ///< company name or "-"
    std::string last_decoded; ///< e.g. "status=0x12 key=..."
    std::chrono::steady_clock::time_point last_seen;
};

/// Build a DeviceEntry from a scan result, picking the best-available type/decoded summary.
DeviceEntry summarize(const ScanResultMessage& msg);

/// LRU tracker bounded by a fixed capacity. Most-recently-seen device is kept at front; oldest is evicted.
class DeviceTracker {
public:
    explicit DeviceTracker(std::size_t capacity);

    void update(const ScanResultMessage& msg);

    /// Snapshot sorted by (rssi DESC, last_seen DESC).
    std::vector<DeviceEntry> snapshot() const;

    std::size_t size() const { return m_index.size(); }
    std::size_t capacity() const { return m_capacity; }

private:
    std::size_t m_capacity;
    std::list<DeviceEntry> m_entries;
    std::unordered_map<std::string, std::list<DeviceEntry>::iterator> m_index;
};

/// Pure-ANSI dashboard. Hides the cursor on construction, restores it on destruction.
/// Renders are throttled internally to avoid jumpiness even when the tracker is updated frequently.
class StatRenderer {
public:
    StatRenderer();
    ~StatRenderer();

    StatRenderer(const StatRenderer&) = delete;
    StatRenderer& operator=(const StatRenderer&) = delete;

    /// Redraws the dashboard. Throttled — calls inside the refresh window are no-ops.
    void render(const DeviceTracker& tracker);

private:
    static int terminal_width();

    std::chrono::steady_clock::time_point m_started;
    std::chrono::steady_clock::time_point m_last_render{};
};

} // namespace ble_sniffer::stat
