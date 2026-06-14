# A1-SW: Software Engineer Review — psc-0029

## Ticket
psc-0029 — Silent failure: init() never called and no-flag exit gives no feedback

## Verdict
CONDITIONAL PASS

## Findings

| ID | Confidence | Severity | File:Line | Description | Status |
|----|-----------|----------|-----------|-------------|--------|
| F1 | 95 | Critical | bluetooth_at_driver.h:50 | Doxygen says "Opens the serial port on construction" — driver does NOT open the port; it takes ownership of an already-opened port | Must fix |
| F2 | 60 | Low | bluetooth_at_driver.cpp:27 | Destructor unconditionally prints "cleaned up" even on failure paths | Informational |
| F3 | 80 | High | SerialPort.h:197, ab_sniffer_serial_port.cpp:75-123 | init() returns bool but always throws on failure — never returns false. Dual error path is confusing. | Advisory for this ticket; flag for follow-up ADR |
| F4 | 75 | Moderate | bluetooth_at_driver.cpp:14-18 | Constructor checks for null but doesn't check is_open(). Accepts un-opened port silently. | Advisory — document precondition |

## Architecture Assessment
- init() called by caller (main.cpp) is architecturally correct — dependency injection principle
- Action flag validation before resource allocation is correct ordering
- Error messages to stderr with non-zero exit codes is correct
- The applied fix in main.cpp (lines 102-119) is sound

## Test Strategy (combined with TX)
- TC-1: Missing action flag → error + non-zero exit
- TC-2: init() failure → error + non-zero exit
- TC-3: Successful init → driver is usable
- TC-4: init() exception caught and reported
- TC-5: Driver with un-opened port gracefully no-ops (regression test)

**Blocking:** No test infrastructure (Catch2, MockSerialPort) exists. Flagged for separate ticket.

## Self-Reflection
1. Why was the bug missed? — Constructor doesn't enforce is_open() precondition. Silent degradation (Pattern 6).
2. What safeguard would catch it? — TDD (test for un-opened port), state machine documentation for ABSnifferSerialPort.
3. Knowledge base update — Constructor precondition enforcement for RAII types taking unique_ptr ownership.