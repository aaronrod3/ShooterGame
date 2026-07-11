# Wave Mission Mode — Demo Plan Master Brief

**Project:** ShooterGame (UE5.8, C++/Blueprint hybrid, server-authoritative co-op)
**Repository:** https://github.com/aaronrod3/ShooterGame
**Plan window:** Monday, July 13, 2026 → Wednesday, September 9, 2026 (8 working weeks + 3-day final buffer)
**UE5.8 documentation reference:** https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation

This folder (`docs/WaveMission_DemoPlan/`) is the authoritative execution plan for taking Wave Mission Mode from its current state (Layer 1 partially complete — movement, basic zombie AI, death/respawn, weapon pickups) to a fully playable working demo that exercises every system described in [`docs/GameplayPlan.md`](../GameplayPlan.md). This plan does not replace `GameplayPlan.md` — that document remains the design authority for *what* the mode is. This plan defines *how and when* it gets built.

Everything here builds on top of, and must stay consistent with, the architecture already declared in [`CLAUDE.md`](../../CLAUDE.md): the `AShooterGameCharacter` component composition, the replication pattern (`ReplicatedUsing` + `Server_X()` + `OnRep_X()` + delegate broadcast), the Mission State Machine phase list, the `AZoneDefinition` zone-state model, and the `ScoringSubsystem`/`PersonalPoints`/`SquadSharedFund` economy nouns. Where this plan introduces a new class, it either extends an existing one (e.g. Kit Presets extend `FLoadoutData`) or fills a gap CLAUDE.md already named but that does not yet exist in `Source/` (e.g. `AZoneDefinition`, `ScoringSubsystem`, `AShooterGameState` — confirmed absent from the codebase as of 2026-07-09).

---

## 1. Project Goal and Demo Definition

**Goal:** Produce a working, playable, multiplayer-capable demo build of Wave Mission Mode that a squad of 1–4 players can run start-to-finish: insert into Zone 1, clear it, complete the Phase 2 task under wave pressure, exfil under pursuit through a transit corridor, resupply, repeat the arc at Zone 2, and either extract on the primary exfil vehicle or fail gracefully. Every core pillar in `GameplayPlan.md` must be *experienceable*, not just implemented in isolation.

"Working demo" specifically means:

- A single continuous map exists with at least 2 objective zones (minimum viable — see [01_Phase_Triggers_and_Map_Structure.md](01_Phase_Triggers_and_Map_Structure.md)) connected by a transit corridor.
- All four mission phases are reachable in sequence, in one continuous session, without a designer manually triggering state via console commands.
- The economy (points, shared fund, resupply windows, at minimum ammo + medical call-ins) is live and server-authoritative.
- Kit presets can be configured pre-mission and swapped mid-mission during a resupply window.
- Early extraction works at any point and does not end the session for remaining squad members.
- The demo survives a full playtest session (see [10_Demo_Definition_and_Acceptance.md](10_Demo_Definition_and_Acceptance.md)) without a server-crashing bug or a phase-transition softlock.

This is a **demo**, not a shippable vertical slice — cosmetic polish, full progression/unlock systems, and class-locking are explicitly out of scope (see File 11).

---

## 2. Two-Month Timeline Summary

Full week-by-week detail lives in [09_Two_Month_Schedule.md](09_Two_Month_Schedule.md). Summary:

| Week | Dates (2026) | Focus |
|---|---|---|
| 1 | Jul 13 – Jul 19 | Foundation: `AShooterGameState`, `EMissionPhase` state machine, `AZoneDefinition`, minimum viable map greybox |
| 2 | Jul 20 – Jul 26 | Wave spawn manager, zone density anchoring, Phase 1 clearance threshold — **Checkpoint A: Phase 1 loop playable solo** |
| 3 | Jul 27 – Aug 2 | Objective/task system (Phase 2 terminal), overrun threshold, doomed-zone spawn logic |
| 4 | Aug 3 – Aug 9 | Field extraction points, reward tiers, Phase 3 pursuit-radius exfil — **Checkpoint B: Phases 1–3 playable, exfil works** |
| 5 | Aug 10 – Aug 16 | Economy core: points, `SquadSharedFund`, resupply window system, ammo/medical call-ins |
| 6 | Aug 17 – Aug 23 | Kit preset system (loadout screen, in-mission swap), Phase 4 zone mirror + exfil vehicle — **Checkpoint C: full 4-phase arc playable** |
| 7 | Aug 24 – Aug 30 | Fire-support call-ins (mortar, chopper, artillery, flare), temporary squad member first pass, networking hardening |
| 8 | Aug 31 – Sep 6 | Integration week: full-loop multiplayer testing, bug fixing, acceptance test pass — **Checkpoint D: demo-complete** |
| Buffer | Sep 7 – Sep 9 | Contingency for any Week 8 acceptance failures, final playtest, freeze |

