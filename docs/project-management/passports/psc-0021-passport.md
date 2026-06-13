# Pipeline Passport: psc-0021

## Task Identity

| Field | Value |
|-------|-------|
| Ticket | psc-0021 |
| Title | Add event stream engine with --events FILE for device event detection |
| Created | 2026-06-13 |
| PM | pm |

## Required Steps

Every step the pipeline requires for this task. Steps are checked off sequentially. No step may be skipped without a written justification in the Skipped Steps section below.

### Phase A — Requirements & Design

- [ ] A0: Task Definition — acceptance criteria, files, constraints, test strategy, doc plan. **Domain classification:** [wireless] [hardware] [security] | **Roster:** SW, TX, DX, HW, WX, SX
- [ ] A1: Specialist Review — all dispatched specialists review independently
  - [ ] A1-SW: Software Engineer
  - [ ] A1-TX: Test Engineer
  - [ ] A1-DX: Docs Writer
  - [ ] A1-HW: Hardware Engineer
  - [ ] A1-WX: Wireless Expert
  - [ ] A1-SX: Security Reviewer
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
- [ ] C4: PM Completion Review — Decision: _______ | Closure type: _______

### Commit

- [ ] COMMIT — C4 decision CLOSE or CLOSE+NEW, all gates passed, all approvals issued

## Post-Completion Decision

After C4, the PM records the final decision here:

| Field | Value |
|-------|-------|
| Decision | _______ |
| Closure type | _______ |
| Rationale | _______ |
| New tickets spawned | _______ |
| Replacement ticket | _______ |
| Delta analysis ticket | _______ |

## Step Log

Every step execution is logged here with timestamp, agent, and result.

| Step | Agent | Timestamp | Result | Notes |
|------|-------|-----------|--------|-------|
| _______ | _______ | _______ |

## Gate Results

| Gate | Tier | Attempt | Result | Retry Budget | Notes |
|------|------|---------|--------|---------------|-------|
| _______ | _______ | _______ | _______ | _______ | _______ |

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