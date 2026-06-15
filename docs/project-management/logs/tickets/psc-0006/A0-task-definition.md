# A0: Task Definition — psc-0006

## Domain Classification
- **Domain:** wireless, software
- **Signals detected:** BLE protocol parsing, byte order issues, manufacturer-specific data format
- **Specialist roster:** SW, TX, DX, WX
- **Rationale:** Ticket involves BLE proprietary parser fixes with byte order and data format issues requiring wireless expert review

## Task Files
- src/proprietary_parsers.cpp:189 (Samsung byte order)
- src/proprietary_parsers.cpp:83-86 (AirPods battery labeling)
- include/ble_sniffer/proprietary_parsers.h (Doxygen @note comments)

## Acceptance Criteria (from ticket)
- [ ] Samsung `device_type` uses little-endian byte order
- [ ] AirPods battery output is labeled as "(raw level)" or includes level-to-range mapping
- [ ] Doxygen `@note` on each vendor parser indicating reverse-engineered status
- [ ] Build passes

## Test Strategy
- Unit tests for byte order conversion
- Regression tests for existing parsers
- Cross-validation with reference implementations

## Documentation Plan
- Update Doxygen comments with @note about reverse-engineered status
- Consider expanding documentation about byte order conventions in BLE

## File Analysis
### src/proprietary_parsers.cpp
- Line 189: Samsung `device_type = (static_cast<uint16_t>(mfr_data[1]) << 8) | mfr_data[2];` - likely wrong byte order
- Lines 83-86: AirPods battery output prints raw integers without labeling

### include/ble_sniffer/proprietary_parsers.h
- Need to add `@note` comments for apple and samsung namespaces indicating formats are reverse-engineered

## Code Inspection
The Samsung parser reads `device_type` as big-endian `(mfr_data[1] << 8) | mfr_data[2]`, but BLE typically uses little-endian for multi-byte fields. The AirPods battery nibbles (0-10 range) are printed as raw integers which users may misinterpret as percentages.
