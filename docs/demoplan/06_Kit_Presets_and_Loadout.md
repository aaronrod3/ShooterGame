# 06 — Kit Presets and Loadout

## Dependencies

- [05_Economy_and_Resupply.md](05_Economy_and_Resupply.md) — in-mission kit swap is gated by that file's resupply-window-phase check and `SquadSharedFund` cost deduction; this file owns only the actual preset-swap mechanics.
- [08_Multiplayer_and_Networking.md](08_Multiplayer_and_Networking.md) — squad visibility of teammates' presets during resupply requires an on-demand replication/RPC design, not full always-on replication; read the authority model first.
- Builds directly on the existing `ULoadoutComponent` / `FLoadoutData` / `FLoadoutSlot` / `EEquipmentSlot` system (`Source/ShooterGame/Types/LoadoutTypes.h`) and `AShooterPlayerState::SavedLoadout` — this file does **not** replace that system, it wraps it in a named-preset layer.

---

## 1. Kit Preset Data Structure

### 1.1 Two existing systems this must bridge

The project currently has two separate equipment systems, both already built:

1. **`ULoadoutComponent` / `FLoadoutData`** (`Types/LoadoutTypes.h`) — the fixed 8-slot weapon/tool loadout (`Primary`, `Secondary`, `Pistol`, `Tool`, `Knife`, `Consumable1`, `Consumable2`, `SpecialItem`), each slot a `TSoftClassPtr<AActor>` + `MagazineCount` + `VariantID`. This is what `AShooterPlayerState::SavedLoadout` currently persists.
2. **`UInventoryComponent`** (per CLAUDE.md) — a tag-gated `FItemInstance` container using the `Slot.*` GameplayTags (`TAG_Slot_Backpack`, `TAG_Slot_Rig`, `TAG_Slot_Belt`, `TAG_Slot_MedPouch`, etc.) — this is where armor, chest rig contents, and carried consumables actually live, referenced by `FItemInstance::InstanceID` GUID (never array index, per the project's stated convention).

GameplayPlan.md's kit preset description ("weapons + attachments, armor tier, chest rig, magazine loadout, consumables") spans **both** systems — a preset is not just a `FLoadoutData`, it must also snapshot the tag-slotted inventory items. This is the one real design decision in this file: how to combine them without duplicating either system's logic.

### 1.2 `FKitPreset`

New file: `Source/ShooterGame/Types/KitPresetTypes.h`

```cpp
USTRUCT(BlueprintType)
struct FKitPresetItemEntry
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) FGameplayTag TargetSlotTag; // Slot.Rig, Slot.Backpack, etc.
    UPROPERTY(EditAnywhere) TSoftClassPtr<UItemDefinition> ItemDefinitionClass; // soft ref, same rationale as FLoadoutSlot::ItemClass
    UPROPERTY(EditAnywhere) int32 Quantity = 1;
};

USTRUCT(BlueprintType)
struct FKitPreset
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
    FName PresetName = NAME_None;

    // Informational only, per GameplayPlan.md — "class-informed... not class-locked."
    // Never gates CanEquipInSlot() or any equip validation. Pure UI/recommendation metadata.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
    FGameplayTag ClassTag; // Class.Assaulter / Class.Support / Class.Heavy / Class.Scout — Section 5

    // Reuses the existing struct exactly — weapons, tools, magazine counts.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
    FLoadoutData WeaponLoadout;

    // Armor tier, chest rig, and carried consumables — a flat list of definition+quantity
    // pairs rather than full FItemInstance snapshots, since a *preset* is a template
    // ("bring 4x bandages") not live item state (a specific bandage's InstanceID and
    // any per-item condition it may have accrued). Concrete FItemInstance GUIDs are
    // only minted when the preset is actually applied to a character.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
    TArray<FKitPresetItemEntry> InventoryLoadout;
};
```

`FKitPresetItemEntry` deliberately does **not** reuse `FItemInstance` — `FItemInstance` represents a concrete, already-instantiated item with a real `InstanceID` and (per the inventory component's design) potential per-item condition/durability state. A stored preset is a *template*; applying it (Section 4) is what mints fresh `FItemInstance`s via `UInventoryComponent`'s existing item-creation path. Treating presets as templates rather than frozen instance snapshots also sidesteps any issue where an item referenced by a saved preset was later rebalanced or removed from the game — the definition soft-reference resolves at apply-time, always against current data.

### 1.3 Storage on `AShooterPlayerState`

```cpp
UPROPERTY(BlueprintReadOnly, Category = "Preset")
TArray<FKitPreset> SavedPresets; // max 6, per GameplayPlan.md "up to 6 named presets"

UPROPERTY(BlueprintReadOnly, Category = "Preset")
int32 ActivePresetIndex = INDEX_NONE;

UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Preset")
void Server_SavePreset(int32 SlotIndex, const FKitPreset& PresetData);

UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Preset")
void Server_DeletePreset(int32 SlotIndex);
```

**Not fully replicated at all times.** Unlike `SavedLoadout` (which is always-relevant to the owning client and its currently-equipped state), a player's full set of up-to-6 stored presets is only ever relevant (a) to that player, during the loadout screen or mission-start selection, and (b) to squadmates, only during an active resupply window (GameplayPlan.md: "Squad can see each other's current and available presets during the window"). Replicating 6 full `FKitPreset` structs (each containing a full `FLoadoutData` plus an inventory array) to every client at all times is unnecessary bandwidth for data that's invisible outside two specific UI contexts. Instead:

- `SavedPresets` is `NotReplicated` and lives server-side (authoritative) plus locally on the owning client (via the owner-only RPC responses below) — **not** a blanket `Replicated` UPROPERTY.
- The owning client requests its own list via `Server_RequestOwnPresets()` → `Client_ReceiveOwnPresets(const TArray<FKitPreset>&)` (an `OwningClient`-reliable RPC), called when the loadout screen or mission-start selection UI opens.
- Squadmates request a *lightweight* summary — not full preset data — via `Server_RequestSquadPresetSummary(APlayerState* TargetPlayer)` → `Client_ReceiveSquadPresetSummary(APlayerState* TargetPlayer, const TArray<FKitPresetSummary>&)`, where `FKitPresetSummary` is just `{ FName PresetName; FGameplayTag ClassTag; }` — enough to render "Alex: [Assaulter — 'CQB Loadout'] [Support — 'Long Watch']" in the resupply UI without ever sending the other player's full weapon/inventory data across the wire until they actually request a swap. This on-demand, summary-first pattern is the correct bandwidth-conscious approach and should be called out explicitly in File 08 as the pattern for any future "let squad see X about each other" feature.

---

## 2. Persistent Storage

This is purely an extension of the existing save path — CLAUDE.md is explicit that `UShooterSaveGameSubsystem` is "the sole disk I/O point... never call SaveGameToSlot/LoadGameFromSlot anywhere else." `Server_SavePreset`/`Server_DeletePreset` do not touch disk directly; they mutate `AShooterPlayerState::SavedPresets` in memory and then call the existing subsystem's save-trigger path (whatever currently persists `SavedLoadout`/`SavedAppearance` — extend that same write, do not add a second one).

`UShooterSaveGame` (the `USaveGame` subclass, `Source/ShooterGame/Framework/Subsystems/ShooterSaveGame.h`) gains a `TArray<FKitPreset> SavedPresets` field alongside its existing loadout/appearance fields. Per `FLoadoutSlot`'s own documented save-compatibility rule ("adding new fields is safe... removing or reordering WILL break existing save files"), this is a pure addition — safe without a save-version bump, but if `UShooterSaveGame` has a `SaveVersion` field already (check during Week 1 of this file's work), increment it per that system's existing convention anyway, since a new top-level array is exactly the kind of change that convention exists to track.

---

## 3. Island Base Loadout Screen (Minimal Demo Version)

New widget: `UW_LoadoutScreen` (HUD/Widgets/) — a pre-mission UMG screen, not an in-world location for the demo (a fully modeled "island base" hub level is a stretch goal explicitly out of scope per File 10 — the demo needs the *loadout configuration flow* to work, not a walkable hub).

Minimal demo scope:
- List of up to 6 preset slots, each showing `PresetName` + `ClassTag` if configured, or "Empty" if not.
- Selecting a slot opens an edit view: 8 weapon-loadout slots (reusing whatever slot-picker widget logic already backs the existing pre-game loadout screen, if one exists — confirm during Week 1; if the project has no loadout UI yet at all, this is new) plus an inventory panel for armor/rig/consumables filtered by the `Slot.*` tags relevant to kit presets (`Slot.Rig`, `Slot.Backpack`, `Slot.Belt`, `Slot.MedPouch` — not `Slot.WeaponMod`, `Slot.Holster`, `Slot.MissionCache`, or `Slot.Stash`, which are either handled inside the weapon loadout itself or are in-mission-only containers not relevant to a pre-game preset).
- Save button calls `Server_SavePreset`.
- `ClassTag` selection is a simple dropdown of the four class tags (Section 5) — purely informational, does not filter available items.

---

## 4. Mission Start Preset Selection UI

New widget: `UW_PresetSelector` (HUD/Widgets/) — shown once, before `EMissionPhase::Phase1Infiltration` begins (during `PreMission`), listing the player's `SavedPresets` (fetched via `Server_RequestOwnPresets`, Section 1.3) for a single-click "select active preset" action.

On selection: `Server_SelectActivePreset(int32 PresetIndex)` on `AShooterPlayerState` sets `ActivePresetIndex` and applies the preset (Section 4.1) to the player's about-to-spawn character — this must complete before `AShooterGameGameMode::RestartPlayer` finishes spawning the character, so the applied loadout is present at first spawn, not applied a frame late in a visibly-popping-in way.

### 4.1 Applying a preset to a character

```cpp
UFUNCTION(BlueprintCallable, Category = "Preset")
void ApplyKitPresetToCharacter(const FKitPreset& Preset, ACharacter* TargetCharacter);
```

Implementation: (1) call the existing `AShooterPlayerState::SetFullLoadout(Preset.WeaponLoadout, /*bPushToCharacter=*/true)` — this is a direct reuse of an already-existing, already-tested function, no new weapon-equip logic needed; (2) for each `FKitPresetItemEntry` in `InventoryLoadout`, resolve the soft `ItemDefinitionClass`, mint a fresh `FItemInstance` (`AssignNewInstanceID()`), and add it to the target character's `UInventoryComponent` at `TargetSlotTag` via that component's existing add-item path (do not bypass its slot-compatibility validation — a preset item must still satisfy `AcceptedSlotTags`, exactly like any other inventory mutation, so a corrupted or hand-edited preset can't smuggle an invalid item placement past normal validation).

---

## 5. Class Identity Integration

Four `FGameplayTag` values, added following the existing `Slot.*` pattern (new root, same three-file update rule from CLAUDE.md applies):

```cpp
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Class_Assaulter);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Class_Support);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Class_Heavy);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Class_Scout);
```

Per GameplayPlan.md, explicitly **not class-locked** in this build — `FKitPreset::ClassTag` is metadata only, read by `UW_LoadoutScreen` (to show a "recommended for" label) and `UW_ResupplyKitSwap` (Section 6). No system in this plan — not `CanEquipInSlot`, not `AObjectiveTerminal::CanInteract`, not `ACallInManager::ValidateRequest` — ever branches on `ClassTag`. This must be enforced as a hard rule during code review of this file's implementation: it would be easy, and wrong, for a future contributor to "helpfully" add a class-gate somewhere.

---

## 6. In-Mission Kit Swap

New widget: `UW_ResupplyKitSwap` (HUD/Widgets/) — a panel within File 05's `UW_ResupplyOverlay`, shown only while `AShooterGameState::CurrentMissionPhase` is `Resupply1`/`Resupply2`.

Flow: player selects a different stored preset from their own list → client calls `Server_SwapToPreset(int32 PresetIndex)` on `AShooterPlayerState` → server validates (File 05 Section 4's economy gate: correct phase, sufficient `SquadSharedFund`) → on success, deducts cost via `UScoringSubsystem`, then calls `ApplyKitPresetToCharacter` (Section 4.1) against the player's *currently possessed* character (not a fresh spawn — the character already exists mid-mission, so this must correctly **replace** currently-equipped items, not merely add the new preset's items alongside old ones: clear `UInventoryComponent` slots targeted by the new preset's `InventoryLoadout` entries and call `SetFullLoadout` again, which already fully overwrites `FLoadoutData` by design).

### 6.1 Squad visibility during the window

`UW_ResupplyKitSwap` also renders a per-squadmate summary panel using `Server_RequestSquadPresetSummary` (Section 1.3) — each squadmate's *currently equipped* preset (if their active loadout matches a named preset — track this via `ActivePresetIndex`, falling back to "Custom/Unsaved" display if the player's current gear doesn't match any saved preset, e.g. after picking up in-mission loot) plus their list of available presets to swap to. This satisfies "Squad can see each other's current and available presets during the window — enables coordination" without requiring full inventory replication to every client (Section 1.3's bandwidth rationale applies identically here).

---

## 7. UE5 References

- **`USaveGame`** — `UShooterSaveGame::SavedPresets` field addition: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see Saving and Loading → Save Game)
- **Data Assets** — `UItemDefinition` soft-reference resolution when applying a preset: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see Content → Data Assets)
- **UMG** — `UW_LoadoutScreen`, `UW_PresetSelector`, `UW_ResupplyKitSwap`: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see User Interface → UMG UI Designer)
- **PlayerState replication and owner-only RPCs** — on-demand preset fetch pattern (`Server_RequestOwnPresets`/`Client_Receive...`): https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see Networking and Multiplayer → RPCs, Player State)
- **GameplayTags** — `Class.*` tag family: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see Gameplay Framework → Gameplay Tags)

