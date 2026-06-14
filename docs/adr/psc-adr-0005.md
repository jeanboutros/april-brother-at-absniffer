# ADR: close_connection() returns void for teardown semantics

**Status:** Accepted
**Date:** 2026-06-14
**Decision:** close_connection() returns void, not bool, because teardown operations are best-effort and cannot meaningfully propagate errors.

## Context
The SW Engineer review (A1-SW) recommended changing `close_connection()` from `void` to `bool` for interface consistency with `init()` and `set_baud_rate()`. This conflicted with a prior intentional decision (R6/R12) to change from `int` to `void`.

## Considered Alternatives
| Option | Pros | Cons |
|--------|------|------|
| `bool` return | Interface consistency with `init()`/`set_baud_rate()` | Destructor must discard return value; misleading — caller can't act on close failure; contradicts teardown semantics |
| `void` return | Matches teardown pattern; best-effort contract is explicit; destructor doesn't need to discard; `is_open()` provides query | Interface appears inconsistent with setup methods |

## Decision
`close_connection()` returns `void`. Setup methods (`init`, `set_baud_rate`) return `bool` because failure is actionable. Teardown methods (`close_connection`) return `void` because failure is best-effort. The `is_open()` query method provides verification if needed.

## Consequences
- Interface appears inconsistent but is semantically correct
- Callers who need to verify close can check `is_open()`
- Destructor call sites don't need to discard return values
- Mock implementations know close is best-effort (no error to simulate)