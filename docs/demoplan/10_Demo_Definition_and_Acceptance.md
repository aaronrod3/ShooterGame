# 10 — Demo Definition and Acceptance

## Dependencies

- Consolidates and cross-references acceptance criteria already stated individually in Files 01–08. This file does not introduce new systems — it is the single place to check "is the demo done," pulling every prior file's Section 9/10 acceptance list into one pass-order.
- [09_Two_Month_Schedule.md](09_Two_Month_Schedule.md) — Week 8 and the Sep 7–9 buffer are explicitly reserved for running the tests defined here.
- [MASTER_BRIEF.md](MASTER_BRIEF.md) Section 7 — this file is the detailed expansion of that section's Definition of Done.

---

## 1. Minimum Viable Feature Set

### 1.1 Must work for the demo to count as complete

Everything in this list is load-bearing for the Master Brief's Definition of Done — none of it is negotiable within the 2-month window without a corresponding change to that document.

- Mission phase state machine cycling all 9 `EMissionPhase` values in order, in one continuous session.
- 2 objective zones + 1 transit corridor, each zone cycling its full `EZoneState` set at least once.
- Wave spawning anchored to zone state, with escalation and an uncapped doomed-state spawn loop.
- Zombie AI (existing, extended with night variants) and a first-pass human AI reinforcement (cover + suppression at minimum; flanking is a stretch — see Section 1.2).
- Stationary objective task system, functional identically at both Zone 1 (Phase 2) and Zone 2 (Phase 4).
- Overrun threshold forcing an unwinnable-but-escapable doomed zone if a task is not completed in time.
- Field extraction points, usable at any time, with all 6 reward tiers correctly calculated.
- Phase 4 exfil vehicle with fixed arrival/departure timing, both "aboard" and "left behind" outcomes demonstrated.
- Secondary emergency extraction and full-gear-loss-on-death for left-behind players.
- Points economy (`PersonalPoints` + `SquadSharedFund`) fed by all 6 award-event types from GameplayPlan.md.
- Resupply window system (timed, purchase-locked outside windows, unspent currency lost on expiry).
- Ammo and medical supply crates, each with their designed cost-beyond-currency (enemy attraction / despawn timer).
- At least one fire-support call-in type functional end-to-end (mortar is the recommended minimum if time forces a cut among mortar/artillery/chopper — see Section 1.2).
- Distraction flare with a demonstrable AI pathing diversion effect.
- Kit preset creation/persistence, mission-start selection, and in-mission resupply-window swap.
- Squad visibility of teammates' presets during a resupply window.
- Temporary squad member: purchasable, correctly outfitted, at least `HoldPosition` + `Follow` commands functional, one-per-team enforced, death causes gear loss.
- Server-authoritative validation on every purchase, phase transition, and command dispatch (no client-trusted gameplay state anywhere).
- A session that survives 3 consecutive full playthroughs without a crash, softlock, or stuck UI state.

### 1.2 Stretch goals — nice to have, not required

Explicitly **not** required for the demo to be considered complete. If any of the "must work" items above are at risk, cutting from this list first is the correct trade, in roughly this priority order (cut top-of-list first):

