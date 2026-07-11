# 01 — Refactor Findings

Full findings from the codebase audit, cross-referenced against the 10 Wave Mission Mode systems in `docs/GameplayPlan.md`'s "Recommended Implementation Order" (Authored phase triggers → Phase 2 task system → Doomed zone logic → Field extraction points → Resupply window system → Basic call-in economy → Phase 4 exfil vehicle → Kit preset system → Fire support call-ins → Temporary squad member) and against `docs/WaveMission_DemoPlan/`'s file-by-file designs.

**Read [00_CODEBASE_INVENTORY.md](00_CODEBASE_INVENTORY.md) first** — every finding below cites specific files/lines cataloged there.

---

## Corrected assumptions (positive findings — not defects, but change scope of already-planned work)

Two places where the actual codebase is **more ready** than `docs/WaveMission_DemoPlan/` assumed. Read these before starting the corresponding files, since they reduce estimated effort:

1. **AI Perception hearing is already wired.** `AZombieAIController` already constructs `UAIPerceptionComponent` with `UAISenseConfig_Sight` **and** `UAISenseConfig_Hearing` (`Enemy/Character/ZombieAIController.h:70-76`). `docs/WaveMission_DemoPlan/02_Enemy_AI_and_Waves.md` §5.1 says to "confirm/add `UAIPerceptionComponent` (hearing sense) to `AZombieAIController` if not already present" as a Week 1 task — it's already present, drop that task.
2. **A generic "make noise" component already exists and is exactly what call-ins need.** `Components/AudioPerceptionComponent.h/.cpp` implements `Server_EmitSoundEvent`/`ReportNoiseEvent` with configurable radius/loudness, already replicated, already used by `Weapon::Fire()`. `docs/WaveMission_DemoPlan/05_Economy_and_Resupply.md` §3.6/3.7 describes ammo crates and distraction flares needing to "report an AI hearing stimulus" as new wiring — this should instead be a one-line call into the existing `UAudioPerceptionComponent` on the crate/flare actor, not new AI Perception plumbing.

---

## BLOCKING

Findings that would create rework or an awkward workaround if the corresponding Wave Mission system is started without addressing them first. Sorted by which system in GameplayPlan.md's implementation order they gate.

### B1 — No `AShooterGameState` exists (gates: Authored phase triggers)

**What's wrong:** `AShooterGameGameMode` (`Framework/GameMode/ShooterGameGameMode.h:13`) extends `AGameModeBase` directly. No `AGameStateBase` subclass exists anywhere in `Source/`. Confirmed via full-repo grep — zero hits for `GameState`, `EMissionPhase`, or any mission-phase enum.

**Why it matters:** Every field GitHub Issue #4 ("Mission State Machine and Phase Flow") and `docs/WaveMission_DemoPlan/01_Phase_Triggers_and_Map_Structure.md` need — `CurrentMissionPhase`, resupply window end-times, `SquadSharedFund`, the future exfil-vehicle countdown — needs a replicated home visible to every client. Building the state machine against `AShooterPlayerState` or a non-replicated singleton instead would work for a single client and silently fail to sync in any 2+ player session.

**What the refactor looks like:** Create `AShooterGameState : public AGameStateBase` in `Framework/GameState/`, register it via `AShooterGameGameMode`'s constructor (`GameStateClass = AShooterGameState::StaticClass()`). This is a pure addition — no existing class needs to change to accommodate it.

**Estimated effort:** 0.5 dev-days (already scoped as Week 1 Task 1 in `docs/WaveMission_DemoPlan/01_Phase_Triggers_and_Map_Structure.md` §8 — this finding confirms that task is correctly ordered first, not a new task).

**Risk of doing it:** None — new class, zero existing callers to break.

**Risk of not doing it:** Every subsequent Wave Mission system (all 10) has no correct place to read/write shared mission state; whichever system gets built first will improvise a wrong home for it (e.g., a `UGameInstanceSubsystem`, repeating the `UQuestTrackerSubsystem` anti-pattern in B7) that the next 9 systems then have to unwind.

---

### B2 — `AZombieSpawnManager`/`AZombieObjectiveArea` already implement a proto zone/wave system that GitHub Issue #5 says must be replaced (gates: Authored phase triggers, Doomed zone logic)

**What's wrong:** `Enemy/ZombieSpawnManager.h/.cpp` (481 lines) already implements: designer-placed spawn actors, `TriggerSpawn()`, a one-shot-gated `FireObjectiveSpawnEvent(TargetArea, Count, DelaySeconds)`, and — notably — a function literally named `ResetForNextWave(bool bClearTrackedZombies)`. `Enemy/ZombieObjectiveArea.h/.cpp` (160 lines) is a placed area marker with a radius that zombies are routed to (`GetRandomPointInRadius`, `RegisterRoutedZombies`) — structurally a simplified precursor to `AZoneDefinition` (has a radius and identity, lacks a state enum and per-phase spawn tables).

GitHub Issue #5's acceptance criteria is explicit: **"All ambient open-world AI spawners replaced with ZoneDefinition actors."** This is an official, already-filed instruction to replace the ambient-trigger model these two classes represent.

