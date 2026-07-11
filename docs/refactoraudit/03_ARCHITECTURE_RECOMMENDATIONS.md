# 03 — Architecture Recommendations

Structural recommendations that go beyond individual class refactors — decisions worth making once, deliberately, before several upcoming systems each independently run into the same question. Every recommendation here references the relevant `CLAUDE.md` section it extends. **None of the recommendations in this file propose changing a documented `CLAUDE.md` architecture rule** — see the confirmation note at the end. Where a recommendation touches something CLAUDE.md explicitly governs (the tag-update rule, the replication pattern, the DataAsset convention), it is applying that existing rule to new territory, not amending it.

---

## R1 — Reconcile the three overlapping "what does a player have equipped" representations before Kit Presets makes it four

**The observation:** This audit found the codebase already has **three** distinct, only loosely-connected representations of "a player's equipment":

1. **`FLoadoutData`/`FLoadoutSlot`** (`Types/LoadoutTypes.h`) — 8 fixed, indexed slots (Primary/Secondary/Pistol/Tool/Knife/Consumable1/Consumable2/SpecialItem), weapon-and-tool-focused, soft-referenced by item *class*. Owned by `ULoadoutComponent` and mirrored on `AShooterPlayerState::SavedLoadout`. This is what CLAUDE.md's "Item / Loadout / Inventory" section documents.
2. **`FEquippedStateSnapshot`/`FNamedEquippedItem`/`FEquippedContainerState`** (`Types/ContainerTypes.h`) — a richer, save-oriented model with named body slots (Primary/Secondary/Sidearm/Helmet/Chest/Tool/Knife) *plus* multi-item Rig/Belt/Backpack containers, weight/burden tracking, referenced by item *instance GUID*. Owned by `UEquippedStateComponent`, persisted via `UShooterSaveGameSubsystem::SaveEquippedState`/`LoadEquippedState`.
3. **`UInventoryComponent::Items`** (`TArray<FItemInstance>`) — the live, tag-gated, in-mission item container, the actual server-authoritative source of what's physically in a player's possession right now.

