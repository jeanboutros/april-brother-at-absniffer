# A2: Dual-Model Challenge — psc-0029

## Ticket
psc-0029 — Silent failure: init() never called and no-flag exit gives no feedback

## Primary Position
1. init() called by caller (main.cpp) is architecturally correct — dependency injection principle
2. Doxygen at line 50 is factually wrong — must fix
3. Dual error path (init() returns bool but always throws) is confusing but not blocking — flag for ADR
4. The fix in main.cpp (lines 102-119) is correct and complete

## Challenger Position
1. Two-phase init is an anti-pattern — the original bug proves someone forgot to call init()
2. Driver should call init() or validate is_open() in constructor
3. The bool return type on init() is actively misleading — it never returns false
4. Constructor doesn't validate is_open() — this is the EXACT root cause class of psc-0029
5. The !port->init() check at main.cpp:112 is dead code for current implementation

## Resolved Decisions

| # | Decision | Winner | Rationale |
|---|----------|--------|-----------|
| D1 | init() called by caller (main.cpp) | Primary (with caveat) | Current fix is correct for CLI scope. Two-phase init anti-pattern should be addressed in future ADR. |
| D2 | Doxygen "Opens on construction" is incorrect | Primary | No reasonable interpretation supports this wording. Must fix. |
| D3 | Dual error path is acceptable as-is | Challenger | Interface contract (bool return) is violated by implementation (always throws). Elevated from advisory to documented contract inconsistency. ADR needed. |
| D4 | Constructor should validate is_open() | Challenger | Elevated from advisory (75) to blocking (90). Passing un-opened port is the exact root cause of psc-0029. |
| D5 | !port->init() check is dead code | Acknowledged | True for current implementation but correct per interface contract. Document as intentional defensive programming. |

## Updated Findings

| ID | Confidence | Severity | Change | Description |
|----|-----------|----------|--------|-------------|
| F1 | 95 | Critical | Unchanged | Doxygen bug in bluetooth_at_driver.h:50 |
| F2 | 60 | Low | Unchanged | Destructor message always prints |
| F3 | 85 | High | Elevated from 80 | SerialPort::init() contract violation — interface says bool but impl always throws |
| F4 | 90 | Critical | Elevated from 75 to 90 | Constructor doesn't validate is_open() — exact root cause of psc-0029 |
| M1 | 70 | Advisory | New | No validation that device is responsive after init |
| M2 | 55 | Advisory | New | --stat without -s produces no error |
| E1 | 60 | Documentation | New | !port->init() check is dead code but intentional |
| E2 | 50 | Advisory | New | ABSnifferSerialPort constructor can throw std::invalid_argument (uncaught) |

## ADR Decisions Needed

| ADR ID | Decision | Priority |
|--------|----------|----------|
| psc-adr-0029-1 | SerialPort::init() error reporting contract (bool return vs throw vs void) | High |
| psc-adr-0029-2 | BluetoothATDriver constructor precondition (validate is_open()) | High |
| psc-adr-0029-3 | Two-phase init vs constructor init for SerialPort | Medium |

## Verdict
CONDITIONAL PASS — Fix in main.cpp is correct. Blocking findings: F1 (Doxygen), F4 (constructor precondition). ADR needed for F3 (init() contract).