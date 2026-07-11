# 02 — Refactor Schedule

How the findings in [01_REFACTOR_FINDINGS.md](01_REFACTOR_FINDINGS.md) slot into the existing week-by-week timeline in `docs/WaveMission_DemoPlan/09_Two_Month_Schedule.md` (Weeks 1-8, Mon Jul 13 – Sun Sep 6 2026, plus a Sep 7-9 buffer). This file does not replace that schedule — it inserts refactor tasks at the points where they must land relative to the feature work already planned there. Every Blocking item is scheduled strictly before the demo-plan week that first needs it.

---

## Week 1 (Jul 13 – Jul 19) — Foundation

**Already scheduled (from `09_Two_Month_Schedule.md`):** `AShooterGameState`, `EMissionPhase`, `AZoneDefinition` (state/config only), minimum viable map greybox.

**Refactor work inserted this week:**

| Item | Type | Effort | Why this week |
|---|---|---|---|
| **B1** — Confirm `AShooterGameState` is built as the first task, not assumed | Blocking | 0 dev-days (already first in the existing schedule — this audit confirms that ordering is correct, no change needed) | Already correctly first; this row exists only to record that the audit validated it |
| **B2** — Decide reuse-vs-rewrite for `AZombieSpawnManager`/`AZombieObjectiveArea` vs. the new zone/wave system | Blocking | 1 dev-day | Must be decided before Week 2's `AWaveSpawnManager` build begins — deciding after Week 2 starts means throwing away whatever was already built against the wrong assumption |
| **H4** — Declare a project log category (`LogWaveMission` or per-system) | High-value | 0.5 dev-days | Cheapest possible time to do it — every subsequent new class (dozens, across Weeks 1-8) adopts it from creation instead of needing a later find-replace |
| **H6** — Verify whether `PlayerAnimInstance` is live or dead (Reference Viewer check) | High-value ⚠ needs runtime verification | 0.25 dev-days | Zero dependency on anything else; cheapest to check now while the editor is already open for Week 1's map greybox work. Deletion itself (if confirmed dead) is not scheduled — optional, do only if convenient |