Each two-week block ends on a checkpoint that must be playable end-to-end for everything built so far, not just individually functional — this is enforced in File 10's acceptance tables.

---

## 3. Complete Feature List for the Demo

Numbered list of every feature/system that must be complete. Each maps to a detail file.

1. **Mission phase state machine** (`EMissionPhase`, `AMissionStateManager`, `AShooterGameState`) — File 01, File 08
2. **Zone definition and zone-state model** (`AZoneDefinition`: Neutral → Active → Doomed → Cleared) — File 01
3. **Minimum viable map** (2 objective zones + 1 transit corridor) — File 01
4. **Enemy density anchoring** (zone-anchored spawn tables, transit sparsity) — File 01, File 02
5. **Hot-zone escalation** (spawn pressure increases as waves progress) — File 01, File 02
6. **Pursuit radius system** (Phase 3 doomed-zone exit condition) — File 01, File 02
7. **Wave spawn manager** (`AWaveSpawnManager`, `UWaveSetDataAsset`) — File 02
8. **Zombie AI extensions** (night variants, escalation-aware aggression) — File 02
9. **Human AI reinforcements** (cover, suppression, flanking) — File 02
10. **Overrun threshold / doomed-zone endless spawn loop** — File 02
11. **Call-in enemy attraction and distraction flare pathing diversion** — File 02, File 05
12. **Stationary objective task system** (`AObjectiveTerminal`, `UObjectiveTaskComponent`) — File 03
13. **Phase 2 data-download task** — File 03
14. **Phase 4 comms-tower upload task** (mirrors Phase 2) — File 03
15. **Field extraction points** (`AFieldExtractionPoint`, timed vulnerability window) — File 04
16. **Phase-based reward tier calculation** — File 04
17. **Phase 4 exfil vehicle** (`AExfilVehicle`, fixed LZ, timed departure) — File 04
18. **Secondary emergency extraction fallback** — File 04
19. **Left-behind state / full gear loss on death** — File 04
20. **Points economy** (`ScoringSubsystem`, `PersonalPoints`, `SquadSharedFund`) — File 05
21. **Resupply window system** (timed overlay, purchase locking) — File 05
22. **Call-in purchase and drop-scheduling system** (`UCallInDefinition`, `ACallInManager`) — File 05
23. **Ammo + medical supply crates** — File 05
24. **Mortar, chopper, artillery fire support** — File 05
25. **Distraction flare** — File 05 (system), File 02 (AI response)
26. **Squad vote / host approval for shared-pool purchases** — File 05
27. **Kit preset data structure and persistence** (extends `FLoadoutData`) — File 06
28. **Island base loadout screen (minimal UMG)** — File 06
29. **Mission-start preset selection UI** — File 06
30. **In-mission kit swap during resupply windows** — File 06
31. **Squad visibility of teammates' presets during resupply** — File 06
32. **Temporary squad member** (`ATemporarySquadMember`, BT, radial command menu) — File 07
33. **Full replication strategy for all above systems** — File 08
34. **Session lifecycle / squad membership / early-extract-without-ending-session** — File 08

---

## 4. UE5 Systems Leveraged Per Major Feature

All references point to the UE5.8 documentation root: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation

| Feature Area | Primary UE5 Subsystem(s) |
|---|---|
| Mission phase state machine, GameState/PlayerState economy fields | **Actor Replication**, **Gameplay Framework (GameMode/GameState/PlayerState)** — Networking and Multiplayer docs |
| Zone spawn management, wave escalation | **Timers (`FTimerManager`)**, **Data Assets (`UPrimaryDataAsset`)**, **GameplayTags** |
| Zombie/Human AI, temporary squad member | **Behavior Trees**, **Blackboard**, **AI Perception System**, **Environment Query System (EQS)**, **NavMesh (`RecastNavMesh`)** |
| Objective task system, resupply overlay, radial command menu, loadout screens | **UMG (Unreal Motion Graphics)**, **Widget Blueprints**, **Enhanced Input** |
| Extraction, exfil vehicle, resupply timers | **Replicated Timers via `GetServerWorldTimeSeconds()`**, **Actor lifecycle (`SetLifeSpan`, `Destroy`)** |
| Kit presets, save/load | **`USaveGame`**, **`UGameplayStatics::SaveGameToSlot`/`LoadGameFromSlot`** (routed exclusively through `UShooterSaveGameSubsystem` per CLAUDE.md) |
| Fire support strikes, AOE damage | **`UGameplayStatics::ApplyRadialDamage`**, **Niagara VFX**, **Timers for authored delay** |
| Networking session structure | **Online Subsystem**, **`APlayerController`/`APlayerState` join-in-progress handling**, **RPC reliability (Server/Client/NetMulticast, Reliable/Unreliable)** |