These are not documented anywhere as three faces of one coherent model — they appear to have grown independently across milestones (Loadout for the original weapon-equip flow, EquippedState for a later carried-gear/burden system, Inventory for the tag-gated container work). `FLoadoutPreset` (also in `ContainerTypes.h`, see Finding B7 in `01_REFACTOR_FINDINGS.md`) references *both* named slots (like #1) *and* GUID-based container item lists (like #2/#3), meaning it already straddles all three.

**Why it matters now, specifically:** Kit Presets (`docs/WaveMission_DemoPlan/06_Kit_Presets_and_Loadout.md`) is the first Wave Mission system that needs to snapshot and restore a *complete* equipped state — weapons, attachments, armor, chest rig, magazines, consumables — in one operation. That system sits at the exact intersection of all three representations. Building it without first deciding "which of these three is the actual source of truth for a mission loadout, and how do the other two relate to it" risks Kit Presets becoming a *fourth* parallel representation, each partially overlapping the other three, none of them fully authoritative.

**Recommendation:** Before Week 6 begins (this is the same moment Finding B7 already requires a decision), produce a one-page internal note answering: for a Wave Mission loadout, is `ULoadoutComponent`/`FLoadoutData` the source of truth for weapons/tools (with `UInventoryComponent` handling everything else via tag-gated slots), with `FEquippedStateSnapshot`/`UEquippedStateComponent` remaining scoped to whatever non-Wave-Mission game loop it currently serves (confirm what that is — likely the persistent stash/extraction-shooter loop implied by `bIsCurrency`/vendor pricing/`Server_LootDeadPlayer`)? This audit's read of the code suggests **yes** — `FLoadoutData` (weapons) + `UInventoryComponent` (everything else, via `Slot.Rig`/`Slot.Backpack`/`Slot.Belt`/`Slot.MedPouch` tags) is the cleaner pairing for a mission-loadout preset, leaving `FEquippedStateSnapshot` as a separate, pre-existing system this plan doesn't need to touch. But this should be a deliberate decision recorded in `docs/WaveMission_DemoPlan/06_Kit_Presets_and_Loadout.md`, not an implicit one discovered mid-implementation.

**Does this require a `CLAUDE.md` rule change?** No — CLAUDE.md's Item/Loadout/Inventory section already documents `FItemInstance`, `ULoadoutComponent`, and slot-tag compatibility as the intended model; this recommendation clarifies how `FEquippedStateSnapshot`/`FLoadoutPreset` (not yet mentioned in CLAUDE.md) relate to that documented model, it does not contradict it.

---

## R2 — Introduce a centralized team/faction identity now, since three upcoming systems all need it

**The observation:** `FGenericTeamId(0)` (player) and `FGenericTeamId(1)` (zombie) are the only two team identities in the codebase, each hardcoded as a literal return value in a different file (`ShooterGameCharacter.h:48`, `ZombieAIController.h:26`), with no attitude-solver and two separate TODO comments already acknowledging the gap (`Projectile.cpp:102`, `ReviveComponent.cpp:113`). Full detail in Finding B3.

**Why it matters now, specifically:** Three separate upcoming systems all need a third (and arguably fourth) faction identity, and each would otherwise solve it independently: Human AI Reinforcement (`docs/WaveMission_DemoPlan/02_Enemy_AI_and_Waves.md` §3, hostile to players, distinct from zombies), Temporary Squad Member (`docs/WaveMission_DemoPlan/07_Temporary_Squad_Member.md`, friendly to its commanding player, must not be targeted as hostile), and — not currently in the 10-system list but worth naming since it uses the exact same `IGenericTeamAgentInterface` surface — any future friendly-fire refinement (`AShooterGameGameMode::bFriendlyFireEnabled` currently a single global bool with no per-team nuance).

**Recommendation:** Do this once, centrally (Finding B3's proposed `ETeamID`/named-constants + attitude-helper approach), in Week 2 — before any of the three consuming systems starts. This is a small, additive change (new named constants, one new helper function) with no behavior change to existing team IDs' numeric values.

**Does this require a `CLAUDE.md` rule change?** No — CLAUDE.md does not currently document a team system at all; this recommendation fills a genuine gap rather than altering documented behavior.

---

## R3 — Standardize the "replicate on demand, not always-on" pattern as a named, documented convention

**The observation:** CLAUDE.md's Replication Pattern section documents exactly one replication idiom: `UPROPERTY(ReplicatedUsing=OnRep_X)` + `Server_X()` + `OnRep_X()` broadcasting a delegate — implicitly always-on replication for anything that follows the pattern. `docs/WaveMission_DemoPlan/06_Kit_Presets_and_Loadout.md` §1.3 and `08_Multiplayer_and_Networking.md` §1 independently arrive at a second, deliberately different idiom for a specific class of data: things that are only relevant to a narrow UI context (a player's full set of up-to-6 stored presets; a squadmate's preset summary during a resupply window) are *not* declared `Replicated` at all — instead fetched via an explicit owner-only request RPC (`Server_RequestOwnPresets` → `Client_ReceiveOwnPresets`) only when the relevant UI actually opens.

**Why it matters now, specifically:** This is a good pattern — it avoids paying constant replication bandwidth for data that's invisible 95% of a session — but it currently exists only inside one demo-plan file's design reasoning, not as a named, reusable convention. Any future system with a similar shape (large, infrequently-relevant, per-player data — which several of the remaining 10 Wave Mission systems could plausibly need) would otherwise have to independently re-derive the same reasoning, or worse, default to always-on replication for something that didn't need it (repeating the exact bandwidth mistake this pattern was designed to avoid).

**Recommendation:** Once `docs/WaveMission_DemoPlan/06_Kit_Presets_and_Loadout.md`'s implementation lands in Week 6-7, promote this pattern into a short, named addition to `CLAUDE.md`'s "Replication Pattern" section — e.g., a second paragraph: *"For data that's large and only relevant to a specific, infrequently-open UI context (not needed for constant gameplay sync), prefer an on-demand `Server_Request_X()`/`Client_Receive_X()` RPC pair over an always-on `Replicated` property. See `ULoadoutComponent`'s preset-fetch RPCs for the reference implementation."* This is additive documentation, not a change to the existing rule — the existing `ReplicatedUsing` pattern remains correct and primary for everything else.

**Does this require a `CLAUDE.md` rule change?** No — this adds a second, narrowly-scoped pattern alongside the existing one for a different class of data; it does not alter when the existing always-on pattern applies.

---

## R4 — Establish `Source/ShooterGame/Mission/` as the new top-level folder, matching existing folder conventions

**The observation:** The project's existing top-level source layout (`Enemy/`, `Quest/`, `Interaction/`, `Items/`, `Inventory/`, `Framework/`) is organized by gameplay domain, one folder per major system family. None of the 10 Wave Mission systems have an obvious home in the current layout — `AZoneDefinition` isn't really "Enemy," `UScoringSubsystem` isn't really "Framework" in the same sense as `ShooterSaveGameSubsystem` (which is cross-cutting infrastructure, not mission-specific), and so on.

**Recommendation:** `docs/WaveMission_DemoPlan/01_Phase_Triggers_and_Map_Structure.md` through `07_Temporary_Squad_Member.md` already independently proposed `Source/ShooterGame/Mission/` with subfolders (`Zone/`, `Objectives/`, `Extraction/`, `Economy/`, `SquadMember/`) for their respective new classes — this recommendation is simply to confirm that structure explicitly as the intended convention before Week 1's `AShooterGameState`/`AZoneDefinition` work creates the first files in it, so every subsequent file (across 7 different demo-plan files, built by potentially different sessions/contributors over 8 weeks) lands in the same place rather than each guessing independently. `AMissionStateManager` belongs in `Mission/` directly (not a subfolder) as the folder's namesake orchestrator.

**Does this require a `CLAUDE.md` rule change?** No — this is a new top-level folder addition, consistent with the existing domain-per-folder convention; CLAUDE.md doesn't document an exhaustive/closed list of top-level folders.

---

## R5 — Apply `WeaponConfig.h`'s good DataAsset patterns to new DataAssets; explicitly avoid its naming-drift mistake

**The observation:** Covered in Finding H3. `WeaponConfig.h` is confirmed (independently, by both sub-agent audits) as a strong structural template (`TSoftObjectPtr` for all asset references, `EditDefaultsOnly`, extensive inline comments, `GetPrimaryAssetId()` for Asset Manager integration) — but it also demonstrates what happens when a DataAsset grows across many milestones without a consolidation pass: three parallel naming schemes now coexist for the same montage concepts (e.g., `TPFire` vs. `TP_Fire` vs. newer "Third Person" category names).

**Why it matters now, specifically:** `UWaveSetDataAsset`, `UCallInDefinition`, and possibly `UZombieVariantDataAsset`/`UExtractionRewardTable` are all new `UPrimaryDataAsset` classes being authored from scratch within the same 8-week window — exactly the condition (incremental growth, multiple contributors/sessions, schedule pressure) that produced `WeaponConfig.h`'s drift in the first place.

**Recommendation:** No code change to `WeaponConfig.h` itself. When authoring each new DataAsset class in Files 02 and 05, explicitly review it against this checklist before considering it done: (a) does every asset reference use `TSoftObjectPtr`/`TSoftClassPtr`, never a hard reference, (b) is there exactly one name for each concept — no "add a new field instead of renaming the old one" shortcuts, (c) does every field have a comment explaining its purpose and units where non-obvious (matching `WeaponConfig.h`'s better sections, not its sparser ones). This is a lightweight, zero-schedule-cost habit, not a separate task.

**Does this require a `CLAUDE.md` rule change?** No — CLAUDE.md's "No weapon names hardcoded... new content = new DataAsset" convention already implies this quality bar; this recommendation makes the bar explicit for the specific new DataAssets this plan is about to create.

---

## R6 — Minor existing-code convention drift, opportunistic fix only (not a scheduled task)

While reading `Enemy/ZombieSpawnManager.h` for Finding B2, this audit noticed `SpawnedZombies` is declared `UPROPERTY(...) TArray<AZombieCharacter*> SpawnedZombies;` — a raw pointer array, where CLAUDE.md's Conventions section specifies `TObjectPtr<T> for UPROPERTY declarations — raw T* only in local function logic`. This is a pre-existing, harmless deviation (raw pointers in a `UPROPERTY() TArray` are still tracked by GC correctly, `TObjectPtr` is a safety/tooling improvement, not a correctness requirement) — not worth a dedicated task, but worth fixing opportunistically if Finding B2's spawn-manager rework (Week 1-2) already has this file open for edits. Not included in `01_REFACTOR_FINDINGS.md`'s Nice-to-have table since it's this narrow and this cheap; recorded here instead as a footnote for whoever does B2's work.

**Does this require a `CLAUDE.md` rule change?** No — this is existing code failing to follow an existing, unchanged rule.

---

## Confirmation: no documented architecture rule changes proposed

This audit reviewed every rule in `CLAUDE.md`'s Conventions, Off-Limits, and Architecture sections against all findings in `00_CODEBASE_INVENTORY.md` and `01_REFACTOR_FINDINGS.md`. **No finding or recommendation in this audit requires changing a documented `CLAUDE.md` rule.** Every recommendation either:

- Fills a gap `CLAUDE.md` already anticipates but that doesn't yet exist in `Source/` (`AShooterGameState`, `AZoneDefinition`, `ScoringSubsystem` — all already named in CLAUDE.md's Mission State Machine / Zone Spawning / Economy sections), or
- Applies an already-documented convention (DataAsset-first content, `Server_`-prefixed mutation, GameplayTag three-file-update rule, `TObjectPtr` for `UPROPERTY`) to new territory, or
- Adds narrowly-scoped documentation (R3) alongside an existing rule without altering when that rule applies.

No human confirmation gate is triggered by this audit's recommendations. (Finding B7 — the `FLoadoutPreset`/`FKitPreset` decision — is a **design** decision, not an architecture-rule change; it's flagged as Blocking in `01_REFACTOR_FINDINGS.md` and requires a decision before Week 6, but that decision is "which existing/proposed data structure to build on," not a change to any rule in `CLAUDE.md`.)
