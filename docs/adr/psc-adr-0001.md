# ADR: -i/--input as mode flag for file replay

**Status:** Proposed
**Date:** 2026-06-13
**Decision:** Use `-i` as a mode flag that repurposes `-d` for file replay, making `--info` long-form only.

## Context

The CLI currently uses `-i, --info` for device information. We need a way to replay captured JSONL files for offline analysis. The original proposal was `-i FILE` as a separate argument, but this conflicts with the existing `-i` short form. The user also wants the device argument (`-d`) to be reused for the file path when in replay mode.

## Considered Alternatives

| Option | Pros | Cons |
|--------|------|------|
| `-i, --input` as mode flag repurposing `-d` | Reuses existing `-d` argument; no new positional argument; intuitive mental model (`-i -d capture.jsonl`) | Existing users of `-i` short form for `--info` must switch to `--info`; minor breaking change |
| `-f, --file FILE` as separate argument | No conflict with existing flags; mutually exclusive with `-d` is clear | Adds a new positional argument; two device-source flags to document |
| `--replay FILE` as standalone argument | Simple; no `-d` reuse confusion | New positional argument; doesn't leverage existing `-d` convention |

## Decision

Option 1. `-i` becomes a mode flag. When `-i` is active, `-d` means file path instead of serial device. `--info` becomes long-form only (loses `-i` short form). This is cleaner because it reuses the existing `-d` argument, avoids adding a new positional argument, and makes the CLI more intuitive (`-i -d capture.jsonl`).

## Consequences

- Easier: No new positional argument. `-d` works in both modes. Intuitive mental model.
- Harder: Existing users of `-i` (short for `--info`) must use `--info` instead. This is a minor breaking change.
- Blocked: None.