Every detail file repeats the relevant subset of these references inline, next to the specific class or feature they apply to, per the formatting requirement.

---

## 5. Risk Register

High-risk design areas from `GameplayPlan.md`, each with a concrete mitigation strategy and the file where the mitigation is elaborated.

| # | Risk | Why It's High-Risk | Mitigation Strategy | Detail File |
|---|---|---|---|---|
| 1 | **Doomed-zone timing** — overrun threshold must feel like pressure, not an arbitrary timer | Pure numeric tuning problem; wrong values make Phase 2/3 feel unfair or trivial | Build the escalation curve as data (`UWaveSetDataAsset` curve table), not hardcoded constants, so it can be retuned without a rebuild; prototype with a debug HUD showing live spawn-rate multiplier during Week 3 playtesting | File 02, File 01 |
| 2 | **Resupply window length** — 90 sec starting point, must be adjusted per playtest | Same numeric-tuning risk; also has UI/UX implications (does the countdown feel rushed?) | Expose window duration as an `EditAnywhere` field on `AMissionStateManager`, not a literal; run a dedicated timing playtest pass in Week 8 | File 05, File 09 |
| 3 | **Temporary squad member AI** — highest implementation complexity in the entire feature list | Combines BT authoring, radial UI, command dispatch, and outfit-from-preset in one system; easiest system to blow the schedule on | Scoped explicitly to **last** in build order (Week 7, after everything else is stable); scope-reduction fallback defined up front (ship with 3 commands instead of 6 if Week 7 runs long) | File 07 |
| 4 | **Drop timing vs. resupply alignment** — call-ins must arrive on a schedule that matches phase pacing, not instantly | Misalignment breaks the "planning pays off" pillar — if drops feel instant or feel too slow, the tactical decision disappears | Drop-window timing is a data-driven interval on `UCallInDefinition`, tuned alongside resupply window length in the same Week 8 pass; ammo/medical (low complexity) are built first in Week 5 specifically so there's time left to tune before fire-support call-ins are layered on in Week 7 | File 05 |
| 5 | **Early extraction reward gap** — Phase 2 vs Phase 4 payout must incentivize completion without making partial extraction worthless | A balance-only risk with a real design failure mode (players extract at first opportunity every time, mission never gets played fully) | Reward tiers implemented as a single data table (`UExtractionRewardTable` or curve) reviewed in the Week 8 acceptance pass against the tier table already defined in `GameplayPlan.md` §"Early Extraction Reward Tiers" | File 04, File 10 |
| 6 | **No `AShooterGameState` currently exists** | Every economy/phase/timer field in this plan needs a replicated home; `AShooterGameGameMode` currently extends `AGameModeBase` (no GameState pairing beyond the engine default) | Create `AShooterGameState : AGameStateBase` in Week 1 as the very first task — everything else in the plan depends on it existing | File 01, File 08 |
| 7 | **Multiplayer verification bandwidth** — solo dev, limited time to manually test 2–4 client sessions every iteration | Networking bugs (desync, RPC ordering, join-in-progress) are the most expensive class of bug to find late | Two dedicated PIE-multiclient test passes per checkpoint (see File 09), not just at the end; File 08 defines authority model up front so systems are built server-authoritative from day one instead of retrofitted | File 08, File 09 |
| 8 | **Scope overrun on human AI reinforcement (cover/suppression/flanking)** | Full tactical AI (cover selection via EQS, suppression, flanking) is itself a multi-week feature in most shooters | Ship a reduced first-pass: EQS-based cover selection + simple suppression fire, no flanking coordination logic between multiple human AI, explicitly flagged as a stretch item if Week 3 runs long | File 02, File 10 |

---

## 6. How the Section Files Relate to Each Other

