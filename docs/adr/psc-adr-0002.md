# ADR: Device discovery limited to Linux with --auto-detect and VID:PID 1915:520f

**Status:** Proposed
**Date:** 2026-06-13
**Decision:** Implement Linux-only `--auto-detect` using sysfs to find the ABSniffer 528 by VID:PID 1915:520f.

## Context

The ABSniffer 528 dongle appears as a USB serial device. Users must currently specify the device path manually (e.g., `/dev/ttyUSB0`). On Linux, the USB device has vendor ID 1915 and product ID 520f with product string "nRF52 USB Product". macOS does not expose USB VID:PID through `/dev` in a portable way. Auto-detection on macOS would require IOKit frameworks, which is out of scope.

## Considered Alternatives

| Option | Pros | Cons |
|--------|------|------|
| Linux-only `--auto-detect` via sysfs | Uses stable VID:PID 1915:520f; no new dependencies; handles multi-dongle | macOS/Windows users still need `-d`; Linux-specific `/sys` reading |
| Cross-platform auto-detect via libusb | Works on all platforms | Adds libusb dependency; complexity for rare macOS need; macOS already has descriptive `/dev/cu.usbmodem*` naming |
| No auto-detect — always specify `-d` | Simplest implementation | Least user-friendly; Linux users must find device path manually |

## Decision

Option 1. Linux-only `--auto-detect` using sysfs. The ABSniffer 528 has a unique VID:PID (1915:520f) and product string "nRF52 USB Product" that makes it distinguishable from other USB serial devices. On Linux, `/sys/bus/usb/devices/` exposes idVendor, idProduct, and product strings. We read these to find the device, then resolve the TTY path. macOS is out of scope — users there use `/dev/cu.usbmodem*` which is already descriptive.

## Consequences

- Easier: Linux users can run `bluetooth-at-driver --auto-detect -s` without knowing the device path. Multi-dongle scenarios are handled by matching VID:PID.
- Harder: macOS and Windows users must still specify `-d`. The sysfs approach requires reading `/sys` files, which is Linux-specific.
- Blocked: None. The feature is additive and does not change existing `-d` behavior.