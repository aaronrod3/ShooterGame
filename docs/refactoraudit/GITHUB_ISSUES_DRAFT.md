# GitHub Issues — Draft

Draft issue text for every Blocking and High-value finding in [01_REFACTOR_FINDINGS.md](01_REFACTOR_FINDINGS.md), formatted to match the repo's real `.github/ISSUE_TEMPLATE/tech-task.yml` structure (Area / Target Milestone / What needs to be done / Why-Context / Done When) so each can be pasted directly into a new issue. Labels and milestones below are drawn from the repository's live label list (36 labels) and live milestone list (6 milestones) — no invented labels or milestones are used.

**Cross-reference note:** The repository currently has exactly 5 open issues, all epics: #1 Player Input System, #2 Health/Damage/PlayerState, #3 Weapon System, #4 Mission State Machine and Phase Flow, #5 Zone and Wave Spawn Orchestration. None of the items below duplicate any of these five — they are prerequisite/supporting tech-tasks that several of them (particularly #4 and #5) depend on. Where a draft below is a prerequisite for one of these epics, it says so explicitly under "Risks / Dependencies" using GitHub's `Blocks #N` convention (referencing another issue number in this way is standard GitHub linking usage, not a claim about `.github` config).

Nice-to-have findings are **not** drafted as issues here per the original task's scope (Blocking + High-value only) — they are tracked in `02_REFACTOR_SCHEDULE.md`'s Post-Demo Backlog section instead, and can be turned into issues later without urgency.

---

## Issue Draft 1 — Create `AShooterGameState`

**Title:** `[TECH] Create AShooterGameState — no GameState class currently exists`

**Labels:** `tech-debt`, `mission-flow`, `p0-critical`, `blocked`

**Area:** Code Architecture / Refactor

**Target Milestone:** First Playable Wave Loop

**What needs to be done**
Create `AShooterGameState : public AGameStateBase` in `Source/ShooterGame/Framework/GameState/`, and register it on `AShooterGameGameMode` via `GameStateClass = AShooterGameState::StaticClass()` in the constructor. Initial fields: `CurrentMissionPhase` (`ReplicatedUsing=OnRep_MissionPhase`), `ActiveWindowEndServerTime` (`ReplicatedUsing`), and an `FOnMissionPhaseChanged` `BlueprintAssignable` delegate.

