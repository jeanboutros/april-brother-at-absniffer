# ADR: BluetoothATDriver Constructor Precondition

| Field | Value |
|-------|-------|
| ID | psc-adr-0029-2 |
| Status | Accepted |
| Date | 2026-06-14 |
| Ticket | psc-0029 |
| Decision | Add a precondition check in BluetoothATDriver constructor: throw `std::invalid_argument` if the port's `is_open()` returns false. |

## Context

`BluetoothATDriver`'s constructor validates that the `unique_ptr<SerialPort>` is not null (bluetooth_at_driver.cpp:14-17) but does NOT validate that `is_open()` returns true. This allows constructing a driver with an un-opened port, which is the exact root cause of psc-0029 — the driver silently no-ops on all operations when the port isn't open, producing no error feedback. The original bug was that `init()` was never called on the serial port before passing it to the driver, and the driver had no way to detect or report this misuse.

## Decision

Add a precondition check in the `BluetoothATDriver` constructor: if the port's `is_open()` returns false, throw `std::invalid_argument`. Update Doxygen to document the precondition.

## Options Considered

### Option A: Throw `std::invalid_argument` if `!port->is_open()`
Provides fail-fast behavior — the original bug (passing un-opened port) would immediately throw rather than silently degrading. The exception type `std::invalid_argument` clearly communicates programmer error (precondition violation), not runtime failure. This matches the existing pattern where the constructor already throws for null port.

### Option B: Document precondition only, no runtime check
Relies on documentation alone to prevent misuse. This approach failed for psc-0029 — the precondition was never documented, and the resulting silent failure was extremely difficult to diagnose. Documentation without enforcement provides no protection against repeated misuse.

### Option C: Call `port->init()` inside the constructor
Eliminates two-phase initialization entirely by having the driver own the port's lifecycle. However, this breaks the dependency injection pattern — the driver should not own the port's initialization lifecycle because callers may need to configure the port differently before initialization (baud rates, timeouts, etc.).

## Rationale

Option A provides fail-fast behavior that directly prevents the psc-0029 bug class. The original bug was that passing an un-opened port caused silent degradation — the constructor precondition check makes this impossible. Option B relies on documentation alone, which failed for psc-0029. Option C breaks the dependency injection pattern — the driver depends on the SerialPort interface, not on how it's initialized. Different callers may need different configuration (baud rates, timeouts) before initialization, and the driver should not own that configuration.

The `std::invalid_argument` exception type is chosen deliberately: this is a programmer error (precondition violation), not a runtime error. It should never be caught in production code — it indicates a bug in the caller.

## Consequences

**Positive:**
- The psc-0029 bug class cannot recur — constructing a driver with an un-opened port immediately throws.
- The precondition is enforced at runtime, not just documented.
- The exception type (`std::invalid_argument`) clearly communicates programmer error.
- Future callers cannot accidentally repeat the psc-0029 bug.

**Negative:**
- Callers who construct a `BluetoothATDriver` must ensure `init()` has been called on the port first. This is already the case in main.cpp after the psc-0029 fix.
- The constructor now has a side effect (throwing) beyond just storing the port pointer. This is acceptable because the existing constructor already throws for null port.
- Test code constructing `BluetoothATDriver` must ensure the mock port returns `true` from `is_open()`.

## Related
- psc-0029 (Silent failure: init() never called)
- psc-adr-0029-3 (SerialPort initialization pattern: caller-managed vs driver-managed)
