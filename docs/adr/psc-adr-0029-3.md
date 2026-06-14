# ADR: SerialPort Initialization Pattern: Caller-Managed vs Driver-Managed

| Field | Value |
|-------|-------|
| ID | psc-adr-0029-3 |
| Status | Accepted |
| Date | 2026-06-14 |
| Ticket | psc-0029 |
| Decision | The caller is responsible for calling `init()`. The driver validates the precondition (`is_open() == true`) in its constructor. This follows the Dependency Inversion Principle. |

## Context

The original bug in psc-0029 occurred because `init()` was not called on the serial port before passing it to the driver. This raises a fundamental design question: should the driver call `init()` on the port, or should the caller be responsible? The answer determines where the initialization responsibility lies and affects testability, configurability, and the Single Responsibility Principle.

## Decision

The caller is responsible for calling `init()`. The driver validates the precondition (`is_open() == true`) in its constructor (per psc-adr-0029-2). This follows the Dependency Inversion Principle — the driver depends on the SerialPort interface, not on how it's initialized.

## Options Considered

### Option A: Caller calls `init()`, driver validates `is_open()`
The caller owns the full lifecycle: create port, configure it, call `init()`, then pass the ready-to-use port to the driver. The driver's only responsibility is to verify the precondition. This follows Dependency Injection — the driver receives a fully-configured dependency. Callers can configure baud rates, timeouts, and other settings before initialization. Test mocks can be set up with `is_open()` returning true without needing a real serial port.

### Option B: Driver calls `init()` in constructor
The driver would call `port->init()` during its own construction. This eliminates the possibility of forgetting to call `init()` but gives the driver ownership of the port's initialization lifecycle. This prevents callers from configuring the port before initialization (e.g., setting a custom baud rate via `set_baud_rate()` before `init()`). It also makes testing harder — test code would need to mock `init()` behavior separately from construction.

### Option C: Two-phase: construct driver, then call `driver.init()` (separate from port init)
Introduces a second initialization phase where the driver itself needs to be initialized after construction. This adds another two-phase initialization pattern, which doesn't solve the underlying problem — callers could still forget to call `driver.init()`. It also adds unnecessary complexity: what state is the driver in between construction and `init()`?

## Rationale

Option A keeps the driver's responsibility clear: "give me a working port." The driver should not own the port's lifecycle (opening, configuring) because:

1. **Different callers need different configurations.** A test might use a MockSerialPort that's always "open." A CLI tool might configure baud rate before init. A library user might need platform-specific serial settings.
2. **Dependency Inversion Principle.** The driver depends on the `SerialPort` abstraction, not on how it's initialized. Mixing initialization responsibility into the driver couples it to the concrete initialization protocol.
3. **Single Responsibility Principle.** The driver's job is "communicate with the device via AT commands." Opening the serial port is a separate responsibility.
4. **Testability.** In tests, a MockSerialPort can simply return `true` from `is_open()` without needing a real `init()` implementation.

The precondition enforcement from psc-adr-0029-2 makes Option A safe — callers cannot accidentally pass an un-opened port because the constructor will throw.

## Consequences

**Positive:**
- Clear separation of responsibilities: caller owns port lifecycle, driver owns AT communication.
- Callers can configure the port before initialization (baud rates, timeouts, custom settings).
- Test code is simpler — mocks don't need real `init()` implementations.
- Follows Dependency Inversion and Single Responsibility principles.
- The precondition check (psc-adr-0029-2) makes this pattern safe against misuse.

**Negative:**
- Callers must remember to call `port->init()` before constructing `BluetoothATDriver`. This is a two-step protocol, but the constructor precondition check catches violations immediately.
- The main.cpp pattern is slightly more verbose: create port → call init() → check result → construct driver. This verbosity is intentional and correct.

## Related
- psc-0029 (Silent failure: init() never called)
- psc-adr-0029-2 (BluetoothATDriver constructor precondition)