**Why it matters:** "Replaced" is correct at the *orchestration* level (phase-and-zone-keyed spawning must supersede designer-triggered one-offs) but the *spawn-execution primitives* inside `AZombieSpawnManager` are solid, already-tested code: nav-mesh-reachable-point resolution with a raw-offset fallback (`GetValidSpawnLocation`), dead-zombie purging (`PurgeDeadZombies`), and a retry-on-not-yet-possessed pattern for routing freshly-spawned zombies to a destination (`RouteZombiesToObjective`). If `docs/WaveMission_DemoPlan/02_Enemy_AI_and_Waves.md`'s `AWaveSpawnManager` is built as a fully separate class with its own spawn-point-resolution logic from scratch, this either duplicates ~150 lines of already-correct nav-mesh/retry logic, or worse, reintroduces bugs `AZombieSpawnManager` already solved (e.g., forgetting the controller-not-yet-possessed race that `RouteZombiesToObjective` explicitly guards against with a 0.1s retry timer).

**What the refactor looks like:** Before Week 2 of the demo plan begins, make an explicit decision (does not need to be elaborate — a one-paragraph design note is enough): either (a) extract `AZombieSpawnManager`'s private spawn-execution methods (`GetValidSpawnLocation`, `SpawnSingleZombie`, `PurgeDeadZombies`) into a reusable non-actor helper (e.g., a static library or a small `UZombieSpawnLibrary`) that both the legacy manager and the new `AWaveSpawnManager` call into, or (b) have `AWaveSpawnManager` compose an internal `AZombieSpawnManager` instance per zone and drive it via its existing public API (`TriggerSpawn`, `FireObjectiveSpawnEvent`, `ResetForNextWave`, `GetAliveZombieCount`) rather than reimplementing spawning. Option (b) is less code and reuses a function that already has the exact name (`ResetForNextWave`) the wave system needs.

**Estimated effort:** 1 dev-day (decision + a short design note) plus whatever `docs/WaveMission_DemoPlan/02_Enemy_AI_and_Waves.md` already budgets for `AWaveSpawnManager` — this finding does not add net new effort, it prevents ~2-3 dev-days of duplicated spawn-logic work during Week 2.

**Risk of doing it:** Low — this is a design decision plus, at most, extracting a few private methods to `public`/a shared header. No behavior change to existing zombie spawning.

**Risk of not doing it:** `AWaveSpawnManager` gets built as a parallel, disconnected system; nav-mesh spawn-point bugs already fixed once in `AZombieSpawnManager` get re-introduced; `AZombieObjectiveArea`'s existing placed-in-level instances become dead content that has to be manually migrated to `AZoneDefinition` later instead of one of them extending the other.

---

### B3 — No team/faction system beyond Player vs. Zombie; hardcoded `FGenericTeamId` literals (gates: Doomed zone logic's human AI reinforcements, Temporary squad member)

**What's wrong:** `AShooterGameCharacter::GetGenericTeamId()` returns a hardcoded `FGenericTeamId(0)` (`Player/Character/ShooterGameCharacter.h:48`). `AZombieAIController::GetGenericTeamId()` returns a hardcoded `FGenericTeamId(1)` (`Enemy/Character/ZombieAIController.h:26`). No `IGenericTeamAgentInterface` attitude solver exists — these are the only two team IDs in the entire codebase. Two separate TODO comments already flag this gap explicitly: `Items/Weapon/Projectile.cpp:102` ("team-vs-team comparison not implemented yet — no team system in place") and `Components/ReviveComponent.cpp:113` ("Team check goes here when team system is implemented").

**Why it matters:** GameplayPlan.md's wave escalation calls for "human AI reinforcements in later waves" (Phase 2) that must be hostile to players but should not be confused with zombies (zombies and human AI have no documented relationship — likely neutral to each other, both hostile to the player). Separately, `docs/WaveMission_DemoPlan/07_Temporary_Squad_Member.md` needs a player-outfitted AI soldier that reads as **friendly** to the player and player-controlled fire, and must not be targeted by other players' weapons or by zombies-vs-player logic that assumes "not team 0 = zombie." With only two hardcoded literal team IDs and no solver, a 3rd (human-enemy) and a would-be-4th (friendly-AI, which should probably just share team 0 with an "is AI" flag) faction cannot be added without touching both existing hardcoded return values and auditing every friend-or-foe check in the codebase for an implicit "team 1 = hostile" assumption.