1. `BreachRoom` and `CoverRetreat` temporary squad member commands (File 07 Section 7's own stated cut order).
2. Chopper and artillery fire-support strikes (keep mortar only).
3. Human AI flanking behavior (`BTService_TagFlanker`) — cover + suppression alone still satisfies the design doc's bar.
4. Zones 3–5 (forest, radioactive blast zone) — the demo's 2-zone structure is explicitly the minimum viable map, not the target end-state map.
5. Mid-session join-in-progress support (File 08 Section 3.1) — pre-session join only is sufficient.
6. A fully modeled, walkable island base hub level for the loadout screen — a UMG pre-mission menu is sufficient (File 06 Section 3).
7. Command-transfer-to-another-player if the commanding player of a temporary squad member leaves (File 07 Section 6).

### 1.3 Explicitly out of scope for this plan (not stretch goals — not attempted)

- Full cosmetics system (skins, camo variants beyond the existing `VariantID`/`ColorVariantID` fields already in `FLoadoutSlot`/`FCharacterAppearance`).
- Full player progression/unlock system (XP levels, unlockable weapons/attachments) — the demo awards XP/currency per GameplayPlan.md's reward tiers but does not implement anything those rewards unlock long-term.
- Class-locking (Assaulter/Support/Heavy/Scout remain informational tags only, per File 06 Section 5 — this is a permanent design decision for this build, not a scope cut).
- Per-part weapon durability (`FWeaponPartCondition` — CLAUDE.md already flags this as "a declared stub — do not wire up without a deliberate design pass"; this plan does not change that status).
- A full day/night cycle system (File 02 Section 2.1 uses a simple flag, not an authored cycle).
- Drop-on-death loot for the temporary squad member (File 07 Section 5).
- Multiple independent squads on one server (File 08 Section 5).
- Full 3D world-space targeting UI for temporary squad member `MoveTo`/`BreachRoom` (crosshair-trace is sufficient — File 07 Section 3).
- Weapon/magazine AnimBPs and the broader Infima integration plan's Phase 5–8 items — those are tracked in [`docs/infima_integration_plan.md`](../infima_integration_plan.md) and are a parallel, largely orthogonal effort to this plan. Where the two plans intersect (e.g., both touch character animation state), this plan assumes the Infima integration plan's relevant phases are handled independently and does not duplicate that work here.

---

## 2. Per-System Acceptance Tests

One table per major system. "Pass condition" and "fail condition" are written to be executed directly during Week 8 testing, not reinterpreted.

### 2.1 Mission Phase State Machine (File 01)

| Test | Pass Condition | Fail Condition |
|---|---|---|
| Full phase sequence, one session | All 9 `EMissionPhase` values occur in the exact documented order with zero manual console intervention | Any phase is skipped, occurs out of order, or requires a debug command to advance |
| Host/client phase sync | `AShooterGameState::CurrentMissionPhase` reads identically on host and every client at every phase, checked at each transition | Any client ever displays a different phase than the host for more than one network frame |
| Illegal transition rejection | A debug-triggered out-of-order `Server_RequestPhaseAdvance` call is rejected, phase unchanged | The phase changes to an illegal next value |

### 2.2 Zone State and Map Structure (File 01)

| Test | Pass Condition | Fail Condition |
|---|---|---|
| Zone 1 full cycle | Neutral → Active → Doomed → Cleared occurs exactly once, matching phase transitions | Zone gets stuck in any state, or skips Doomed |
| Zone 2 full cycle | Neutral → Active → Cleared occurs (no Doomed required) | Zone 2 incorrectly enters Doomed, or never clears |
| Transit density | Measured spawn count over a fixed 5-minute window in transit is lower than the same window in an Active zone | Transit density is equal to or higher than zone density |
| Pursuit radius, multi-player | All living players must clear the radius simultaneously before the zone clears; a lagging player blocks it | Zone clears while a player is still inside the pursuit radius |

### 2.3 Enemy AI and Waves (File 02)

| Test | Pass Condition | Fail Condition |
|---|---|---|
| Doomed-state escalation | Spawn-rate multiplier measurably increases at least every 15s for 5+ continuous minutes, no plateau | Multiplier stops increasing or is clamped |
| Night variant gating | Night-variant zombies only spawn when `bIsNightPhase == true` | Night variants spawn during a non-night phase |
| Human AI cover usage | ≥80% of observed human AI encounters show the AI taking cover before engaging | AI stands in the open in the majority of encounters |
| Enemy attraction | A crate/strike spawn within perception range redirects ≥1 nearby wandering zombie in ≥4 of 5 manual trials | Zombies never react to the stimulus |
| Distraction flare selectivity | Flare diverts a wandering zombie but does not pull one actively engaged with a player in melee range | An actively-engaged zombie abandons a player for a distant flare |

### 2.4 Objectives and Task System (File 03)

| Test | Pass Condition | Fail Condition |
|---|---|---|
| Lock gating | Terminal is non-interactable until zone clearance + correct phase are both true | Terminal is interactable early |
| Stationary detection | Progress advances only while required player count is stationary in range; stepping away holds (not resets) progress | Progress resets to zero on a brief step-away, or advances while no one is present |
| Pressure response | Wave intensity in-zone measurably increases specifically at task start, not merely at zone-active | No measurable change in spawn behavior at task start |
| Overrun handoff | Zone enters Doomed if task not completed within `OverrunTimeLimit`, mission does not crash or hang | Mission softlocks or silently ignores overrun expiry |

### 2.5 Extraction and Exfil (File 04)

| Test | Pass Condition | Fail Condition |
|---|---|---|
| Reward tier accuracy | All 6 tiers match GameplayPlan.md's table exactly, verified by triggering extraction at each phase boundary | Any tier awards the wrong reward |
| Vulnerability/cancellation | Damage or leaving range during extraction resets progress to zero | Extraction completes despite damage/range violation, or pauses instead of resetting |
| Concurrent extraction | Two players extracting at the same point simultaneously do not interfere with each other's timers | One player's cancellation affects the other's progress |
| Exfil vehicle timing | Vehicle arrives at exactly `ArrivalDelayAfterUpload` after task completion, consistent across repeated runs | Arrival time drifts between runs |
| Left-behind resolution | A left-behind player can use emergency extraction (reduced-currency tier) or dies with confirmed full gear loss | Left-behind player has no valid resolution path, or retains gear after death |
| Session continuity | Early extraction or left-behind death does not end the session while ≥1 other player is active | `HandleMatchOver` fires incorrectly on a single player's extraction/death |

### 2.6 Economy and Resupply (File 05)

| Test | Pass Condition | Fail Condition |
|---|---|---|
| Award correctness | Each of the 6 award-event types fires exactly once per qualifying event, no double-counting | Any event double-awards or fails to award |
| Purchase phase-gate | Server rejects a call-in RPC attempted outside `Resupply1`/`Resupply2`, even via direct debug RPC | Purchase succeeds outside a resupply window |
| Unspent currency loss | `SquadSharedFund`/`PersonalPoints` become unspendable (not refunded, not cleared) the instant the window phase ends | Currency is refunded, or remains spendable after window close |
| Delayed delivery | A purchased call-in arrives no earlier than its `DropDelaySeconds`, snapped to the next drop-window tick, verified across ≥3 requests at different times within a window | Any call-in arrives instantly |
| Artillery safety check | Request is rejected if any living player is within `MinSafeDistance` of the target | Artillery can be called in near a player |
| Solo-session vote handling | A solo player can successfully purchase a vote-gated High-tier call-in without soft-locking | Solo session cannot ever approve a High-tier call-in |

### 2.7 Kit Presets and Loadout (File 06)

| Test | Pass Condition | Fail Condition |
|---|---|---|
| Persistence | Up to 6 presets save and reload correctly across a full editor/session restart via `UShooterSaveGameSubsystem` | Presets are lost on restart, or bypass the save subsystem |
| Mission-start apply | Selected preset's weapons and inventory items are equipped before the character is first visible to others | Gear pops in visibly late, or wrong preset applied |
| Class tag neutrality | Grep audit confirms `ClassTag` is never read by any equip-validation or gating code path | Any equip/interact path branches on `ClassTag` |
| Mid-mission swap correctness | Swapping presets during resupply fully replaces old equipped items with the new preset's — no leftover items | Old preset's items remain alongside the new preset's |
| Squad visibility bandwidth | Preset data is only sent to squadmates on-demand during a resupply window, not continuously | Network profiling shows preset data replicated outside resupply windows |

### 2.8 Temporary Squad Member (File 07)

| Test | Pass Condition | Fail Condition |
|---|---|---|
| Outfitting accuracy | Spawned squad member's equipped gear exactly matches the selected preset | Gear mismatch or missing items |
| Command floor | At minimum `HoldPosition` and `Follow` produce clearly distinguishable behavior | Commands produce identical or no observable behavior |
| Command persistence | Squad member continues its last command indefinitely with no further input (tested over several minutes) | Squad member reverts to idle/default behavior without a new command |
| Command authority | Only the commanding player's RPCs are accepted; another player's attempt is rejected server-side | Any player can command any squad member |
| One-per-team | A second purchase attempt while one is active and alive is rejected | Two squad members can be active simultaneously |
| Death gear loss | Squad member death clears `ActiveSquadMember` and triggers no gear-return logic | Gear is returned to the player, or `ActiveSquadMember` is not cleared |

### 2.9 Multiplayer and Networking (File 08)

| Test | Pass Condition | Fail Condition |
|---|---|---|
| Authority audit | Every gameplay-affecting mutation across Files 01–07 traces to a `Server_`-prefixed function | Any client-side direct mutation of gameplay state found |
| Timer representation audit | Every timed system uses a server-time end-timestamp, none use a raw replicated countdown float | Any system found using a raw countdown |
| Cross-network session | A full session completes correctly on 2 physically separate machines/networks | Any desync observed that did not appear in same-machine PIE testing |
| Race condition | Two near-simultaneous High-tier call-in requests do not double-spend `SquadSharedFund` | Double-spend occurs |

---

## 3. Known Out-of-Scope Items for the Demo

Restated concisely from Section 1.3 for quick reference during any scope conversation:

- Full cosmetics beyond existing skin/color-variant fields.
- Full progression/unlock trees.
- Class-locking (permanently informational for this build, not a temporary cut).
- Weapon part durability (`FWeaponPartCondition`).
- Authored day/night cycle (a flag suffices).
- Squad member gear drop-on-death.
- Multiple independent squads per server.
- Full 3D world-space command-targeting UI.
- Infima integration plan's animation-layer phases (tracked separately in `docs/infima_integration_plan.md`).

Any request to add one of these during the 2-month window should be treated as a scope-change decision requiring an explicit trade-off against the Master Brief's Definition of Done, not a routine task.

---

## 4. Playtest Plan

### 4.1 Ongoing per-checkpoint playtests (Weeks 2, 4, 6 — see File 09)

At each checkpoint (A, B, C), run the full playable slice solo, then with at least one additional PIE client. Record: any crash, any softlock, any point where the intended next action wasn't obvious from in-game feedback alone (a UX signal, not just a functional bug). Log findings as GitHub Issues immediately per CLAUDE.md's workflow rule ("Log bugs and discoveries as Issues immediately — never hold in chat"), tagged with the appropriate severity label (`p0-critical` for anything session-ending, `p1-high` for anything on the must-work list in Section 1.1).

### 4.2 Full end-to-end demo session validation (Week 8)

**Setup:** minimum 2 players, at least one test run on physically separate machines/networks (File 08 Section 8's Week 8 requirement) rather than only multi-client PIE.

**Script — run this exact sequence at least 3 times consecutively without restart-on-failure:**

1. Both players join pre-session (`PreMission`), each configures/selects a kit preset.
2. Insert, clear Zone 1 to threshold, confirm `Resupply1` opens automatically.
3. In `Resupply1`: one player purchases an ammo crate (confirm delayed arrival + enemy attraction), one player swaps to a different kit preset.
4. Let the window expire naturally; confirm remaining points become inert.
5. Engage the Phase 2 terminal, confirm pressure response and progress bar sync; complete the task.
6. Confirm Zone 1 goes doomed; both players run the pursuit radius together; confirm `Resupply2` opens.
7. In `Resupply2`: purchase a mortar strike and (if built) the temporary squad member; issue at least 2 different commands to the squad member.
8. Advance into Zone 2, clear via `AreaControl`, complete the Phase 4 terminal task.
9. Confirm the exfil vehicle arrives on schedule; **first full run:** both players board before departure, confirm full mission reward. **Second full run:** deliberately leave one player behind, confirm left-behind resolution (test both emergency extraction and death/gear-loss paths across repeated runs).
10. Confirm `PostMission` is reached cleanly and the session can end/reset without requiring a server restart.

**Separately, at least once:** have one player extract early (after Phase 1, and again in a different run after Phase 2) via a field extraction point and confirm the session continues uninterrupted for the remaining player, with the correct reward tier awarded to the extractor.

### 4.3 Exit condition

The playtest plan is considered passed when: Section 4.2's full script completes 3 consecutive times with no `p0-critical` issue logged, both the aboard and left-behind exfil outcomes have been observed at least once, and both early-extraction test cases in the "separately" note have been confirmed. This, combined with every table in Section 2 passing, is what satisfies Master Brief Definition of Done item 11.

---

## 5. Estimated Effort

| Task Cluster | Estimate |
|---|---|
| Writing/maintaining acceptance test scripts (ongoing, light touch per checkpoint) | 2 dev-days (spread across Weeks 2, 4, 6) |
| Full Section 2 acceptance-table execution (Week 8) | 3 dev-days |
| Section 4.2 full end-to-end scripted playtest sessions (3+ consecutive runs, both machines-separate and PIE) | 2.5 dev-days |
| Issue logging, triage, and fix-verification loop during Week 8 | 2.5 dev-days (overlaps with, but is distinct from, the fixing time already budgeted in Files 01–08's own estimates) |
| **Total** | **10 dev-days** (concentrated in Week 8 and the Sep 7–9 buffer, per File 09's schedule) |
