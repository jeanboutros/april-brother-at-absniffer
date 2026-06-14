# A0: Task Definition — psc-0029

## Task
Silent failure: init() never called and no-flag exit gives no feedback

## Domain Classification
- Domain: software (CLI bug fix)
- Domain signals: [security] (silent failure could mask security issues)
- Roster: SW, TX, DX
- Total: 3 specialists

## Specialist Roster
- SW: Software Engineer — architecture review, code fix review
- TX: Test Engineer — test strategy, acceptance criteria verification
- DX: Docs Writer — documentation review (bluetooth_at_driver.h Doxygen fix)

## Acceptance Criteria
1. init() is called with error handling; failure → stderr message + non-zero exit
2. Missing action flag → stderr message + non-zero exit
3. No silent exit with return code 0 when driver is unusable or no action requested
4. All existing tests pass
5. New test cases for missing flag and init failure
6. Code changes reviewed through full pipeline (A→B→C→C4)

## Files
- src/main.cpp — primary fix (already applied, needs review)
- include/ble_sniffer/bluetooth_at_driver.h — documentation fix (F1)
- include/ble_sniffer/SerialPort.h — init() interface review (F3, advisory)

## Constraints
- close_connection() must return void (ADR psc-adr-0005)
- AtBaudParam (not BaudRate) for AT protocol codes in ble_sniffer namespace
- Conventional Commits v1.0.0 for all git operations
- Test infrastructure (Catch2, MockSerialPort) is not yet available — AC5 may be deferred