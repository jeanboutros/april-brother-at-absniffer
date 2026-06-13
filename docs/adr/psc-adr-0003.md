# ADR: JSONL schema uses raw hex MAC and "type" discriminator

**Status:** Proposed
**Date:** 2026-06-13
**Decision:** Use raw hex MAC addresses and a `"type"` discriminator field to unify write and read formats.

## Context

The JSONL output format needs to be consistent between live scan output and replay input. Currently the writer emits MAC addresses in colon-separated format (`"12:3B:6A:1A:F0:E6"`) but the parser expects raw hex (`"123B6A1AF0E6"`). Additionally, when `--json` and `--events` both write to stdout, the consumer needs a way to distinguish packet lines from event lines.

## Considered Alternatives

| Option | Pros | Cons |
|--------|------|------|
| Raw hex MAC (`"123B6A1AF0E6"`) + `"type"` discriminator | No format conversion between write and read; replay parsing is straightforward; `jq` filtering is easy | Less human-readable in raw output; consumers must format for display |
| Colon-separated MAC (`"12:3B:6A:1A:F0:E6"`) + `"type"` discriminator | More human-readable | Requires format conversion for replay parsing; mismatch between writer and parser |
| Separate output streams (packets on stdout, events on stderr) | No discriminator needed | Breaks pipe conventions; makes `jq` filtering harder; non-standard |

## Decision

Option 1. Raw hex MAC addresses and `"type"` discriminator. Raw hex is what the device sends and what the parser expects. The `"type"` field enables consumers to filter packets vs events with `jq '.type == "packet"'`. Human-readable MAC formatting is a display concern, not a storage concern.

## Consequences

- Easier: No format conversion between write and read. Replay parsing is straightforward. `jq` filtering is easy.
- Harder: Raw hex MACs are less human-readable in JSONL output. Consumers who want colon format must add their own formatting.
- Blocked: None. This is a schema decision that affects psc-0017 (JSONL output) and psc-0019 (replay input).