**Why / Context**
`AShooterGameGameMode` currently extends `AGameModeBase` directly with no paired GameState class anywhere in the codebase (confirmed via full-repo grep for `GameState`). Every field the Mission State Machine (#4) and Zone Spawning (#5) epics need — mission phase, resupply window timing, shared economy state — needs a replicated home visible to every client, and none currently exists. This is the single highest-priority prerequisite in the entire Wave Mission Mode build; see `docs/RefactorAudit/01_REFACTOR_FINDINGS.md` Finding B1.

**Done When**
- [ ] `AShooterGameState` exists and is registered as the active GameStateClass
- [ ] `CurrentMissionPhase` and `ActiveWindowEndServerTime` replicate correctly and are verified identical on host and a joined PIE client
- [ ] `FOnMissionPhaseChanged` fires and is bindable from Blueprint

**Effort estimate:** XS — 0.5 dev-days

**Risk level:** Low — new class, zero existing callers to break

**Risks / Dependencies:** Blocks #4. Blocks #5 (zone state needs a replicated home too, most naturally on this class).

---

## Issue Draft 2 — Decide reuse-vs-rewrite for `AZombieSpawnManager`/`AZombieObjectiveArea` before building the new Wave Spawn Manager

**Title:** `[TECH] Decide whether AWaveSpawnManager reuses AZombieSpawnManager's spawn-execution primitives`

**Labels:** `tech-debt`, `ai`, `wave-spawning`, `p0-critical`, `design`

**Area:** AI / Behavior Tree

**Target Milestone:** First Playable Wave Loop

**What needs to be done**
Record a short design decision (one paragraph is sufficient) on whether the new zone/phase-keyed wave spawn manager (#5) reuses `AZombieSpawnManager`'s existing spawn-execution methods (`GetValidSpawnLocation` nav-mesh resolution, `SpawnSingleZombie`, `PurgeDeadZombies`, `RouteZombiesToObjective`'s possession-retry handling) rather than rebuilding equivalent logic from scratch. Recommended: have the new zone-based manager compose/drive an `AZombieSpawnManager` instance per zone via its existing public API (`TriggerSpawn`, `FireObjectiveSpawnEvent`, `ResetForNextWave`, `GetAliveZombieCount`) rather than duplicating spawn-point resolution.

**Why / Context**
GitHub Issue #5's acceptance criteria states "All ambient open-world AI spawners replaced with ZoneDefinition actors" — but `AZombieSpawnManager` (`Source/ShooterGame/Enemy/ZombieSpawnManager.h/.cpp`) already contains ~150 lines of working nav-mesh spawn-point resolution, dead-zombie purging, and a race-condition-aware retry pattern for routing freshly-spawned zombies (handles the case where a zombie's AIController hasn't finished possessing it yet). It already has a function literally named `ResetForNextWave()`. Rebuilding this from scratch for the new system risks reintroducing bugs this class already solved. See `docs/RefactorAudit/01_REFACTOR_FINDINGS.md` Finding B2.

**Done When**
- [ ] Decision recorded (reuse via composition, or extract shared helper, or deliberately rewrite — with reasoning either way) in `docs/WaveMission_DemoPlan/02_Enemy_AI_and_Waves.md`
- [ ] If reuse is chosen: `AZombieSpawnManager`'s relevant methods confirmed callable/composable from the new manager without modification, or a minimal extraction is scoped

**Effort estimate:** XS — 1 dev-day (decision + note)

**Risk level:** Low

**Risks / Dependencies:** Blocks #5. Must be resolved before Week 2 of `docs/WaveMission_DemoPlan/09_Two_Month_Schedule.md` begins building `AWaveSpawnManager`.

---

## Issue Draft 3 — Centralize team/faction identity (`FGenericTeamId`)

**Title:** `[TECH] Centralize team IDs — hardcoded FGenericTeamId(0)/(1) with no attitude solver`

**Labels:** `tech-debt`, `ai`, `combat`, `p1-high`

**Area:** AI / Behavior Tree

**Target Milestone:** First Playable Wave Loop

**What needs to be done**
Introduce named team-ID constants (e.g., `namespace ShooterTeams { constexpr uint8 Players = 0; constexpr uint8 Zombies = 1; constexpr uint8 HumanEnemies = 2; }`) and a small shared `GetTeamAttitude(uint8, uint8)` helper, replacing the two hardcoded literal returns in `AShooterGameCharacter::GetGenericTeamId()` (`Player/Character/ShooterGameCharacter.h:48`) and `AZombieAIController::GetGenericTeamId()` (`Enemy/Character/ZombieAIController.h:26`) with references to the new constants.

**Why / Context**
Only two team IDs exist in the whole codebase, hardcoded independently in two files, with no attitude-solver. Two separate TODO comments already flag this gap (`Items/Weapon/Projectile.cpp:102`, `Components/ReviveComponent.cpp:113`). Both the upcoming Human AI Reinforcement work (part of #5's wave escalation) and the Temporary Squad Member system need a third/fourth faction identity and will each independently invent one if this isn't centralized first. See `docs/RefactorAudit/01_REFACTOR_FINDINGS.md` Finding B3.

**Done When**
- [ ] Named team constants exist in a shared header
- [ ] Both existing `GetGenericTeamId()` overrides reference the new constants (no behavior change — same numeric values)
- [ ] `GetTeamAttitude` helper exists and is unit-testable/callable independent of any specific actor

**Effort estimate:** S — 1.5 dev-days

**Risk level:** Low — purely additive, no change to existing numeric team values

**Risks / Dependencies:** Blocks Human AI Reinforcement work under #5. Blocks the Temporary Squad Member system (Alpha — Feature Complete milestone).

---

## Issue Draft 4 — Add `EPlayerMissionStatus` to distinguish extracted players from dead players

**Title:** `[TECH] AlivePlayers/HandleMatchOver has no "extracted" player status`

**Labels:** `tech-debt`, `mission-flow`, `multiplayer`, `extraction`, `p0-critical`

**Area:** Networking / RPC

**Target Milestone:** Core Mission Systems

**What needs to be done**
Add an `EPlayerMissionStatus {Active, Extracted, LeftBehind}` field (on `AShooterPlayerState` or tracked in `AShooterGameGameMode`), and extend `AShooterGameGameMode::HandleMatchOver`'s trigger condition from "`AlivePlayers.Num() == 0`" to account for players who left the mission via extraction rather than death.

**Why / Context**
`AShooterGameGameMode::PlayerDied` (`Framework/GameMode/ShooterGameGameMode.cpp:74-126`) has exactly two player states today: alive (in `AlivePlayers`) or dead (removed, contributes to `HandleMatchOver` firing at zero). Early extraction (GameplayPlan.md: "solo extract does not end the session for remaining squadmates") needs a third state that removes a player from active-mission accounting without counting as a death. See `docs/RefactorAudit/01_REFACTOR_FINDINGS.md` Finding B4 and `docs/WaveMission_DemoPlan/08_Multiplayer_and_Networking.md` §5.1 (which independently specifies the same fix).

**Done When**
- [ ] `EPlayerMissionStatus` exists and is set correctly on extraction completion and on left-behind resolution
- [ ] `HandleMatchOver` does not fire when one player extracts while others remain `Active`
- [ ] Verified in a 2-client PIE session: one player extracts after Phase 1, the other continues the mission uninterrupted

**Effort estimate:** S — 1.5 dev-days

**Risk level:** Medium — touches `HandleMatchOver`'s trigger condition; needs 2-client regression testing, not just solo

**Risks / Dependencies:** Blocks Field Extraction Points (GameplayPlan.md implementation-order item 4).

---

## Issue Draft 5 — Decide gear-loss vs. lootable-corpse behavior for left-behind player death

**Title:** `[TECH] Reconcile Server_LootDeadPlayer with planned "full gear loss" on left-behind death`

**Labels:** `design`, `extraction`, `economy`, `p2-medium`

**Area:** Other

**Target Milestone:** Core Mission Systems

**What needs to be done**
Record a design decision: when a left-behind player dies (Phase 4 exfil missed), should their gear become lootable by squadmates via the existing `EquippedStateComponent::Server_LootDeadPlayer` mechanic, or should it be cleared outright with no corpse loot, per the current literal reading of GameplayPlan.md's "full gear loss"?

**Why / Context**
`Inventory/EquippedStateComponent.h/.cpp` already implements `Server_LootDeadPlayer` — a working lootable-corpse mechanic. The Wave Mission demo plan's extraction design (`docs/WaveMission_DemoPlan/04_Extraction_and_Exfil.md` §6.2) currently designs left-behind death as gear simply being cleared, without referencing this existing mechanic. Left undecided, this risks inconsistent gear-loss behavior across different death paths in the same game. See `docs/RefactorAudit/01_REFACTOR_FINDINGS.md` Finding B5.

**Done When**
- [ ] Decision recorded in `docs/WaveMission_DemoPlan/04_Extraction_and_Exfil.md` §6.2
- [ ] Implementation (whichever path is chosen) is consistent with any other death path already using `Server_LootDeadPlayer`

**Effort estimate:** XS — 0.25 dev-days (decision only; implementation cost already budgeted in File 04)

**Risk level:** Low

**Risks / Dependencies:** None blocking — informs Field Extraction Points implementation.

---

## Issue Draft 6 — Do not replicate `UQuestTrackerSubsystem`'s authority pattern into the new points economy

**Title:** `[TECH] UQuestTrackerSubsystem is not multiplayer-safe — do not use as a template for UScoringSubsystem`

**Labels:** `tech-debt`, `multiplayer`, `economy`, `p2-medium`

**Area:** Networking / RPC

**Target Milestone:** Core Mission Systems

**What needs to be done**
No code change required against the Wave Mission plan (it already avoids this pattern). This issue exists to put the warning on record and to separately track fixing `UQuestTrackerSubsystem` itself later: it is a `UGameInstanceSubsystem` holding flat, non-player-keyed arrays, with its own header comment admitting mutations aren't yet server-RPC-routed for a networked session. Confirm the new `UScoringSubsystem` (points economy) is built instead on properly-replicated `AShooterPlayerState`/`AShooterGameState` fields.

**Why / Context**
This is the only existing precedent in the codebase for "track a numeric value that changes over time." If a future contributor searches the codebase for a pattern to copy, this is the most discoverable existing example — and it's the wrong one to copy for anything that needs to be correct in a 2+ player session. See `docs/RefactorAudit/01_REFACTOR_FINDINGS.md` Finding B6.

**Done When**
- [ ] `UScoringSubsystem` confirmed built on `AShooterPlayerState`/`AShooterGameState`, not a `UGameInstanceSubsystem` flat-array pattern
- [ ] Separate follow-up issue filed (not part of this plan's 10 systems) to fix `UQuestTrackerSubsystem`'s own authority model — see Issue Draft 6b below

**Effort estimate:** XS — 0 dev-days against this plan (verification only)

**Risk level:** None

**Risks / Dependencies:** None blocking.

---

## Issue Draft 6b — (Post-demo, separate) Fix `UQuestTrackerSubsystem` player-keying and authority model

**Title:** `[TECH] UQuestTrackerSubsystem needs per-player keying and server-RPC-routed mutation for multiplayer correctness`

**Labels:** `tech-debt`, `multiplayer`, `blocked`

**Area:** Networking / RPC

**Target Milestone:** Beta — Content and Balance

**What needs to be done**
Rework `UQuestTrackerSubsystem`'s quest/reputation state to be keyed per-player (not a flat `UGameInstanceSubsystem`-wide array) and route all mutations through server-side RPCs, per the gap the class's own header comment already documents.

**Why / Context**
Not required for the Wave Mission Mode demo (this subsystem serves a separate vendor/reputation game loop), but the gap is real and will eventually need fixing for that loop to be correct in multiplayer. Filed here so it isn't forgotten. See `docs/RefactorAudit/01_REFACTOR_FINDINGS.md` Finding B6 and `docs/RefactorAudit/02_REFACTOR_SCHEDULE.md`'s Post-Demo Backlog.

**Done When**
- [ ] Quest/reputation state is keyed per-player
- [ ] All mutation entry points are server-RPC-routed
- [ ] Verified correct in a 2-client PIE session (two players have independently correct reputation values)

**Effort estimate:** M — 3-4 dev-days

**Risk level:** Medium

**Risks / Dependencies:** Explicitly deferred past the Wave Mission demo window (targeted at Beta milestone, not First Playable Wave Loop/Core Mission Systems).

---

## Issue Draft 7 — Resolve `FLoadoutPreset` vs. the proposed Kit Preset design before implementation starts

**Title:** `[TECH] FLoadoutPreset already exists and is structurally different from the proposed Kit Preset system — decide before building`

**Labels:** `tech-debt`, `loadout`, `design`, `p0-critical`, `blocked`

**Area:** Save / Load / Persistence

**Target Milestone:** Alpha — Feature Complete

**What needs to be done**
Investigate current callers (if any) of `FLoadoutPreset`/`UShooterSaveGameSubsystem::SaveLoadoutPresets`/`LoadLoadoutPresets`, then make and record an explicit decision: extend `FLoadoutPreset` (`Types/ContainerTypes.h`) into the Wave Mission Kit Preset system (reusing its already-built save/load path), or deliberately build a separate, distinctly-named struct with a documented cross-reference explaining why two preset systems coexist.

**Why / Context**
`Types/ContainerTypes.h` already defines a complete, already-save-wired `FLoadoutPreset`/`FLoadoutPresetSlot` system (named presets referencing items by InstanceID from a persistent stash, with a `bItemMissingFromStash` edge-case flag already handled), and `UShooterSaveGameSubsystem` already exposes working `SaveLoadoutPresets`/`LoadLoadoutPresets` methods. `docs/WaveMission_DemoPlan/06_Kit_Presets_and_Loadout.md` independently designs a new, differently-shaped `FKitPreset` (item-Definition-template-based, no stash-ownership concept) without referencing this existing system. Building both would create two parallel, incompatible "kit preset" concepts in the same game. **This is the highest-value finding in the full refactor audit** — see `docs/RefactorAudit/01_REFACTOR_FINDINGS.md` Finding B7 for full detail.

**Done When**
- [ ] Current callers of `FLoadoutPreset`/`SaveLoadoutPresets`/`LoadLoadoutPresets` identified
- [ ] Decision recorded in `docs/WaveMission_DemoPlan/06_Kit_Presets_and_Loadout.md` §1.2
- [ ] If extending: `FLoadoutPreset` gains whatever Wave-Mission-specific fields are missing (e.g., `ClassTag`); persistence layer work in File 06 is descoped accordingly

**Effort estimate:** S — 0.5 dev-days research/decision (changes downstream Kit Preset implementation effort by an estimated -3 to -5 dev-days if extension is chosen)

**Risk level:** Low (research), but high-consequence if skipped

**Risks / Dependencies:** Blocks Kit Preset System (GameplayPlan.md implementation-order item 8). Must resolve before Week 6 of `docs/WaveMission_DemoPlan/09_Two_Month_Schedule.md`.

---

## Issue Draft 8 — Scope the magazine-shim conflict for Kit Preset magazine-loadout snapshots

**Title:** `[TECH] Kit Preset "magazine loadout" snapshot conflicts with the deprecated magazine shim`

**Labels:** `tech-debt`, `loadout`, `weapon`, `p1-high`

**Area:** Save / Load / Persistence

**Target Milestone:** Alpha — Feature Complete

**What needs to be done**
Decide whether Kit Presets' "magazine loadout" field (per GameplayPlan.md's kit preset spec) snapshots `InventoryComponent`'s legacy `MagazineSlots`/`FMagazine` shim directly (small, explicitly-temporary exception to CLAUDE.md's "do not extend it" guidance), or is scoped out of the demo's Kit Preset feature until the ammo-to-`FItemInstance` migration happens.

**Why / Context**
CLAUDE.md already documents `InventoryComponent`'s `FMagazine`/`MagazineSlots` as "a legacy 'magazine shim'... being phased out — do not extend it." GameplayPlan.md's kit preset spec explicitly requires capturing "magazine loadout" as part of a preset. These two requirements directly conflict without a scoping decision. See `docs/RefactorAudit/01_REFACTOR_FINDINGS.md` Finding B8.

**Done When**
- [ ] Decision recorded in `docs/WaveMission_DemoPlan/06_Kit_Presets_and_Loadout.md`
- [ ] If scoped out: documented as a known demo limitation, not a silent gap
- [ ] If included: shim-touching code explicitly commented as temporary, pending the ammo migration

**Effort estimate:** XS — 0.5 dev-days decision; +1-2 dev-days implementation only if the shim-touching option is chosen

**Risk level:** Low

**Risks / Dependencies:** Blocks Kit Preset System's magazine-loadout feature specifically (not the whole system).

---

## Issue Draft 9 — Fix `SpawnAndEquipWeaponFromSlot` to equip all occupied loadout slots, not just the first

**Title:** `[BUG] CombatComponent::SpawnAndEquipWeaponFromSlot only equips the first occupied slot`

**Labels:** `bug`, `weapon`, `loadout`, `p1-high`

**Area:** Weapon System

**Target Milestone:** Alpha — Feature Complete

**What needs to be done**
Fix the loadout-apply path (`OnLoadoutUpdated` → `ApplyPendingLoadout` → `SpawnAndEquipWeaponFromSlot` in `Components/CombatComponent.cpp`) to loop over every occupied `EEquipmentSlot` in the applied `FLoadoutData`, not exit after the first one found. Sequence weapon destruction/replacement so a new weapon is confirmed equipped before the old one is torn down.

**Why / Context**
Confirmed via close reading of `CombatComponent.cpp`'s loadout-apply path: the slot-iteration loop currently `break`s after the first occupied slot. A full mid-mission Kit Preset swap (potentially changing Primary + Secondary + Pistol + Tool + Knife + Consumables simultaneously) would silently equip only one of those and leave the rest stale — while `AShooterPlayerState::SavedLoadout` would show the full, correct preset, producing a visible mismatch between what the player selected and what their character actually holds. See `docs/RefactorAudit/01_REFACTOR_FINDINGS.md` Finding B9.

**Done When**
- [ ] All occupied slots in an applied `FLoadoutData` are correctly equipped, not just the first
- [ ] Existing mission-start loadout-apply flow (`PushLoadoutToCharacter` → `Server_SetFullLoadout`) regression-tested — this fix touches a path every player already exercises at spawn, not just kit-swap
- [ ] Verified with a preset that changes 3+ slots simultaneously during a resupply window

**Effort estimate:** S — 1.5-2 dev-days

**Risk level:** Medium — touches the weapon-equip path used by every player at every mission start

**Risks / Dependencies:** Blocks Kit Preset System's mid-mission swap acceptance criteria. Should land before or alongside the mid-mission swap UI/RPC work.

---

## Issue Draft 10 — Ensure Kit Preset swap updates both replicated loadout copies

**Title:** `[TECH] Server_SwapToPreset must update both AShooterPlayerState::SavedLoadout and ULoadoutComponent::LoadoutData`

**Labels:** `tech-debt`, `loadout`, `multiplayer`, `p2-medium`

**Area:** Networking / RPC

**Target Milestone:** Alpha — Feature Complete

**What needs to be done**
Implement the future `Server_SwapToPreset` (Kit Preset mid-mission swap) using the existing `AShooterPlayerState::SetFullLoadout(NewLoadout, /*bPushToCharacter=*/true)` function, which already correctly updates both `SavedLoadout` (PlayerState) and pushes to `ULoadoutComponent::LoadoutData` (character) in the right order — rather than writing directly to only one of the two.

**Why / Context**
`AShooterPlayerState::SavedLoadout` and `ULoadoutComponent::LoadoutData` are two independently-replicated properties kept in sync only by explicit push calls, not enforced as one source of truth at the type level. Squadmate-visible "currently equipped preset" summaries (per `docs/WaveMission_DemoPlan/06_Kit_Presets_and_Loadout.md` §6.1) read from the PlayerState layer; actual equipped gear lives on the component. Updating only one produces a silent desync between what a player's own screen shows and what squadmates see. See `docs/RefactorAudit/01_REFACTOR_FINDINGS.md` Finding B10.

**Done When**
- [ ] `Server_SwapToPreset` calls `SetFullLoadout(..., true)` (or equivalent that updates both copies) rather than reaching into `ULoadoutComponent` directly
- [ ] Verified in a 2-client session: squadmate's preset summary view matches the swapping player's actual equipped gear immediately after swap

**Effort estimate:** XS — 0.5 dev-days

**Risk level:** Low — uses an existing, already-correct function

**Risks / Dependencies:** Fold into the same implementation task as Kit Preset mid-mission swap; not a separately-schedulable item.

---

## Issue Draft 11 — Extract `UInteractionComponent` from `ShooterGameCharacter`

**Title:** `[TECH] Extract interaction-trace/prompt-widget logic into UInteractionComponent`

**Labels:** `tech-debt`, `player-input`, `p2-medium`

**Area:** Code Architecture / Refactor

**Target Milestone:** First Playable Wave Loop

**What needs to be done**
Extract `FindBestInteractableInView`, `UpdateInteractFocus`, `ResolveBestInteractionCandidate`, `RefreshCurrentPromptWidget`, `ClearCurrentPromptWidget`, and related fields (`FocusedInteractableActor`, `CurrentPromptActor`, `InteractPromptWidgetInstance`) from `AShooterGameCharacter` into a new `UInteractionComponent`.

**Why / Context**
`ShooterGameCharacter.h/.cpp` (580h/1662cpp) directly owns ~300+ lines of interaction-trace and prompt-widget logic despite CLAUDE.md describing the character as a "thin shell composing single-responsibility components." The `IInteractable` interface call pattern itself is confirmed clean and generic (no character-side changes needed for new implementers) — this is not blocking, but two new `IInteractable` consumers (`AObjectiveTerminal`, `AFieldExtractionPoint`) are about to be added within the next 3 weeks, and doing the extraction first means both are built against a clean, testable component instead of more inline character code. See `docs/RefactorAudit/01_REFACTOR_FINDINGS.md` Finding H1.

**Done When**
- [ ] `UInteractionComponent` exists and owns all interaction-trace/prompt state
- [ ] `AShooterGameCharacter` delegates to it; existing interaction behavior (picking up weapons, ammo, `TestInteractableActor`) regression-tested with no behavior change
- [ ] New `IInteractable` implementers can be added with zero `AShooterGameCharacter` changes (already true today, confirm it remains true post-extraction)

**Effort estimate:** S — 2.5 dev-days

**Risk level:** Low-medium — refactor-only, no new behavior; needs a PIE pass covering every existing interactable type

**Risks / Dependencies:** None blocking — recommended before Week 3 (Objective Terminal) for maximum benefit, but not required.

---

## Issue Draft 12 — Extract shared `UHealthComponent` for player/zombie/(future) squad-member health

**Title:** `[TECH] Extract UHealthComponent — Health/MaxHealth/OnRep_Health duplicated between Character and Zombie`

**Labels:** `tech-debt`, `combat`, `temp-squad-member`, `p3-low`

**Area:** Health / Damage System

**Target Milestone:** Alpha — Feature Complete

**What needs to be done**
Extract the replicated `Health`/`MaxHealth`/`OnRep_Health` pattern (currently independently implemented in both `AShooterGameCharacter` and `AZombieCharacter`) into a shared `UHealthComponent` with a death delegate, reused by both existing classes and by the upcoming `ATemporarySquadMember`.

**Why / Context**
Without this, `ATemporarySquadMember` (Temporary Squad Member system) would be a *third* independent copy of the same replicated-health pattern. See `docs/RefactorAudit/01_REFACTOR_FINDINGS.md` Finding H1/H2.

**Done When**
- [ ] `UHealthComponent` exists, used by `AShooterGameCharacter` and `AZombieCharacter` with no behavior change
- [ ] `TakeDamage` regression-tested for both classes (solo and 2-client PIE) — this touches a safety-critical path exercised by every weapon and melee attack
- [ ] `ATemporarySquadMember` (when built) uses the shared component from the start

**Effort estimate:** M — 3 dev-days

**Risk level:** Medium — touches `TakeDamage`/death handling for both existing character types

**Risks / Dependencies:** **Recommended to defer to Post-Demo Backlog per `docs/RefactorAudit/02_REFACTOR_SCHEDULE.md`** — Week 7 (Temporary Squad Member week) is already the highest schedule-risk week in the plan; `ATemporarySquadMember` shipping with its own independent health fields as a known, accepted duplication is preferable to adding 3 dev-days to that week. Revisit after the demo ships.

---

## Issue Draft 13 — Declare a project log category

**Title:** `[TECH] Add LogWaveMission log category — project uses LogTemp everywhere`

**Labels:** `tech-debt`, `p3-low`

**Area:** Build / CI

**Target Milestone:** First Playable Wave Loop

**What needs to be done**
Add `DECLARE_LOG_CATEGORY_EXTERN(LogWaveMission, Log, All)` (or per-system categories) in a shared header, defined once in the module's main `.cpp`. Adopt it in every new Wave Mission class going forward; no forced migration of existing `LogTemp` call sites required.

**Why / Context**
Zero custom log categories exist anywhere in the project — every file uses `UE_LOG(LogTemp, ...)`. The 8-week Wave Mission plan relies heavily on live-logging-driven tuning for its highest-risk areas (escalation curves, spawn rates, resupply timing). A filterable log category makes that iteration loop meaningfully faster. See `docs/RefactorAudit/01_REFACTOR_FINDINGS.md` Finding H4.

**Done When**
- [ ] `LogWaveMission` (or equivalent) category declared and available project-wide
- [ ] New Wave Mission classes use it from creation

**Effort estimate:** XS — 0.5 dev-days

**Risk level:** None

**Risks / Dependencies:** None — do this in Week 1 for maximum benefit at minimum cost.

---

## Issue Draft 14 — Clarify `PersonalPoints`/`SquadSharedFund` vs. existing physical-currency-item economy

**Title:** `[TECH] Document that Mission Points are intentionally separate from the existing bIsCurrency item economy`

**Labels:** `design`, `economy`, `ui`, `p3-low`

**Area:** Other

**Target Milestone:** Core Mission Systems

**What needs to be done**
Add a short clarifying note (in `docs/WaveMission_DemoPlan/05_Economy_and_Resupply.md` and in the resupply UI's naming/copy) confirming `PersonalPoints`/`SquadSharedFund` are session-scoped mission currency, intentionally distinct from `ItemDefinition`'s existing `bIsCurrency`/`CurrencyValue`/vendor-pricing physical-item economy (e.g., "Rubles"/"Dollars"). Use visibly distinct naming in UI (e.g., "Mission Points" vs. a physical currency name) so the distinction is legible to players, not just to code.

**Why / Context**
`Inventory/ItemDefinition.h` already has a fully-built physical currency concept with concrete documented examples. Both systems are fundamentally "a number that goes up and can be spent," which risks player and future-contributor confusion if the distinction isn't made explicit. See `docs/RefactorAudit/01_REFACTOR_FINDINGS.md` Finding H5.

**Done When**
- [ ] Clarifying note added to `docs/WaveMission_DemoPlan/05_Economy_and_Resupply.md`
- [ ] Resupply UI uses visibly distinct naming for mission points vs. any physical currency

**Effort estimate:** XS — 0.25 dev-days

**Risk level:** None

**Risks / Dependencies:** None.

---

## Issue Draft 15 — Verify whether `PlayerAnimInstance` is dead code

**Title:** `[TECH] Verify PlayerAnimInstance usage — appears to duplicate ShooterAnimInstanceBase`

**Labels:** `tech-debt`, `animation`, `needs-design`

**Area:** Animation / ABP

**Target Milestone:** Pre-Production Refresh

**What needs to be done**
Check (via the UE5 editor's Reference Viewer, or by opening each AnimBP asset's Class Settings) whether any live Animation Blueprint currently has `PlayerAnimInstance` (or a Blueprint subclass of it) set as its parent class. If confirmed unused, delete `Player/Animation/PlayerAnimInstance.h/.cpp` via a branch (per CLAUDE.md's "no commented-out code" convention extended to dead classes). If confirmed still in use, rename one of the two classes or consolidate.

**Why / Context**
`PlayerAnimInstance.h/.cpp` (86h/110cpp) has near-identical property/method names to `ShooterAnimInstanceBase.h/.cpp` (the documented "Phase 1 Infima bridge" base class) with no comment explaining why both exist. Static file reading cannot determine which Blueprints reference it. **⚠ NEEDS RUNTIME VERIFICATION** — see `docs/RefactorAudit/01_REFACTOR_FINDINGS.md` Finding H6.

**Done When**
- [ ] Reference Viewer / Class Settings check completed and documented
- [ ] If dead: class deleted via a branch, not commented out
- [ ] If live: relationship to `ShooterAnimInstanceBase` documented or classes consolidated

**Effort estimate:** XS — 0.25 dev-days to verify; +0.5 dev-days if deletion is warranted

**Risk level:** Low if verified first; high (silent animation breakage) if deleted without verification

**Risks / Dependencies:** None — zero Wave Mission Mode dependency, safe to do anytime, cheapest during Week 1 while other editor work is already happening.

---

## Summary table

| # | Title | Labels (primary) | Milestone | Effort | Blocking? |
|---|---|---|---|---|---|
| 1 | Create `AShooterGameState` | tech-debt, p0-critical | First Playable Wave Loop | 0.5d | Yes — blocks #4, #5 |
| 2 | `AZombieSpawnManager` reuse decision | tech-debt, ai, p0-critical | First Playable Wave Loop | 1d | Yes — blocks #5 |
| 3 | Centralize team IDs | tech-debt, ai, p1-high | First Playable Wave Loop | 1.5d | Yes — blocks Human AI + Squad Member |
| 4 | `EPlayerMissionStatus` | tech-debt, multiplayer, p0-critical | Core Mission Systems | 1.5d | Yes — blocks Extraction |
| 5 | Gear-loss vs. lootable-corpse decision | design, p2-medium | Core Mission Systems | 0.25d | Yes (decision) |
| 6 | Don't copy `UQuestTrackerSubsystem` pattern | tech-debt, p2-medium | Core Mission Systems | 0d | Warning only |
| 6b | Fix `UQuestTrackerSubsystem` (post-demo) | tech-debt, blocked | Beta | 3-4d | No — deferred |
| 7 | `FLoadoutPreset` vs. `FKitPreset` decision | tech-debt, loadout, p0-critical | Alpha — Feature Complete | 0.5d | **Yes — highest-value finding** |
| 8 | Magazine shim scoping decision | tech-debt, loadout, p1-high | Alpha — Feature Complete | 0.5d | Yes — blocks Kit Presets |
| 9 | Fix `SpawnAndEquipWeaponFromSlot` | bug, weapon, p1-high | Alpha — Feature Complete | 1.5-2d | Yes — blocks Kit Preset swap |
| 10 | Dual loadout replication fix | tech-debt, multiplayer, p2-medium | Alpha — Feature Complete | 0.5d | Yes — fold into #9's task |
| 11 | Extract `UInteractionComponent` | tech-debt, player-input, p2-medium | First Playable Wave Loop | 2.5d | No — High-value |
| 12 | Extract `UHealthComponent` | tech-debt, combat, p3-low | Alpha — Feature Complete | 3d | No — recommend deferring to Post-Demo |
| 13 | Add log category | tech-debt, p3-low | First Playable Wave Loop | 0.5d | No — High-value |
| 14 | Points vs. currency naming | design, economy, p3-low | Core Mission Systems | 0.25d | No — High-value |
| 15 | Verify `PlayerAnimInstance` | tech-debt, animation | Pre-Production Refresh | 0.25-0.75d | No — High-value |
