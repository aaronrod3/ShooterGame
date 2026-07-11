# 09 — Two-Month Schedule

## Dependencies

- This file sequences and consolidates the week-by-week task sections already defined in every other file (01–08). It introduces no new classes or systems — it is the scheduling layer that ties them together. Read it alongside [MASTER_BRIEF.md](MASTER_BRIEF.md)'s dependency diagram (Section 6) for the underlying *why* of this ordering.

---

## 0. Calendar

| Week | Dates (2026) |
|---|---|
| 1 | Mon Jul 13 – Sun Jul 19 |
| 2 | Mon Jul 20 – Sun Jul 26 |
| 3 | Mon Jul 27 – Sun Aug 2 |
| 4 | Mon Aug 3 – Sun Aug 9 |
| 5 | Mon Aug 10 – Sun Aug 16 |
| 6 | Mon Aug 17 – Sun Aug 23 |
| 7 | Mon Aug 24 – Sun Aug 30 |
| 8 | Mon Aug 31 – Sun Sep 6 |
| Buffer | Mon Sep 7 – Wed Sep 9 (deadline) |

Solo-dev plan: each week is treated as ~5 effective engineering days (weekends available as informal overflow/buffer, not scheduled as required work).

---

## 1. Week-by-Week Build Plan

### Week 1 (Jul 13 – Jul 19) — Foundation

**Systems being built:** Mission phase state machine, `AShooterGameState`, `AZoneDefinition` (state/config only, no spawning), minimum viable map greybox. Light parallel start on File 06 (data structures only) and File 08 (session-lifecycle audit, shared timer utility).

**Why first:** every other system in this plan reads or writes `AShooterGameState` and/or `AZoneDefinition`. Nothing else can be meaningfully built or tested until these exist. File 01 Section 1.2 confirms `AShooterGameState` does not currently exist in `Source/` — this is the single highest-priority gap in the whole plan.

**Dependencies:** none upstream.

**Exit condition:** `AShooterGameState` compiles and replicates a test value correctly between host and one PIE client; `AZoneDefinition` placed in the greybox map with a manually-triggerable state change visible on both clients.

---

### Week 2 (Jul 20 – Jul 26) — Phase 1 Loop

**Systems being built:** `AWaveSpawnManager`, Zone 1 clearance threshold (`EClearanceMode::EnemyCount`), spawn-point tagging/density anchoring, pursuit-radius check code (built now, not yet wired to a live trigger).

**Dependencies:** Week 1's `AZoneDefinition` and `AShooterGameState`.