**Week 1 total added:** 1.75 dev-days (against Week 1's existing ~5-day budget in `docs/WaveMission_DemoPlan/01_Phase_Triggers_and_Map_Structure.md` §10, which already runs light relative to Weeks 2-4 — this fits without displacing planned work).

---

## Week 2 (Jul 20 – Jul 26) — Phase 1 Loop

**Already scheduled:** `AWaveSpawnManager`, Zone 1 clearance threshold, spawn-point tagging, pursuit-radius check code. **Checkpoint A** at week end.

**Refactor work inserted this week:**

| Item | Type | Effort | Why this week |
|---|---|---|---|
| **B3** — Centralize team IDs (`ETeamID`/named constants + attitude helper) | Blocking (gates Week 3's Human AI Reinforcement and Week 7's Temporary Squad Member) | 1.5 dev-days | Small and standalone — does the least damage to schedule momentum done now, while `AWaveSpawnManager` work is the main focus and this can run in parallel rather than stealing time from a later week that's already fuller (Week 3 has Objective+Doomed+Human-AI all landing at once per the existing schedule) |
| **H1a** — Extract `UInteractionComponent` from `ShooterGameCharacter` | High-value | 2.5 dev-days | Not blocking, but File 03 (Week 3) adds the first new `IInteractable` consumer (`AObjectiveTerminal`) and File 04 (Week 4) adds a second (`AFieldExtractionPoint`) — doing the extraction before either exists means both are built against the clean component from day one instead of the inline character code, avoiding a mid-stream API change later. This is the last natural gap before that pressure starts |

**Week 2 total added:** 4 dev-days. **Schedule impact:** `docs/WaveMission_DemoPlan/09_Two_Month_Schedule.md` Week 2 is scoped tightly around Checkpoint A (wave spawn manager + Zone 1 clearance). Adding 4 dev-days here risks Checkpoint A slipping. **Recommendation:** if Week 2 is already full, move H1a (not B3) to the Week 3/4 boundary instead — B3 is the one item in this row that's genuinely blocking and should not move.

---

## Week 3 (Jul 27 – Aug 2) — Objective and Doomed-Zone Core

**Already scheduled:** `AObjectiveTerminal`/`UObjectiveTaskComponent`, `EClearanceMode::AreaControl`, hot-zone escalation, doomed-state spawn switch, overrun trigger, human AI first pass.

**Refactor work inserted this week:** None required. B3 (team IDs) must have already landed by the start of this week's human-AI work (see Week 2) — if it slipped, it must be pulled forward and completed at the very start of this week before `AHumanEnemyAIController`/`BTTask_TakeCover` work begins, since those systems are the first real consumers of team-attitude logic.

**Corrected-assumption reminder for this week:** Per `01_REFACTOR_FINDINGS.md`'s "Corrected assumptions" section, `AZombieAIController` already has `UAIPerceptionComponent` with Sight+Hearing configured — do not schedule "add perception hearing sense" as a task this week, it doesn't need doing.

---

## Week 4 (Aug 3 – Aug 9) — Exfil Core

**Already scheduled:** `AFieldExtractionPoint`/`UExtractionRewardLibrary`, pursuit-radius wiring, Zone 2/Phase 4 transition wiring, night variants, enemy-attraction wiring, Zone 2 wave content. **Checkpoint B** at week end.

**Refactor work inserted this week:**

| Item | Type | Effort | Why this week |
|---|---|---|---|
| **B4** — Extend `AlivePlayers`/`HandleMatchOver` with `EPlayerMissionStatus` | Blocking | 1.5 dev-days (already counted in `docs/WaveMission_DemoPlan/08_Multiplayer_and_Networking.md`'s estimate — not additive, just confirming it must land this week specifically, alongside `AFieldExtractionPoint`, not deferred to Week 8 as that file's own week-by-week table might suggest in isolation) | `AFieldExtractionPoint` (this week's headline feature) is the first system that can produce an "extracted" player — the gap must be closed before Checkpoint B's playtest exercises early extraction with 2+ clients |
| **B5** — Record the gear-loss-vs-lootable-corpse decision for left-behind death | Blocking (decision only) | 0.25 dev-days | Cheap, should happen alongside `AFieldExtractionPoint` design work this week even though the left-behind path itself isn't built until Week 6 — deciding now means Week 6 doesn't reopen a "wait, what were we doing here" design question under more schedule pressure |

**Corrected-assumption reminder for this week:** `AudioPerceptionComponent::EmitSoundEvent` already exists — the enemy-attraction wiring task this week is "call this existing API from crate/strike actors," not "add new AI Perception plumbing." Effort for that task should be lighter than a first read of `docs/WaveMission_DemoPlan/02_Enemy_AI_and_Waves.md` §5.1 implies.

**Week 4 total added:** 0.25 dev-days net-new (B4 was already budgeted elsewhere; this schedule just confirms its timing).

---

## Week 5 (Aug 10 – Aug 16) — Economy Core

**Already scheduled:** Points/`SquadSharedFund`/`UScoringSubsystem`, resupply-window-as-phase model, `UW_ResupplyOverlay`, `UCallInDefinition`/`ACallInManager` skeleton, ammo+medical crates.

**Refactor work inserted this week:**

| Item | Type | Effort | Why this week |
|---|---|---|---|
| **B6** — No action, but read the warning before writing `UScoringSubsystem` | Blocking (informational — the design it warns against was already avoided) | 0 dev-days | Zero cost — this is a "read this paragraph before you start" item, included in the schedule only so it isn't skipped. Confirm `UScoringSubsystem` is a `UWorldSubsystem`/`AShooterPlayerState`-and-`AShooterGameState`-based design, not a `UGameInstanceSubsystem`-with-flat-arrays design like `UQuestTrackerSubsystem` |
| **H5** — Record the points-vs-currency naming/UX distinction | High-value | 0.25 dev-days | Natural to note while actively naming and building the points economy UI this week |

**Week 5 total added:** 0.25 dev-days.

---

## Week 6 (Aug 17 – Aug 23) — Full Arc Closure (Kit Presets + Exfil Vehicle)

**Already scheduled:** Kit preset storage/persistence, `UW_LoadoutScreen`, `UW_PresetSelector`, `AExfilVehicle`, secondary emergency extraction, left-behind resolution, Phase 4 terminal placement, kit-swap economy gating. **Checkpoint C** at week end.

**This is the week with the most Blocking findings converging.** All four Kit-Preset-gating findings (B7, B8, B9, B10) must resolve at or before the start of this week, in this order:

| Item | Type | Effort | Sequencing |
|---|---|---|---|
| **B7** — Resolve `FLoadoutPreset` vs. proposed `FKitPreset` duplication | Blocking | 0.5 dev-days research + decision | **Must be the very first thing done this week, before any Kit Preset code is written.** If the decision is "extend `FLoadoutPreset`," it directly changes what gets built for the rest of the week (skip building a new struct + persistence layer — reuse `SaveLoadoutPresets`/`LoadLoadoutPresets` as-is). This is a same-day, first-thing-Monday decision, not something to leave open while building in parallel |
| **B8** — Scope the magazine-shim conflict (snapshot magazines directly, or defer to post-migration) | Blocking | 0.5 dev-days decision | Immediately after B7, same day — both are scoping decisions that shape the rest of the week's implementation |
| **B9** — Fix `SpawnAndEquipWeaponFromSlot`'s single-slot bug | Blocking | 1.5-2 dev-days | Do this **before** building the mid-mission swap UI/RPC flow (§6 of File 06) — building swap UI against a broken equip path means re-testing the UI once the underlying bug is fixed, wasted sequencing if done in the other order. ⚠ **Partially SUPERSEDED by [11_Equip_State_Decision.md](../WaveMission_DemoPlan/11_Equip_State_Decision.md):** the fix itself is still required and still scheduled/budgeted exactly as below — but the "or accept a one-frame gap if that's judged acceptable" fallback originally offered in `01_REFACTOR_FINDINGS.md`'s write-up of this finding is now closed off. The only acceptable fix is full sequencing (new item attached before old item destroyed) per that decision's Section 5 — no gap is ever acceptable, including during mid-mission kit swaps. |
| **B10** — Ensure `Server_SwapToPreset` updates both `AShooterPlayerState::SavedLoadout` and `ULoadoutComponent::LoadoutData` via the existing `SetFullLoadout(..., true)` path | Blocking | 0.5 dev-days | Fold directly into the `Server_SwapToPreset` implementation task — not a separate step, a correctness requirement on that same piece of code |

**Effort reallocation, not pure addition:** If B7 resolves to "extend `FLoadoutPreset`," the persistence-layer portion of `docs/WaveMission_DemoPlan/06_Kit_Presets_and_Loadout.md`'s existing 17.5 dev-day estimate (specifically §2 "Persistent Storage" and part of §1.3) shrinks by an estimated 3-5 dev-days, since `SaveLoadoutPresets`/`LoadLoadoutPresets` and the "missing item" edge case (`bItemMissingFromStash`) already exist. That saved time approximately offsets B9's 1.5-2 dev-day bug fix and B7/B8's combined 1 dev-day of decision-making — **net effect on Week 6/7's total schedule is close to neutral**, not a net addition, provided B7 resolves toward reuse rather than parallel-build.

**Week 6 total added (worst case, if B7 resolves toward "keep separate"):** ~5 dev-days on top of the existing 17.5. **Best case (B7 resolves toward "extend"):** roughly neutral. Either way, flag Week 6/7 as the tightest point in the schedule and protect it from scope creep elsewhere.

---

## Week 7 (Aug 24 – Aug 30) — High-Tier Economy and Squad Member

**Already scheduled:** Squad vote/host approval, fire-support strikes, `UW_ResupplyKitSwap`, and the entire Temporary Squad Member system (13.5 dev-days per `docs/WaveMission_DemoPlan/07_Temporary_Squad_Member.md` §11).

**Refactor work inserted this week:**

| Item | Type | Effort | Why this week |
|---|---|---|---|
| **H1b** — Extract shared `UHealthComponent` (if not already folded into Week 2's H1a pass) | High-value | 3 dev-days | Must land **before** `ATemporarySquadMember` writes its own replicated health, or this becomes a 3-way consolidation (player + zombie + squad member) instead of a 2-way one done once. If schedule is tight, this is the item to cut to Post-Demo Backlog — File 07 can ship with `ATemporarySquadMember` having its own independent health fields as a known, accepted duplication, per this file's own Section 7 risk-reduction guidance already prioritizing command-count over polish |

**Given Week 7 is already the single most schedule-risky week in the entire plan** (per `docs/WaveMission_DemoPlan/09_Two_Month_Schedule.md`'s own risk framing), **the recommendation is to defer H1b to the Post-Demo Backlog** rather than add 3 dev-days here. `ATemporarySquadMember` duplicating the health pattern a third time is an accepted, reversible cost — not a Blocking finding — precisely so it doesn't compete with Week 7's actual critical path (the BT command work).

---

## Week 8 (Aug 31 – Sep 6) — Integration Week

**No new refactor work scheduled.** Per `docs/WaveMission_DemoPlan/09_Two_Month_Schedule.md`, this week is reserved entirely for full-loop testing, bug fixing, and the acceptance pass — consistent with this audit's own approach of front-loading every Blocking item to land before, not during, integration testing.

**One verification task carries into this week's existing acceptance pass:** confirm during multiplayer testing (already scheduled) that B4's `EPlayerMissionStatus` correctly prevents `HandleMatchOver` from firing on an extracted player — this is testing, not new refactor work, and slots into the existing "2-client, physically separate networks" test pass already planned.

---

## Post-Demo Backlog

Nice-to-have items — explicitly deferred past the September 9, 2026 deadline. None of these gate any of the 10 Wave Mission systems; revisit after the demo ships.

- Merge `ImpactAudioComponent`/`ShellAudioComponent`'s duplicated surface-audio lookup into a shared interface or base class.
- Move `HitZoneComponent`'s bone-to-zone map and damage multipliers from hardcoded constructor values to a DataAsset, if per-character-type retuning is ever needed.
- Resolve the `Content/Animation/` vs. `Content/Animations/` folder naming collision.
- Wire up or remove `ItemComponent`'s unused `PickupMessage`.
- Investigate and fix `Weapon.h`'s dual ammo-state tracking (`InsertedMagazine` local vs. `ReplicatedMagState` replicated) for reorder-safety — reasonable to fold into a future networking-hardening pass rather than the demo window.
- Fix `ShooterHUD`/`QuestbookWidget`'s missing null-guards before delegate unbind (crash-on-teardown class, not exercised by normal play).
- Continue the Rider Inspect Code backlog `PROGRESS.md` explicitly paused mid-cleanup (categories #16, #29, #30, #31, #34 — const-correctness and unused-parameter sweeps across ~389 remaining instances). Orthogonal to Wave Mission Mode; resume on its own schedule.
- **`UQuestTrackerSubsystem`'s multiplayer-correctness gap** (flagged in Finding B6) — give vendor reputation the same player-keyed, server-RPC-routed treatment `PersonalPoints`/`SquadSharedFund` get in this plan. Not required for the Wave Mission demo (the vendor/reputation system is a separate game loop), but should not be forgotten indefinitely — tracked as its own future issue in `GITHUB_ISSUES_DRAFT.md`.
- If H1b (shared `UHealthComponent`) was deferred per the Week 7 recommendation above, revisit it once `ATemporarySquadMember` exists, consolidating all three (player/zombie/squad-member) health implementations in one pass.

---

## Summary — net schedule impact

| Week | Refactor dev-days added | Net vs. existing week budget |
|---|---|---|
| 1 | 1.75 | Low-risk absorb (Week 1 already light) |
| 2 | 4.0 (or 1.5 if H1a moves out) | Moderate — recommend moving H1a if Week 2 is tight |
| 3 | 0 | None |
| 4 | 0.25 | Negligible |
| 5 | 0.25 | Negligible |
| 6 | 0-5 (net-neutral to +5 depending on B7's resolution) | **Highest risk point — resolve B7 first thing Monday morning** |
| 7 | 0 (H1b deferred) | None |
| 8 | 0 | None |
| **Total** | **~6.25-11.25 dev-days**, most of it either net-neutral (Week 6, if B7 resolves toward reuse) or absorbed into already-light weeks (1, 4, 5) | Fits within the existing 8-week plan without moving the September 9 deadline, provided B7 is resolved in favor of extending `FLoadoutPreset` rather than building a parallel system |