**What the refactor looks like:** Introduce a small `ETeamID` enum (or keep using raw `uint8` values but centralize them as named constants, e.g., `namespace ShooterTeams { constexpr uint8 Players = 0; constexpr uint8 Zombies = 1; constexpr uint8 HumanEnemies = 2; }`) plus a shared `UGenericTeamIdSolver`-adjacent helper (a free function `GetTeamAttitude(uint8 TeamA, uint8 TeamB)` is sufficient — a full `IGenericTeamAgentInterface::GetTeamAttitude` override table) so all friend-or-foe logic (AI Perception's built-in team-attitude filtering, melee/damage checks) reads from one place instead of assuming binary team 0 vs. team 1.

**Estimated effort:** 1.5 dev-days — small in isolation, but must land before Human AI Reinforcement (`docs/WaveMission_DemoPlan/02_Enemy_AI_and_Waves.md` §3) or Temporary Squad Member (`docs/WaveMission_DemoPlan/07_Temporary_Squad_Member.md` §1) write any perception/targeting logic against the current two-team assumption.

**Risk of doing it:** Low — purely additive (new named constants, one new helper function); existing `FGenericTeamId(0)`/`FGenericTeamId(1)` return values are unchanged in value, just centralized.

**Risk of not doing it:** Human AI Reinforcement and Temporary Squad Member each independently invent their own ad-hoc team-ID scheme, likely conflicting (e.g., both picking `FGenericTeamId(2)` for different purposes), requiring a second refactor pass later that's harder because by then AI Perception configs, damage checks, and possibly UI (friendly-fire indicators) all encode the wrong assumption.

---

### B4 — `AShooterGameGameMode::AlivePlayers`/`HandleMatchOver` has no "extracted" player status (gates: Field extraction points)

**What's wrong:** `AShooterGameGameMode::PlayerDied` (`Framework/GameMode/ShooterGameGameMode.cpp:74-126`) removes a character from `AlivePlayers` and calls `HandleMatchOver()` once `AlivePlayers.Num() == 0`. There is no third state for "this player left the mission successfully (extracted), do not count them as available but also do not treat their absence as a loss condition."

**Why it matters:** `docs/WaveMission_DemoPlan/04_Extraction_and_Exfil.md` §6.3 already identifies this exact gap and proposes `EPlayerMissionStatus {Active, Extracted, LeftBehind}` — this finding **confirms that proposal against the actual current source** (the demo plan's assumption was correct, this is not new information, but it upgrades that section from "should probably check this" to "confirmed required, do not skip"). Without it, a squad of 2 where one player extracts early after Phase 1 would either (a) incorrectly trigger `HandleMatchOver` if the extraction path naively calls the same removal path as death, or (b) leave the extracted player's character/controller in an undefined state that's neither "alive and playing" nor "cleanly removed."

**What the refactor looks like:** Add `EPlayerMissionStatus` to `AShooterPlayerState` (or as a field tracked in `AShooterGameGameMode`), and extend `HandleMatchOver`'s trigger condition from "`AlivePlayers.Num() == 0`" to "all non-`Extracted` players are in `LeftBehind`-final or dead." Exact shape already specified in `docs/WaveMission_DemoPlan/08_Multiplayer_and_Networking.md` §5.1 — this finding is confirmation to build it as designed, not a new design task.

**Estimated effort:** Already budgeted (1.5 dev-days) in `docs/WaveMission_DemoPlan/08_Multiplayer_and_Networking.md` §10 — no change to that estimate.

**Risk of doing it:** Low-medium — touches `HandleMatchOver`'s trigger condition, which is currently simple and well-tested by inspection; needs a 2-client PIE test (one extracts, one plays to completion) before trusting it.

**Risk of not doing it:** Early extraction either ends the session for the remaining squad (violates GameplayPlan.md's explicit "solo extract does not end the session" rule) or leaves a zombie `APlayerController` connection in limbo.

---

### B5 — `EquippedStateComponent::Server_LootDeadPlayer` already exists and may conflict with File 04's "full gear loss" design (gates: Field extraction points)

**What's wrong:** `Inventory/EquippedStateComponent.h/.cpp` already implements `Server_LootDeadPlayer` — an existing mechanic for transferring a dead player's equipped gear to another player/container. `docs/WaveMission_DemoPlan/04_Extraction_and_Exfil.md` §6.2 designs left-behind-player death as gear simply being cleared ("full gear loss... `UInventoryComponent`/`ULoadoutComponent` are cleared") without referencing or reconciling this existing lootable-corpse mechanic.

**Why it matters:** If left-behind-player death both (a) leaves a corpse that other squad members can loot via the existing `Server_LootDeadPlayer` path and (b) simultaneously clears the dead player's own inventory per the new design, the two systems don't conflict functionally (looting happens on the corpse actor, clearing happens on the player's components) — but the *design intent* is ambiguous without an explicit decision: does GameplayPlan.md's "full gear loss" mean "gone forever" (no one gets it) or "lost to that player, but a squadmate can loot the body" (redistributed)? These produce different player experiences and the existing mechanic makes the second one nearly free to support if desired.

**What the refactor looks like:** Not a code refactor — a design decision to record in `docs/WaveMission_DemoPlan/04_Extraction_and_Exfil.md` §6.2 before implementation: either explicitly route left-behind-death gear loss through the existing `Server_LootDeadPlayer`-adjacent corpse-spawning path (cheaper — reuse existing mechanic, gear becomes squad-lootable) or explicitly clear inventory with no corpse loot (matches the demo plan's current literal wording, but ignores an existing, working mechanic that does something adjacent).

**Estimated effort:** 0.25 dev-days (design decision only) + implementation cost already budgeted in File 04's estimate, which does not change regardless of which option is chosen.

**Risk of doing it:** None — this is a decision, not a code change by itself.

**Risk of not doing it:** Two different engineers (or the same engineer at two different times) build inconsistent gear-loss behavior for left-behind death vs. any other death path that already uses `Server_LootDeadPlayer`, producing a game where death sometimes drops a lootable corpse and sometimes doesn't, with no player-facing explanation why.

---

### B6 — `UQuestTrackerSubsystem` is not a multiplayer-safe pattern to copy for the points economy (gates: Basic call-in economy)

**What's wrong:** `Framework/Subsystems/QuestTrackerSubsystem.h` is a `UGameInstanceSubsystem` holding flat, non-player-keyed `TArray<FQuestState>` fields with no player identity association. Its own header comment admits the gap: *"AUTHORITY MODEL: All state mutation functions are guarded by `HasAuthority()` on the outer world... When moving to a fully networked session, these mutations should be driven by server-side RPCs that call into this subsystem on the server only."* In its current form this subsystem's data is effectively per-machine, not per-player-in-a-session — every client and the server each have their own `UGameInstance`, so this data does not currently give a correct multiplayer picture of "who has how much reputation."

**Why it matters:** This is the only existing precedent in the codebase for "track a numeric value that changes over time and persists." `docs/WaveMission_DemoPlan/05_Economy_and_Resupply.md` was written *not* copying this pattern (it correctly proposes `PersonalPoints` on `AShooterPlayerState` and `SquadSharedFund` on `AShooterGameState`, both properly replicated) — this finding exists to make that choice explicit and on-the-record, so a future contributor skimming the codebase for "how do we track player stats around here" doesn't copy `UQuestTrackerSubsystem`'s subsystem-level flat-array pattern by default, since it's the more prominent/discoverable existing example.

**What the refactor looks like:** No change required to build `docs/WaveMission_DemoPlan/05_Economy_and_Resupply.md` as designed. Separately (out of scope for the Wave Mission demo, but worth its own future ticket): `UQuestTrackerSubsystem` itself will need the same fix (player-keyed state, server-RPC-routed mutation) before vendor reputation is meaningfully multiplayer-correct — flagged in `GITHUB_ISSUES_DRAFT.md` as a related-but-separate tech-debt item, not part of this plan's 10 systems.

**Estimated effort:** 0 dev-days against the Wave Mission plan (design already avoided the trap); 3-4 dev-days if/when `UQuestTrackerSubsystem` itself is fixed later (not scheduled in this plan).

**Risk of doing it:** N/A.

**Risk of not doing it (i.e., if this warning weren't recorded and someone copied the pattern anyway):** `PersonalPoints`/`SquadSharedFund` would silently be wrong in any 2+ player session — each client would compute/see different values with no error, which is a very expensive class of bug to catch late (exactly the "networking risk" class flagged in `docs/WaveMission_DemoPlan/08_Multiplayer_and_Networking.md` §6).

---

### B7 — `FLoadoutPreset` already exists and is structurally different from the demo plan's proposed `FKitPreset` (gates: Kit preset system)

**This is the single highest-value finding in this audit.**

**What's wrong:** `Types/ContainerTypes.h` already defines a complete, already-save-wired kit preset system:

```cpp
USTRUCT(BlueprintType)
struct FLoadoutPreset
{
    FString PresetName;
    TArray<FLoadoutPresetSlot> EquippedSlots;  // EEquipmentSlot + FGuid ItemInstanceID + bItemMissingFromStash
    TArray<FGuid> RigItemIDs;
    TArray<FGuid> BeltItemIDs;
    TArray<FGuid> BackpackItemIDs;
    // + InitializeDefaultSlots(), ClearAll(), HasAnyAssignedItems()
};
```

...and `UShooterSaveGameSubsystem` already exposes `SaveLoadoutPresets(const TArray<FLoadoutPreset>&)` / `LoadLoadoutPresets() const` as public, implemented API (`Framework/Subsystems/ShooterSaveGameSubsystem.h:103-108`).

`docs/WaveMission_DemoPlan/06_Kit_Presets_and_Loadout.md` §1.2 independently designs a **new, differently-shaped** `FKitPreset`/`FKitPresetItemEntry` struct, where each entry is a soft-reference to an item **Definition class** plus a quantity (a *template* that mints fresh `FItemInstance`s on apply).

These are two fundamentally different design philosophies for the same concept:
- **Existing (`FLoadoutPreset`):** references specific, already-owned item *instances* by GUID, pulled from a persistent stash. Applying a preset = "equip these specific stash items." If an item was sold/lost, `bItemMissingFromStash` flags it. This models a scarcity/ownership economy (consistent with the existing vendor/currency/stash system found throughout `ItemDefinition.h`, `QuestTrackerSubsystem`).
- **Proposed (`FKitPreset`):** references item *definitions* (classes), no ownership concept — applying a preset always mints exactly what's specified, regardless of what's "owned." This models a loadout-template system with no scarcity, which is more consistent with GameplayPlan.md's mission-loadout framing ("Players configure up to 6 named presets... weapons + attachments...") but does not obviously fit an ownership/stash economy that already exists elsewhere in the project.

**Why it matters:** Building `FKitPreset` as currently designed would create a second, parallel preset system alongside an already-functional, already-persisted one, with no shared code and two different mental models for what a "preset" means in this codebase. Whichever system a player interacts with first will set their expectations wrong for the other. This is exactly the kind of duplicate-system risk the task's "do not duplicate" instruction and the "High-value" category (extracting a shared interface before 3 more systems need it) both exist to catch — except here it's worse than usual because the duplicate already exists and is already wired to disk.

**What the refactor looks like:** Before any File 06 implementation work begins, make an explicit, recorded decision between two paths:

1. **Extend `FLoadoutPreset`** to be the Wave Mission kit preset system — add the missing pieces GameplayPlan.md needs that it doesn't currently have (a `ClassTag` field for Assaulter/Support/Heavy/Scout metadata; confirm whether the stash-ownership model is actually desired for mission loadouts, or whether it should be adapted to not require pre-owned stash items). This reuses the already-built save/load path (`SaveLoadoutPresets`/`LoadLoadoutPresets`) entirely as-is.
2. **Keep them deliberately separate**, with `FLoadoutPreset` remaining whatever pre-existing system it serves (possibly a different, non-Wave-Mission game loop — confirm what currently calls `SaveLoadoutPresets`/`LoadLoadoutPresets`, since if nothing does yet, it may itself be unfinished scaffolding rather than a live feature) and a new, distinctly-named struct (not `FKitPreset` if it risks confusion — consider `FMissionLoadoutPreset`) for Wave Mission specifically, with a one-paragraph comment in both files cross-referencing the other and explaining why two exist.

Either is workable; what's not workable is building File 06 without first checking which of these is true.

**Estimated effort:** 0.5 dev-days to research current callers of `FLoadoutPreset`/`SaveLoadoutPresets` and make the decision; the decision itself changes File 06's estimated effort — if path 1 is chosen, `docs/WaveMission_DemoPlan/06_Kit_Presets_and_Loadout.md`'s 17.5 dev-day estimate likely **drops by 3-5 dev-days** (persistence layer, "missing item" edge case handling, and the preset struct itself are already built).

**Risk of doing the research:** None — read-only investigation.

**Risk of not doing it:** Two parallel, incompatible "kit preset" concepts ship in the same game, doubling the persistence surface area, confusing players, and doubling the Week 6-7 testing burden in `docs/WaveMission_DemoPlan/09_Two_Month_Schedule.md`.

---

### B8 — Magazine shim blocks a clean Kit Preset "magazine loadout" snapshot (gates: Kit preset system)

**What's wrong:** `InventoryComponent`'s magazine storage is split across two parallel systems: the main `TArray<FItemInstance> Items` (clean, replicated, has a full `Server_Add/Remove/Transfer` API) and a legacy `TArray<FMagazine> MagazineSlots` (`AddMagazine`/`GetBestMagazine`/`RemoveMagazine`/`ReturnMagazine`/`ClearAllMagazines`) that CLAUDE.md already documents as "a legacy 'magazine shim' (FMagazine) being phased out — do not extend it."

**Why it matters:** GameplayPlan.md's own kit preset spec explicitly lists "magazine loadout" as one of the things a preset must capture ("weapons + attachments, armor tier, chest rig, **magazine loadout**, consumables"). A correct Kit Preset snapshot/restore therefore needs to touch `MagazineSlots`, which is precisely the system CLAUDE.md says not to extend. There is no way to build a complete Kit Preset system without either (a) extending the deprecated shim, directly contradicting existing project guidance, or (b) the ammo-to-`FItemInstance` migration CLAUDE.md already anticipates happening first.

**What the refactor looks like:** This finding does not require *this plan* to perform the ammo migration (that is a separate, larger effort not scoped in `docs/WaveMission_DemoPlan/`). It requires File 06 to explicitly acknowledge the conflict: either (a) scope Kit Presets' magazine-loadout snapshot as a small, isolated, clearly-labeled exception that touches `MagazineSlots` directly (accepting the shim will need updating again when the migration eventually happens), or (b) treat "magazine loadout" as out-of-scope for the demo's Kit Preset feature and only snapshot weapons/attachments/consumables via the clean `FItemInstance` path, deferring magazine-count-per-preset to after the ammo migration.

**Estimated effort:** 0.5 dev-days (scoping decision, recorded in File 06) + no change to File 06's implementation estimate if option (b) is chosen; +1-2 dev-days if option (a) is chosen (shim-touching code is simple, just explicitly temporary).

**Risk of doing it:** Low either way — this is a scope decision.

**Risk of not doing it:** Kit Preset implementation discovers this mid-Week-6 with no plan for it, and either silently ships presets that don't restore magazine counts (a quiet functionality gap) or extends the shim without anyone having decided that's acceptable.

---

### B9 — `CombatComponent::SpawnAndEquipWeaponFromSlot` only equips the first occupied loadout slot (gates: Kit preset system's mid-mission swap)

**What's wrong:** Per close reading of `Components/CombatComponent.cpp`'s loadout-apply path (`OnLoadoutUpdated` → `ApplyPendingLoadout` → `SpawnAndEquipWeaponFromSlot`), the slot-iteration loop exits after the **first** occupied slot it finds (a `break` rather than continuing through all `EEquipmentSlot` values), and the previously-equipped weapon is destroyed unconditionally before the new one is confirmed equipped.

**Why it matters:** `docs/WaveMission_DemoPlan/06_Kit_Presets_and_Loadout.md` §6 depends on `SetFullLoadout(...)` mid-mission correctly re-equipping **every** slot in a swapped-to preset (Primary + Secondary + Pistol + Tool + Knife + Consumables, potentially all changing at once). As currently written, a mid-mission kit swap would silently equip only whichever slot happens to be first in iteration order and leave the rest of the new preset's items un-equipped on the character, while the player's PlayerState-level `SavedLoadout` (see B10) would show the full correct preset — a visible mismatch between "what I selected" and "what my character is holding."

**What the refactor looks like:** `SpawnAndEquipWeaponFromSlot`'s caller needs to loop over all `EEquipmentSlot` values in the new `FLoadoutData` (not just handle the first occupied one), and weapon destruction/replacement should be sequenced so a new weapon is fully attached before the old one is torn down (or accept a one-frame gap if that's judged acceptable for a mid-mission swap, but this should be a decision, not an accident).

**Estimated effort:** 1.5-2 dev-days — this is a real bug fix in existing, currently-single-weapon-only-tested code, not a new feature. Should land in Week 6 immediately before or alongside `docs/WaveMission_DemoPlan/06_Kit_Presets_and_Loadout.md`'s mid-mission swap work (§6), since that section's acceptance criteria ("no leftover items from the previous preset") cannot pass without this fix.

**Risk of doing it:** Medium — this is the weapon-equip path every player already exercises at mission start (`PushLoadoutToCharacter` → `Server_SetFullLoadout`), so it needs a regression pass on normal mission-start equipping, not just kit-swap testing, after the fix.

**Risk of not doing it:** Kit Preset mid-mission swap (File 06's core feature) visibly fails its own acceptance criteria — the demo's Definition of Done item 5 in the Master Brief ("a player configures a kit preset... successfully swaps to a different stored preset during a resupply window") cannot be met.

---

### B10 — Two independently-replicated copies of loadout data must both be updated by Kit Preset swap (gates: Kit preset system)

**What's wrong:** `AShooterPlayerState::SavedLoadout` and `ULoadoutComponent::LoadoutData` are two separately-declared `Replicated`/`ReplicatedUsing` properties holding conceptually "the same" data, kept in sync only via explicit one-directional push calls (`PushLoadoutToCharacter`, `Server_SetFullLoadout`) — there is no single source of truth enforced at the type level. `AShooterPlayerState::OnRep_SavedLoadout`'s own comment confirms this is already anticipated for a future feature: *"This handles the case where loadout changes while in-game (e.g. future mid-mission loadout swap feature)"* — but its implementation only calls `LoadoutComp->BroadcastCurrentLoadout()`, which re-broadcasts whatever the component's *local* `LoadoutData` already holds; it does not copy `SavedLoadout` into `LoadoutData`.

**Why it matters:** `docs/WaveMission_DemoPlan/06_Kit_Presets_and_Loadout.md` §6.1 depends on squadmates seeing an accurate "currently equipped preset" via `AShooterPlayerState`-level data during resupply windows. If a kit swap only pushes through `ULoadoutComponent::Server_SetFullLoadout` (the character-local path) without also updating `AShooterPlayerState::SavedLoadout`, squadmates' summary view will show stale data. If it only updates `SavedLoadout` without pushing to the component, the character's actual equipped gear won't change.

**What the refactor looks like:** File 06's `Server_SwapToPreset` implementation must explicitly write both: call `AShooterPlayerState::SetFullLoadout(NewLoadout, /*bPushToCharacter=*/true)` (existing function, already does both steps in the right order per `ShooterPlayerState.cpp:50-64`) rather than reaching directly into `ULoadoutComponent` and skipping the PlayerState layer. This is achievable with the existing API — no new plumbing needed — but must be called correctly, which is worth stating explicitly since the two-copies structure makes it easy to update only one by accident.

**Estimated effort:** 0.5 dev-days (this is a "call the right existing function" fix, not new code) — fold into File 06 Week 7 implementation, not a separate task.

**Risk of doing it:** None — using an existing, already-correct function.

**Risk of not doing it:** A subtle, hard-to-notice-in-solo-testing desync between what a player's own screen shows and what squadmates see during resupply — exactly the class of bug `docs/WaveMission_DemoPlan/08_Multiplayer_and_Networking.md` flags as expensive to catch late.

---

## HIGH-VALUE

Not blocking, but fixing now saves significant future time.

### H1 — `AShooterGameCharacter` is accumulating responsibilities that belong in components

**What's wrong:** Despite CLAUDE.md's stated architecture ("Thin shell composing single-responsibility components"), `ShooterGameCharacter.h/.cpp` (580h / 1662cpp) directly owns: 12 camera/TPS tunables + shoulder-swap logic, prone state, sprint/stamina (7 fields + logic), replicated `Health`/`MaxHealth`/`OnRep_Health` (duplicated independently from `AZombieCharacter`'s near-identical health pattern), IK socket config, and ~300+ lines across 5-6 functions of interaction-trace/prompt-widget management (`FindBestInteractableInView`, `UpdateInteractFocus`, `ResolveBestInteractionCandidate`, `RefreshCurrentPromptWidget`, `ClearCurrentPromptWidget`).

**Why it matters:** `docs/WaveMission_DemoPlan/03_Objectives_and_Task_System.md` and `04_Extraction_and_Exfil.md` both add new `IInteractable` implementers (`AObjectiveTerminal`, `AFieldExtractionPoint`) that flow through this exact interaction-trace code. The call pattern itself is confirmed clean and generic (see the "Corrected assumptions" section is not the right place — this is confirmed via direct grep of `ShooterGameCharacter.cpp:675,748,954,962` — new Interactables need zero character-side changes to work), so this is **not blocking** File 03/04. But every one of the 10 Wave Mission systems that touches the character (Kit Presets re-equipping, Temporary Squad Member possibly needing character-adjacent camera/UI hooks) adds more surface area to an already-1662-line file with no unit-testable seams.

**What the refactor looks like:** Extract a `UInteractionComponent` (owns `FindBestInteractableInView`, focus/prompt-widget state) and consider a shared `UHealthComponent` (replicated `Health`/`MaxHealth`/`OnRep_Health`/death delegate) reused by both `AShooterGameCharacter` and `AZombieCharacter` — and, per `docs/WaveMission_DemoPlan/07_Temporary_Squad_Member.md`, by `ATemporarySquadMember` too, which would otherwise be a *third* independent copy of the same replicated-health pattern.

**Estimated effort:** `UInteractionComponent` extraction: 2.5 dev-days. Shared `UHealthComponent`: 3 dev-days (touches both existing character classes, needs a careful regression pass on damage/death since both `TakeDamage` overrides currently route through it independently).

**Risk of doing it:** Medium — `TakeDamage` and death-handling are safety-critical paths touched by every weapon and zombie melee attack; needs thorough PIE regression testing (both solo and 2-client) after extraction, not just a compile check.

**Risk of not doing it:** Not blocking, but each new system that needs "does this thing have health" (Temporary Squad Member, File 07) either copies the pattern a third time or awkwardly reaches into `AShooterGameCharacter`'s specific implementation.

**Best time to do this:** Before File 07 (Temporary Squad Member, Week 7) if doing the `UHealthComponent` extraction at all — doing it after `ATemporarySquadMember` already has its own copy means a 3-way consolidation instead of a 2-way one.

---

### H2 — `AZombieCharacter`/`AShooterGameCharacter` health duplication (see H1 — listed for cross-reference from the inventory table's per-file notes; not a separate task)

Folded into H1's `UHealthComponent` recommendation above.

---

### H3 — `WeaponConfig.h`'s legacy-alias drift should not be copied into new DataAssets

**What's wrong:** `Items/Weapon/WeaponConfig.h` (658 lines) is independently confirmed as a good structural template for future `UPrimaryDataAsset`s (`TSoftObjectPtr` refs, `EditDefaultsOnly`, extensive comments) — but it carries three parallel naming schemes for the same montage concepts accumulated across milestones (e.g., `TPFire`/`TP_Fire`/newer "Third Person" category names all referring to related-but-not-identical fields), which is exactly the kind of drift that accumulates when a DataAsset grows over many milestones without a consolidation pass.

**Why it matters:** `docs/WaveMission_DemoPlan/02_Enemy_AI_and_Waves.md`'s `UWaveSetDataAsset`, `05_Economy_and_Resupply.md`'s `UCallInDefinition`, and `06_Kit_Presets_and_Loadout.md`'s preset-adjacent structs are all new DataAssets being authored from scratch over the same 8-week window — exactly the kind of incremental growth that produced `WeaponConfig.h`'s drift, if the same "add a field, don't touch the old ones" habit is repeated without periodic review.

**What the refactor looks like:** Not a change to `WeaponConfig.h` itself (out of scope, working system, no Wave Mission dependency forces touching it). Instead: when authoring `UWaveSetDataAsset`/`UCallInDefinition`/similar, follow `WeaponConfig.h`'s good parts (soft references, EditDefaultsOnly, comments) and explicitly avoid its bad part (parallel naming schemes for the same concept) — call this out in `03_ARCHITECTURE_RECOMMENDATIONS.md`'s DataAsset standardization guidance.

**Estimated effort:** 0 dev-days as a standalone task — this is a stated convention to follow during File 02/05 work, not separate work.

**Risk of doing it:** N/A.

**Risk of not doing it:** By Week 8, 3-4 new DataAsset classes have each independently drifted, and the "standardize DataAssets" cleanup this finding is meant to prevent becomes its own late-cycle task instead of a zero-cost habit applied from Week 1.

---

### H4 — No custom log category exists project-wide

**What's wrong:** Every file uses `UE_LOG(LogTemp, ...)`. Zero `DEFINE_LOG_CATEGORY` declarations exist anywhere in `Source/`.

**Why it matters:** The 8-week plan explicitly relies on live-logging-driven tuning for several of its highest-risk areas (escalation curves, spawn rates, resupply timing, drop scheduling — see Master Brief Risk Register items 1, 2, 4). Debugging any of these against a shared, unfilterable `LogTemp` stream alongside every other engine/project log line makes that tuning workflow slower than it needs to be for no reason.

**What the refactor looks like:** Add one `DECLARE_LOG_CATEGORY_EXTERN(LogWaveMission, Log, All)` (or per-system categories: `LogMission`, `LogEconomy`, `LogWaveSpawn`) in a shared header, defined once in the module's main `.cpp`. Every new Wave Mission class (Files 01-08) uses it from the start; existing `LogTemp` call sites are not required to change (no forced migration).

**Estimated effort:** 0.5 dev-days (declare the category); adopted at zero marginal cost by every new class going forward since it's just a different first argument to `UE_LOG`.

**Risk of doing it:** None.

**Risk of not doing it:** Not blocking, but every Week 2-8 debugging session involving spawn rates or economy tuning is slightly slower than it needs to be, compounding across 6+ weeks of exactly the kind of iterative numeric tuning this project's own risk register identifies as high-risk.

---

### H5 — `ItemDefinition`'s existing physical-currency-item system may be confused with Wave Mission's abstract points economy

**What's wrong:** `Inventory/ItemDefinition.h` already has a fully-fleshed-out physical currency concept: `bIsCurrency`, `CurrencyValue`, `VendorBuyPrice`, `VendorSellPrice` (lines 213-278), documented with concrete examples ("DA_Currency_Rubles → CurrencyValue = 1... DA_Currency_Dollars → CurrencyValue = 120..."). `docs/WaveMission_DemoPlan/05_Economy_and_Resupply.md` proposes a separate, abstract `PersonalPoints`/`SquadSharedFund` integer economy with no relationship to carryable/lootable currency items.

**Why it matters:** These are legitimately different systems serving different purposes (session-scoped mission points vs. a persistent stash/vendor economy) and GameplayPlan.md's wording ("Points tracked per player and pooled") supports keeping them separate — but if this isn't a deliberate, documented decision, a future contributor (or player-facing UI/copy) could easily conflate "points" and "currency," especially since both are fundamentally "a number that goes up and can be spent."

**What the refactor looks like:** No code change required. Record the distinction explicitly — e.g., a one-line note in `docs/WaveMission_DemoPlan/05_Economy_and_Resupply.md` clarifying `PersonalPoints`/`SquadSharedFund` are mission-session-scoped and intentionally separate from the existing `bIsCurrency` physical-item economy — and ensure UI copy/naming (e.g., "Mission Points" vs. "Rubles/Dollars") makes the distinction legible to players, not just to code.

**Estimated effort:** 0.25 dev-days (documentation only).

**Risk of doing it:** None.

**Risk of not doing it:** Player-facing confusion between two different number-that-goes-up systems; low risk of an actual code defect, moderate risk of a UX/copy defect.

---

### H6 — `PlayerAnimInstance` appears to duplicate `ShooterAnimInstanceBase` ⚠ NEEDS RUNTIME VERIFICATION

**What's wrong:** `Player/Animation/PlayerAnimInstance.h/.cpp` (86h/110cpp) has near-identical property and method names to `ShooterAnimInstanceBase.h/.cpp` (the documented "Phase 1 Infima bridge" base class) — IK calculation, lean, locomotion booleans all appear twice with the same intent, no comments explaining why both exist.

**Why it matters:** Not a Wave Mission blocker directly (none of the 10 systems touch animation), but if `PlayerAnimInstance` is genuinely dead code (superseded by `ShooterAnimInstanceBase` per the Infima integration work), it's ~200 lines of maintenance burden and a source of confusion for anyone searching the codebase for "how does locomotion animation work" who finds two answers.

**⚠ NEEDS RUNTIME VERIFICATION:** Static file reading cannot determine whether any live Animation Blueprint (`ABP_FP_Default`, `ABP_TP_Default`, or any other AnimBP asset in `Content/Animation/` or `Content/Animations/`) currently has `PlayerAnimInstance` (or a Blueprint subclass of it) set as its parent class. Check via the UE5 editor's Reference Viewer on `PlayerAnimInstance`, or grep `Content/` `.uasset` binary for the class name (acceptable here since this is a reference *search*, not parsing structured data — or more simply, open each AnimBP in-editor and check its parent class in Class Settings) before deleting.

**What the refactor looks like:** If confirmed unused: delete `PlayerAnimInstance.h/.cpp` outright (CLAUDE.md's "no commented-out code" convention implies dead classes shouldn't linger either — use a branch, not a comment-out, per that same convention). If confirmed still in use by some AnimBP: rename one of the two classes to make the relationship explicit, or consolidate.

**Estimated effort:** 0.25 dev-days to verify; 0.5 dev-days to delete if confirmed dead; N/A if confirmed live (different investigation needed).

**Risk of doing it:** Low if verification is done first; high (silent animation breakage on whichever character/AnimBP loses its base class) if deleted without verification.

**Risk of not doing it:** Ongoing confusion, zero functional risk — this is why it's High-value rather than Blocking or urgent.

---

## NICE-TO-HAVE

General code quality improvements, no urgency, safe to defer past the 2-month demo window. Grouped rather than individually estimated in detail, since none gate any Wave Mission system.

| Item | File(s) | Note |
|---|---|---|
| Merge duplicated surface-audio lookup | `Components/ImpactAudioComponent.cpp`, `ShellAudioComponent.cpp` | Near-identical `GetCueForSurface`/`GetImpactCueForSurface`; candidate for a shared `ISurfaceAudioPlayer` interface or common base |
| `HitZoneComponent` config hardcoded in constructor | `Components/HitZoneComponent.cpp` | Bone-to-zone map and damage multipliers are C++ literals, not `EditAnywhere`/DataAsset — fine as-is for the demo, but blocks quick retuning without a rebuild if ever needed |
| `TestInteractableActor` sets `bReplicates=true` with no actual replicated state | `Interaction/TestInteractableActor.h/.cpp` | It's a test/reference actor, not shipped content — flagged in the inventory table as "do not copy this non-pattern," no fix required on the file itself |
| Dual ammo-state tracking (`InsertedMagazine` local `TOptional` + `ReplicatedMagState`) | `Items/Weapon/Weapon.h/.cpp` | Manually kept in sync; a desync-under-reorder risk worth a look during File 08's networking hardening pass, but not caused by and not blocking any Wave Mission system |
| Two separate top-level `Content/Animation/` and `Content/Animations/` folders | `Content/` | Naming collision risk for anyone placing new assets; a housekeeping cleanup, zero code impact |
| `ItemComponent` (`Items/Components/ItemComponent.h/.cpp`) is an unused placeholder | — | `PickupMessage` stored but never consumed by any system found in this audit; either wire it up or remove it |
| `QuestGiverInterface`/`QuestDefinition` design-time/runtime split | `Interaction/QuestGiverInterface.h`, `Quest/QuestDefinition.h` | Already a clean pattern — no action needed, noted only because it's a good example to point to when reviewing new DataAsset designs |
| Rider Inspect Code backlog (const-correctness, unused params) | Project-wide, ~389 remaining instances per `PROGRESS.md` | Already tracked and explicitly paused mid-cleanup by a prior session (see `PROGRESS.md`'s "STOPPING POINT"); orthogonal to this audit, do not re-scope it here |
| `ShooterHUD`/`QuestbookWidget` missing null-guards before delegate unbind | `HUD/ShooterHUD.cpp`, `HUD/Inventory/QuestbookWidget.cpp` | Minor crash-risk-on-teardown class of bug, not exercised by normal play; low priority |