**✅ Checkpoint A — end of Week 2: Phase 1 loop playable solo.** A single player can insert into Zone 1, fight through an authored defended-position wave set, reach the clearance threshold, and see the mission phase advance to `Resupply1` (even though nothing happens *in* `Resupply1` yet — File 05 hasn't built the window system). This is the first point in the plan where "press play, do a real gameplay loop" is possible, and validates the File 01/02 split (state vs. spawn execution) actually works end-to-end before three more files build on top of it.

---

### Week 3 (Jul 27 – Aug 2) — Objective and Doomed-Zone Core

**Systems being built:** `AObjectiveTerminal` + `UObjectiveTaskComponent` (Phase 2 terminal), `EClearanceMode::AreaControl` (Zone 2), hot-zone escalation curve consumption, doomed-state continuous-spawn switch, overrun-threshold trigger path, human AI first pass (cover + suppress, flanking cut-first if tight).

**Dependencies:** Week 2's wave spawn manager (for pressure-response wiring and doomed-state spawn-loop switch); Week 1's zone state model.

**Exit condition:** the Phase 2 terminal is interactable once Zone 1 clears, progress ticks while a player stands on it, and failing to complete it within the overrun window correctly forces the zone doomed instead of crashing or silently continuing.

---

### Week 4 (Aug 3 – Aug 9) — Exfil Core

**Systems being built:** `AFieldExtractionPoint` + `UExtractionRewardLibrary`, pursuit-radius wiring to `Phase3Exfil → Resupply2`, Zone 2 clearance + task-completion wiring to `Phase4SecondObjective → Phase4Exfil`, night-variant zombies, enemy-attraction perception wiring, Zone 2 wave content authoring.

**Dependencies:** Week 3's objective task completion (drives the Phase 3 transition); Week 2's pursuit-radius check code (now wired live).

**✅ Checkpoint B — end of Week 4: Phases 1–3 playable, exfil works.** A player can run the full Zone 1 arc — infiltrate, clear, complete the Phase 2 task under increasing pressure, watch the zone go doomed, physically run the pursuit radius, and either reach the transit corridor (mission continues, un-gated since Resupply2's window system doesn't exist yet) or bail early via a field extraction point and receive a phase-correct reward tier. This is the first checkpoint that exercises a *complete* high-risk system (doomed-zone timing, Risk #1) rather than an isolated piece of one.

---

### Week 5 (Aug 10 – Aug 16) — Economy Core

**Systems being built:** `PersonalPoints`/`SquadSharedFund` + `UScoringSubsystem` + `UScoreValuesDataAsset`, resupply-window-as-phase model (the "unspent points are lost" behavior and phase-gated purchase guard), `UW_ResupplyOverlay` first pass, `UCallInDefinition` + `ACallInManager` skeleton, ammo + medical supply crates.

**Dependencies:** Week 1's phase state machine (resupply windows are phases, not a bolt-on); Week 4's extraction reward calculation (hands off to `UScoringSubsystem` here).

**Exit condition:** a solo player can earn points across a Zone 1 run, enter `Resupply1`, see a live countdown, purchase an ammo crate, and watch it arrive on a delay rather than instantly — and watch unspent points become inert the moment the window phase ends.

---

### Week 6 (Aug 17 – Aug 23) — Full Arc Closure

**Systems being built:** Kit preset storage/persistence, `UW_LoadoutScreen`, `UW_PresetSelector` + mission-start apply flow, `AExfilVehicle` + `UW_ExfilCountdown`, secondary emergency extraction, left-behind detection/resolution, Phase 4 terminal placement (content reuse of Week 3's engineering), kit-swap economy gating.

**Dependencies:** Week 5's economy/resupply-window system (kit swap and Phase 4 exfil both gate on resupply-phase logic and `SquadSharedFund`); Week 3's task component (Phase 4 terminal reuses it directly).

**✅ Checkpoint C — end of Week 6: full 4-phase arc playable.** A player selects a kit preset at mission start, runs the complete `PreMission → Phase1Infiltration → Resupply1 → Phase2Objective → Phase3Exfil → Resupply2 → Phase4SecondObjective → Phase4Exfil → PostMission` chain in one session, and experiences the exfil vehicle's fixed arrival/departure with both the "aboard" and (in a second test run) "left behind → emergency extraction" outcomes. This is the point at which every phase transition in File 01 Section 1.4 has been exercised at least once.

---

### Week 7 (Aug 24 – Aug 30) — High-Tier Economy and Squad Member

**Systems being built:** squad vote/host approval (including solo-session handling), fire-support strikes (mortar, artillery, chopper) + distraction flare, `UW_ResupplyKitSwap` + squad-visibility summary panel, and — the week's centerpiece — the entire Temporary Squad Member system (File 07), scoped and risk-managed per that file's Section 7 cut-order.

**Dependencies:** Week 5's `ACallInManager` (fire support and squad member are both call-in types); Week 6's kit preset system (squad member outfitting reuses `ApplyKitPresetToCharacter` directly); Week 2's human AI cover/suppress BT nodes (squad member reuses them).

**Why squad member is here, not earlier:** every one of its dependencies (call-in economy, kit presets, human-AI-adjacent BT nodes) must already exist and be stable, or its 13.5 dev-day estimate (File 07 Section 11) balloons trying to build those prerequisites inline. This is the direct execution of GameplayPlan.md's explicit instruction: "Temporary squad member... Implement last."

**Exit condition:** at minimum, `HoldPosition` and `Follow` commands work end-to-end (File 07 Section 7's absolute floor) with correct outfitting, one-per-team enforcement, and death/gear-loss handling — ideally all 6 commands, per that file's stretch target.

---

### Week 8 (Aug 31 – Sep 6) — Integration Week

**No new features.** This week is reserved entirely for:

1. Full-loop multiplayer testing — minimum 2 clients on physically separate networks (File 08 Section 6/8), not just multi-client PIE.
2. Bug fixing surfaced by that testing.
3. The full acceptance-test pass defined in [10_Demo_Definition_and_Acceptance.md](10_Demo_Definition_and_Acceptance.md), run system-by-system.
4. Tuning pass on every numeric knob flagged across Files 01–07 as data-driven-for-this-reason (escalation curves, window durations, drop intervals, point values, deployment duration) — informed by cumulative playtesting from Weeks 2–7, not guessed fresh this week.
5. `AlivePlayers`/`HandleMatchOver`/`EPlayerMissionStatus` interaction verification (File 04 Section 6.3, File 08 Section 5.1) if not already fully closed out in Week 6.

**✅ Checkpoint D — end of Week 8: demo-complete**, per the Master Brief's Definition of Done (Section 7 of that file).

---

### Buffer (Sep 7 – Sep 9)

Reserved exclusively for: (a) any Definition of Done item still failing after Week 8's acceptance pass, triaged by the priority order below, and (b) one final full-session playtest recording/walkthrough to confirm demo-readiness. No new feature work is scheduled here under any circumstance — if a Week 7/8 feature is incomplete, File 07 Section 7's scope-reduction guidance is applied instead of spending buffer time trying to finish it.

**Triage priority if the buffer is tight** (protect top items first): (1) the core phase-transition loop and session stability (Master Brief DoD items 1–3, 10) — a demo that can't reliably complete a session is not a demo; (2) economy + resupply + extraction (DoD items 4, 6, 7) — these are the pillars most directly tied to "planning pays off" and "graceful failure path"; (3) kit presets (DoD item 5); (4) temporary squad member (DoD item 8) — explicitly last, consistent with its "implement last" design-doc instruction and File 07's own risk framing.

---

## 2. Dependency Graph Summary

```
Week 1 (Foundation)
   └─> Week 2 (Phase 1 loop) ──────────────┐
          └─> Week 3 (Objectives+Doomed)    │
                 └─> Week 4 (Exfil core) ───┼─> Checkpoint B
                        └─> Week 5 (Economy)│
                               └─> Week 6 (Presets+Exfil vehicle) ─> Checkpoint C
                                      └─> Week 7 (High-tier econ + Squad Member)
                                             └─> Week 8 (Integration) ─> Checkpoint D
                                                    └─> Buffer
```

File 08 (Networking) is not a node in this chain — it is a horizontal constraint applied at every node (see its own Section 8 "ongoing" tasks). File 09 (this file) is the chain itself, not a node in it.

---

## 3. Milestone Checkpoints Summary

| Checkpoint | End of Week | What's playable |
|---|---|---|
| A | 2 | Phase 1 infiltration → clearance → phase advance, solo |
| B | 4 | Full Zone 1 arc (Phases 1–3) including doomed-zone pursuit and early extraction with correct reward tiers |
| C | 6 | Complete 4-phase arc, kit preset selection, exfil vehicle with both aboard/left-behind outcomes |
| D | 8 | Demo-complete per Master Brief Definition of Done, acceptance-tested |

Each checkpoint is a **playtest**, not a code-review — the owner should actually play the build start-to-finish (or start-to-checkpoint) before marking it met, not infer it from "the tasks are checked off."

---

## 4. Integration Weeks

Unlike a team project where "integration week" means merging separate developers' branches, this is a solo-dev plan — integration risk here is primarily **temporal coupling** (does System B, built in Week 5, actually work correctly against System A as built in Week 2, or did Week 2's implementation drift from what Week 5 assumed). Two points in the schedule are explicitly reserved for this:

- **Week 6** functions as a light integration week in addition to its own feature work — it's the first week where nearly every prior file's output gets exercised together (kit presets at mission start, exfil vehicle closing out the loop File 01 opened in Week 1). Budget slack here if Weeks 1–5 ran even slightly long.
- **Week 8** is a full dedicated integration week, per Section 1 above — no exceptions, no feature creep into this week even if Week 7's squad member work finished early. If Week 7 finishes ahead of schedule, use the surplus time to *start* Week 8's testing early, not to add scope.

---

## 5. Buffer Time Allocation for Highest-Risk Systems

Cross-referencing the Master Brief's risk register:

| Risk | Where its buffer lives |
|---|---|
| Doomed-zone timing (Risk #1) | Data-driven from Week 3; retuned using Weeks 4–8 cumulative playtest data, no dedicated week but continuously adjustable without a rebuild |
| Resupply window length (Risk #2) | Same — data-driven from Week 5, tuned in Week 8's dedicated tuning pass |
| Temporary squad member (Risk #3) | Entire Week 7 is its buffer; File 07 Section 7 defines the in-week cut order if even that dedicated week is insufficient |
| Drop timing vs. resupply alignment (Risk #4) | Tuned alongside window length in Week 8 |
| Early extraction reward gap (Risk #5) | Verified against GameplayPlan.md's tier table in Week 8's acceptance pass (File 10) |
| Missing `AShooterGameState` (Risk #6) | Front-loaded to Week 1 specifically because every other risk's mitigation depends on it existing |
| Multiplayer verification bandwidth (Risk #7) | Explicit cross-network (not just PIE) test pass scheduled in Week 8, not left implicit |
| Human AI scope (Risk #8) | Flanking is the named cut-first item within Week 3 itself, not deferred to a later buffer |

The Sep 7–9 buffer is the final backstop for all of the above, not the primary mitigation for any of them — every risk above already has a mitigation living inside the 8-week plan proper.

---

## 6. Demo Readiness Checklist — Week 8 Exit

Before entering the Sep 7–9 buffer, confirm:

- [ ] All 11 Definition of Done items in [MASTER_BRIEF.md](MASTER_BRIEF.md) Section 7 pass.
- [ ] All acceptance-criteria checklists in Files 01–08 pass (cross-check against [10_Demo_Definition_and_Acceptance.md](10_Demo_Definition_and_Acceptance.md)'s consolidated per-system tables).
- [ ] At least 3 consecutive full-session playthroughs completed without a crash, softlock, or stuck UI state.
- [ ] At least 1 full-session playthrough completed on physically separate machines/networks (not PIE-only).
- [ ] Every numeric tuning knob identified across Files 01–07 has been touched at least once since its default value was first authored (i.e., nothing is still sitting at a placeholder guess from Week 1–2 that no one has looked at since).
- [ ] The Section 1 triage priority order (Section 1, "Buffer") is understood and agreed before entering the buffer window, so no time is lost deciding what to cut under deadline pressure if something is still failing.
