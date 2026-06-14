# ADR: SerialPort::init() Error Reporting Contract

| Field | Value |
|-------|-------|
| ID | psc-adr-0029-1 |
| Status | Accepted |
| Date | 2026-06-14 |
| Ticket | psc-0029 |
| Decision | Keep `bool init()` interface as-is, add `[[nodiscard]]`, and document that ABSnifferSerialPort always throws on failure. |

## Context

The `SerialPort::init()` interface declares `virtual bool init() = 0` (SerialPort.h:197), suggesting `false` is a valid return value for initialization failure. However, `ABSnifferSerialPort::init()` always throws `SerialPortException` on failure and never returns `false`. This creates a contract inconsistency: callers who check the bool return (like `main.cpp:112`) are writing dead code for the default implementation, while callers who only catch exceptions would miss a `false` return from an alternative implementation.

## Decision

For psc-0029, keep the interface as-is (`bool init()`) and document the contract. The `if (!port->init())` check in main.cpp is intentionally defensive programming against alternative implementations. The contract inconsistency will be resolved in a future ticket when MockSerialPort is introduced (psc-0014).

## Options Considered

### Option A: Keep `bool init()`, add `[[nodiscard]]`, document that ABSnifferSerialPort always throws
Preserves the current interface and allows alternative implementations to return `false`. The `[[nodiscard]]` attribute prevents callers from ignoring the return value. Documentation clarifies that the concrete implementation throws, but the interface permits `false`. Minimizes scope for this bugfix ticket.

### Option B: Change to `void init()` that throws exclusively
Makes the contract unambiguous — init either succeeds or throws. However, this is a breaking interface change that requires updating all implementations (ABSnifferSerialPort, and the future MockSerialPort from psc-0014). Too much scope for a bugfix ticket.

### Option C: Change to `std::expected<void, Error>` or result type
Provides the most type-safe error handling, but requires C++23 or an external library dependency. Over-engineered for the current codebase which targets C++17.

## Rationale

Option A minimizes scope creep for this bugfix ticket. The `bool` return type correctly communicates to alternative implementations that they CAN return false. The defensive check in main.cpp is correct per the interface contract even if it's dead code for the current implementation. Option B is the correct long-term fix but requires touching SerialPort.h and all implementations — too much scope for a bugfix. Option C requires a dependency or C++23, which is not justified here.

## Consequences

**Positive:**
- No interface changes needed for psc-0029 — minimal scope.
- `[[nodiscard]]` prevents future callers from silently ignoring init failures.
- Defensive checks in callers are correct per the interface contract.
- Alternative implementations (MockSerialPort) have a clear contract: either return `true` or throw/return `false`.

**Negative:**
- The contract inconsistency remains documented but unresolved — ABSnifferSerialPort::init() never returns `false`, but the interface suggests it can.
- Callers must handle both `false` return AND exceptions, which is redundant for the current implementation.
- When MockSerialPort is added in psc-0014, the contract should be revisited to determine whether `void init()` is the better long-term signature.

## Related
- psc-0029 (Silent failure: init() never called)
- psc-0014 (MockSerialPort introduction — revisit this contract)
- psc-adr-0005 (close_connection() returns void for teardown semantics)