```
                         MASTER_BRIEF.md (this file)
                                   │
        ┌─────────────────┬───────┴────────┬──────────────────┐
        │                 │                │                  │
01_Phase_Triggers   02_Enemy_AI       03_Objectives     04_Extraction
_and_Map_Structure  _and_Waves        _and_Task_System  _and_Exfil
        │                 │                │                  │
        │  (zone/phase    │  (spawns INTO  │  (task completion│  (reads phase
        │   state feeds   │   zones defined│   triggers phase │   completion to
        │   all others)   │   in File 01)  │   transitions)   │   compute reward
        │                 │                │                  │   tier)
        └────────┬────────┴────────┬───────┴─────────┬────────┘
                  │                 │                 │
          05_Economy_and_Resupply   │         06_Kit_Presets_and_Loadout
          (resupply windows gate    │         (swap only valid during
           kit swap + call-ins;     │          resupply windows opened
           reads phase state)       │          by File 05 / File 01)
                  │                 │                 │
                  └────────┬────────┴────────┬────────┘
                            │                 │
                  07_Temporary_Squad_Member   │
                  (purchased via File 05      │
                   economy, outfitted via     │
                   File 06 presets, built     │
                   LAST — depends on both)    │
                            │                 │
                            └────────┬────────┘
                                     │
                     08_Multiplayer_and_Networking
                     (cross-cutting — defines replication/
                      authority for every system above;
                      read alongside every other file,
                      not after them)
                                     │
                     09_Two_Month_Schedule
                     (sequences the build order implied
                      by the dependency arrows above)
                                     │
                     10_Demo_Definition_and_Acceptance
                     (defines "done" for every system above)
```

**Reading order for implementation:** File 01 → File 08 (read together, since every subsequent system is built server-authoritative from the start) → File 02 → File 03 → File 04 → File 05 → File 06 → File 07 → validate against File 10 continuously → File 09 governs pacing throughout.

**Dependency rule enforced throughout this plan:** no file's Week N tasks assume a system from a *later* file exists yet. Where an early system needs a stub (e.g., File 01's zone clearance needs *something* to happen on Phase 1 completion before File 05's resupply window exists), the stub is explicitly named in that file's week-by-week tasks.

---

## 7. Definition of Done for the Demo

The demo is complete when **all** of the following are simultaneously true:

1. A host and at least one joined client can start a session, both spawn in, and both see identical mission phase state at all times (verified via `AShooterGameState` replication, not just the host's screen).
2. The squad can progress, in one continuous session with no manual console intervention, through: `PreMission → Phase1Infiltration → Resupply1 → Phase2Objective → Phase3Exfil → Resupply2 → Phase4SecondObjective → Phase4Exfil → PostMission`.
3. Zone 1 and Zone 2 each demonstrate the full Neutral → Active → Doomed → Cleared zone-state cycle at least once during that session.
4. At least one player successfully calls in an ammo crate, a medical crate, and one fire-support strike type during the session, each drawing from `SquadSharedFund` and arriving on a delayed schedule (not instantly).
5. At least one player configures a kit preset pre-mission and successfully swaps to a different stored preset during a resupply window, and a squadmate can see that preset was available to swap to.
6. A player triggers early extraction mid-mission (after Phase 1, before Phase 4) and the session continues uninterrupted for the remaining squad member(s).
7. The Phase 4 exfil vehicle arrives at its fixed LZ, departs at its fixed time, and the demo demonstrates both outcomes at least once across testing: a player aboard at departure (full reward) and a player left behind (emergency extraction or death/gear-loss path).
8. The temporary squad member can be called in, outfitted from a stored preset, commanded through at least 3 of the 6 designed commands, and its death correctly triggers gear loss.
9. No system in the feature list (Section 3) is stubbed, disabled, or hidden behind a debug-only trigger in the final build — every trigger is a real in-game condition (clearance threshold, task completion, timer expiry, player action).
10. The full session survives start-to-finish at least three consecutive times during Week 8 playtesting without a crash, a phase-transition softlock, or a permanently-stuck UI state.
11. Every acceptance test defined in [10_Demo_Definition_and_Acceptance.md](10_Demo_Definition_and_Acceptance.md) passes.

If, entering the Sep 7–9 buffer, any of the above is not true, File 09's Week 8 checklist governs the triage order — protect items 1–3 and 10 (the core loop and stability) over items 8 (temporary squad member) and item 4's fire-support subset, per the scope-reduction guidance in File 07 and the risk register above.
