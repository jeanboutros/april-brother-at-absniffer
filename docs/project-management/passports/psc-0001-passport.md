# Pipeline Passport: psc-0001

## Task Identity

| Field | Value |
|-------|-------|
| Ticket | psc-0001 |
| Title | Full Codebase Review — ABSniffer 528 BLE AT Driver Library + CLI |
| Created | 2026-06-13 |
| PM | pm |

## Required Steps

Every step the pipeline requires for this task. Steps are checked off sequentially. No step may be skipped without a written justification in the Skipped Steps section below.

### Phase A — Requirements & Design

- [ ] A0: Task Definition — acceptance criteria, files, constraints, test strategy, doc plan. **Domain classification:** hardware + wireless + security | **Roster:** SW, TX, DX, HW, WX, SX (6 specialists)
- [ ] A1: Specialist Review — all dispatched specialists review independently
  - [ ] A1-SW: Software Engineer
  - [ ] A1-TX: Test Engineer
  - [ ] A1-DX: Docs Writer
  - [ ] A1-HW: Hardware Engineer (hardware in scope: serial port, termios, device I/O)
  - [ ] A1-WX: Wireless Expert (wireless in scope: BLE advertising, AT commands, AD parsing)
  - [ ] A1-SX: Security Reviewer (security in scope: serial I/O, external input parsing, proprietary AD decode)
- [ ] A2: Dual-Model Challenge — primary pass + challenger pass
- [ ] A2a: ADR Creation — ADR file for every resolved design decision
- [ ] A3: A-GATE — T3 ✅/❌ | T-ARCH ✅/❌ | ADRs present ✅/❌ | Verdict: _______

### Phase B — Build (PAU Loop)

- [ ] B1: PLAN — identify files, acceptance criteria, logical units
- [ ] B2-1: APPLY (unit 1) — implement, run build
- [ ] B2a-1: B-UNIT-GATE — T1 ✅/❌ | T-ARCH ✅/❌ | Verdict: _______
- [ ] B2-N: APPLY (unit N) — implement, run build
- [ ] B2a-N: B-UNIT-GATE — T1 ✅/❌ | T-ARCH ✅/❌ | Verdict: _______
- [ ] B3: VALIDATE — full build, optional flash
- [ ] B3a: B-FINAL-GATE — T1 ✅/❌ | T2 ✅/❌ | T-ARCH ✅/❌ | Verdict: _______

### Phase C — Multi-Agent Verify

- [ ] C0: T1 Re-run — all T1 checks pass
- [ ] C1: Dual-Model Challenge (Verification) — primary + challenger
- [ ] C2: Specialist Approval — all dispatched specialists
- [ ] C3: C-GATE — T1 ✅/❌ | T3 ✅/❌ | T-ARCH ✅/❌ | Verdict: _______

### Commit

- [ ] COMMIT — all gates passed, all approvals issued

## Step Log

Every step execution is logged here with timestamp, agent, and result.

| Step | Agent | Timestamp | Result | Notes |
|------|-------|-----------|--------|-------|
| A0 | _______ | _______ | _______ | _______ |
| A1-SW | software-engineer | _______ | APPROVED/CONDITIONAL PASS/REJECTED | _______ |
| A1-TX | test-engineer | _______ | APPROVED/CONDITIONAL PASS/REJECTED | _______ |
| A1-DX | docs-writer | _______ | APPROVED/CONDITIONAL PASS/REJECTED | _______ |
| A1-HW | hardware-engineer | _______ | APPROVED/CONDITIONAL PASS/REJECTED | _______ |
| A1-WX | wireless-expert | _______ | APPROVED/CONDITIONAL PASS/REJECTED | _______ |
| A1-SX | security-reviewer | _______ | APPROVED/CONDITIONAL PASS/REJECTED | _______ |
| A2-Primary | _______ | _______ | _______ | _______ |
| A2-Challenger | _______ | _______ | _______ | _______ |
| A2a-ADR | _______ | _______ | _______ | _______ |
| A3-Gate | supreme-leader | _______ | PASS/FAIL | T3: __ T-ARCH: __ ADRs: __ |
| ... | ... | ... | ... | ... |

## Gate Results

| Gate | Tier | Attempt | Result | Retry Budget | Notes |
|------|------|---------|--------|---------------|-------|
| A-GATE | T3 | 1 | _______ | 0/3 | _______ |
| A-GATE | T-ARCH | 1 | _______ | 0/3 | _______ |
| B-UNIT-GATE-1 | T1 | 1 | _______ | 0/3 | _______ |
| B-UNIT-GATE-1 | T-ARCH | 1 | _______ | 0/3 | _______ |
| ... | ... | ... | ... | ... | ... |

## Skipped Steps

Any step that was skipped MUST have a written justification here. If this section is empty, no steps were skipped.

| Step | Justification | Authorised By |
|------|--------------|---------------|
| _______ | _______ | _______ |

## Loop History

Tracks all pipeline loops (A→B→A→B, B-unit retries, gate failures).

| Loop | From Step | To Step | Reason | Timestamp |
|------|-----------|---------|--------|-----------|
| _______ | _______ | _______ | _______ | _______ |

## Correction Records

Produced by the `post-rejection-correction` skill. One record per retry. Required for every gate failure before the retry is dispatched. Permanent — must not be edited after stamping.

| Retry | Gate | Tier | RC Category | Root cause (why missed) | Corrective action | Codified where |
|-------|------|------|-------------|------------------------|-------------------|----------------|
| _______ | _______ | _______ | _______ | _______ | _______ | _______ |