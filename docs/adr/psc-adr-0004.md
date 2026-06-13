# ADR: Shared DeviceRegistry between --stat and event engine

**Status:** Proposed
**Date:** 2026-06-13
**Decision:** Use a single shared DeviceRegistry that holds both device data and signal processing state, following Single Source of Truth.

## Context

Both `--stat` (device statistics display) and `--events` (event detection) need per-MAC state. Without a shared registry, both would maintain duplicate state: last_seen, rssi, payload, etc. This violates DRY and wastes memory (especially with 10K+ devices in busy environments).

## Considered Alternatives

| Option | Pros | Cons |
|--------|------|------|
| Single DeviceRegistry shared by stat and event engine | Single source of truth; halved memory; `--stat` and `--events` always agree | Registry entry grows larger; LRU eviction must evict both together; thread safety consideration if concurrent access added |
| Separate registries for stat and events | Simpler to implement initially; no cross-concern coupling | Duplicate state; wasted memory; stat and events can disagree on device data |
| Registry for device data, separate signal processing state in EventTracker | Clean separation of concerns | Duplicates state for signal processing fields; harder to correlate device data with signal state; still two sources of truth |

## Decision

Option 1. Single DeviceRegistry with shared state. DeviceRegistry stores both device data (last_seen, rssi, payload) and signal processing state (rssi_ema, inside_region, last_event_fired). DeviceEventTracker reads/writes through the registry. StatView reads for display. This follows Single Source of Truth, eliminates duplicate state, and makes DeviceRegistry the one place to query device information.

The signal processing state stored in DeviceRegistry is computed by pure functions from the `signal_processing` namespace (update_ema, check_hysteresis, check_debounce). DeviceEventTracker orchestrates the computation but does not own the state. This keeps the tracker thin and the algorithms pure.

## Consequences

- Easier: Single source of truth. `--stat` and `--events` always agree on device state. Memory usage is halved vs separate registries.
- Harder: DeviceRegistry entry grows larger (device data + signal processing state). LRU eviction must evict both together. Thread safety considerations if stat and event engine access concurrently (currently single-threaded, but future-proofing needed).
- Blocked: None. This is an architectural decision that affects psc-0021 (event engine) and psc-0023 (DeviceRegistry extraction).