---

## 8. Week-by-Week Tasks

**Week 1 (Jul 13 – Jul 19)** — light touch, parallel with File 01 foundation work
- Confirm whether a pre-game loadout UI already exists to build on top of; confirm `UShooterSaveGame`'s current field list and whether it has a `SaveVersion` field.
- Draft `FKitPreset`/`FKitPresetItemEntry` structs.

**Week 6 (Aug 17 – Aug 23)**
- Implement `SavedPresets` storage on `AShooterPlayerState`, `Server_SavePreset`/`Server_DeletePreset`, extend `UShooterSaveGame`/`UShooterSaveGameSubsystem` persistence.
- Implement `Server_RequestOwnPresets`/`Client_ReceiveOwnPresets` and `Server_RequestSquadPresetSummary`/`Client_ReceiveSquadPresetSummary`.
- Build `UW_LoadoutScreen` (minimal version, Section 3).
- Build `UW_PresetSelector` (Section 4) and wire `ApplyKitPresetToCharacter` into `PreMission`/`RestartPlayer` spawn flow.
- Add `Class.*` GameplayTags (all three files per CLAUDE.md rule).
- **Checkpoint C dependency:** kit preset selection at mission start is required for this week's "full 4-phase arc playable" checkpoint.

**Week 7 (Aug 24 – Aug 30)**
- Build `UW_ResupplyKitSwap`, wire `Server_SwapToPreset` (coordinating with File 05's economy gate, built the same week).
- Implement mid-mission preset swap correctly replacing (not stacking on top of) currently-equipped items.
- Implement squad-visibility summary panel (Section 6.1).
- Verify "Custom/Unsaved" fallback display when a player's live gear doesn't match any saved preset.

---

## 9. Acceptance Criteria

- [ ] A player can create, edit, and save up to 6 named presets from the loadout screen, each persisting across a full editor session restart (verified via `UShooterSaveGameSubsystem`, not a mocked save).
- [ ] Selecting an active preset at mission start correctly equips both weapon-loadout slots and inventory items (armor/rig/consumables) before the character is first visible to other players.
- [ ] `ClassTag` never blocks or filters any equip action anywhere in the codebase — grep-verified as part of code review, not just manually tested.
- [ ] During a resupply window, a player can swap to a different stored preset, and their character's equipped gear fully reflects the new preset afterward (no leftover items from the previous preset in slots the new preset doesn't occupy).
- [ ] Kit swap is rejected server-side outside a resupply window and when `SquadSharedFund` is insufficient (cross-checked against File 05's acceptance criteria).
- [ ] A squadmate can see another player's available presets and currently-active preset name during a resupply window without that data being visible/replicated outside of resupply windows (verify via network profiling that preset data isn't sent every tick).
- [ ] Applying a preset with a soft-referenced item whose `UItemDefinition` asset has since changed does not crash — it resolves against current data at apply-time.

---

## 10. Estimated Effort

| Task Cluster | Estimate |
|---|---|
| `FKitPreset`/`FKitPresetItemEntry` structs + `AShooterPlayerState` storage/RPCs | 3 dev-days |
| `UShooterSaveGame`/`UShooterSaveGameSubsystem` persistence extension | 1.5 dev-days |
| `UW_LoadoutScreen` | 3 dev-days |
| `UW_PresetSelector` + mission-start apply flow | 2 dev-days |
| `ApplyKitPresetToCharacter` (weapon loadout + inventory item minting) | 2.5 dev-days |
| `UW_ResupplyKitSwap` + mid-mission swap logic (replace-not-stack) | 3 dev-days |
| Squad visibility summary panel | 2 dev-days |
| `Class.*` GameplayTags plumbing | 0.5 dev-days |
| **Total** | **17.5 dev-days** (spans Weeks 1, 6, and 7) |
