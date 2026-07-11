# 11 — Equip State Decision: No Bare-Hands State

## Verification performed before writing this file

Per instruction, this was checked against the actual plan/audit files and, where those files pointed at specific code, against the current source — not assumed. Findings:

- **`docs/WaveMission_DemoPlan/06_Kit_Presets_and_Loadout.md`** (Kit Presets, mid-mission swap) and **`07_Temporary_Squad_Member.md`** (squad member outfitting) are the only demo-plan files that touch equip state directly. Neither explicitly designed *for* a bare-hands state, but neither explicitly ruled it out either — File 06 §4.1 says a preset "must complete before `AShooterGameGameMode::RestartPlayer` finishes spawning the character, so the applied loadout is present at first spawn, not applied a frame late," which is *close* to this decision but doesn't say it outright. File 06 §6 (mid-mission swap) requires the swap to "correctly replace currently-equipped items" but says nothing about whether a transient empty-handed tick during that replacement is acceptable.
- **`docs/RefactorAudit/01_REFACTOR_FINDINGS.md` Finding B9** explicitly left this open: its "What the refactor looks like" section says the loadout-apply loop should equip every occupied slot "and weapon destruction/replacement should be sequenced so a new weapon is fully attached before the old one is torn down (**or accept a one-frame gap if that's judged acceptable for a mid-mission swap, but this should be a decision, not an accident**)." This decision closes that open question.
- **Reading the actual code** (not just the audit's summary of it) turned up two things worth being precise about, since one of them corrects a premise in the request:
  1. The property that actually represents "what item is currently in the player's hands" is `UCombatComponent::EquippedWeapon` (`Components/CombatComponent.h`), a nullable `AWeapon*`, replicated via `OnRep_EquippedWeapon`. It **is** currently nullable in practice: `UCombatComponent::ApplyPendingLoadout()` (`CombatComponent.cpp:1354-1371`) destroys the current weapon and sets `EquippedWeapon = nullptr` *before* spawning the replacement — exactly the gap this decision closes.
  2. **`CurrentGrip`/`EWeaponGrip` is not the held-item slot.** It's a separate concept: the *foregrip pose* the currently-equipped weapon uses (`None`/`GripVertical`/`GripAngled`), read once from `WeaponConfig->WeaponGrip` at equip time (`CombatComponent.cpp:99`) and consumed by the anim graph for left-hand grip-blend selection. It has nothing to do with whether an item is held at all. This document uses the correct property (`EquippedWeapon`) below rather than the one named in the initial request.
- A genuine, live "unarmed" branch **does** already exist in running code, contrary to what "nobody considered bare hands" might suggest: `UCombatComponent::GetPlayerWeaponStance()` (`CombatComponent.cpp:1159-1172`) returns `EPlayerWeaponStance::EPWS_Unarmed` when `!EquippedWeapon`, and `ShooterAnimInstanceBase.cpp:156` computes `bWeaponEquipped = ShooterGameCharacter->GetEquippedWeapon() != nullptr` for the anim graph to branch on. This is addressed below (Section 4) rather than ignored.

No plan file or audit finding *designed a feature around* bare hands (no "unequip to fight barehanded" gameplay was ever proposed) — the gap was purely structural (a nullable pointer with one known code path, B9, that actually nulls it), not a missing feature. Section 2 below is therefore short.

---

## 1. The Decision

This game will **not** implement a true unequipped/bare-hands state.

- The player always holds something: a weapon (primary/secondary) or a tool/consumable (syringe, mortar marker, grenade, etc.).
- Default spawn state is always primary weapon equipped.
- The single held-item slot is `UCombatComponent::EquippedWeapon` — nothing ever clears it to empty after initial spawn.
- Equip/holster **transitions** between held items (weapon-to-weapon, weapon-to-tool, tool-to-weapon) still happen and still use the Infima Pack's Equip/Holster/EquipQuick animations. Only the "item → None" endpoint is cut.
- Kit swaps during resupply windows must be atomic — no frame where nothing is equipped.
- The Temporary Squad Member follows the same always-equipped rule when spawned with a preset.
- Revisit only if a specific future feature explicitly needs bare hands (melee-only stealth, surrender, climbing) — not designed around now.

---

## 2. Plan File Amendments

| File | Section | What changes |
|---|---|---|
| `docs/WaveMission_DemoPlan/06_Kit_Presets_and_Loadout.md` | §4.1 "Applying a preset to a character" | Add one sentence: `ApplyKitPresetToCharacter` must guarantee `CombatComponent::EquippedWeapon` is non-null at the end of the call — if the target preset's weapon slots somehow resolve to nothing (corrupted/empty preset), fall back to the character's `DefaultLoadout`/`SpawnDefaultWeapon()` rather than leaving the character bare-handed. |
| `docs/WaveMission_DemoPlan/06_Kit_Presets_and_Loadout.md` | §6 "In-Mission Kit Swap" | Add one sentence: `Server_SwapToPreset`'s call into `ApplyKitPresetToCharacter` must be atomic with respect to equip state — the new item must be spawned and attached *before* the previously-equipped item is destroyed, not after (this directly supersedes the "accept a one-frame gap" option `01_REFACTOR_FINDINGS.md` Finding B9 had left open — see Section 6 below). |
| `docs/WaveMission_DemoPlan/07_Temporary_Squad_Member.md` | §1.1 `ATemporarySquadMember` | Add one sentence: `Server_OutfitFromPreset` must guarantee the squad member has a non-null `EquippedWeapon`-equivalent before it becomes visible/possessed by its AIController — same invariant as the player character, no bare-handed squad member ever spawns or exists mid-mission. |

No other demo-plan file (01, 02, 03, 04, 05, 08, 09, 10) references equip state in a way that needs amending — confirmed by review, not assumed.

---

## 3. Audited Code Class Amendments

Documentation only — none of these are implemented in this pass.

| Class / Property | What changes |
|---|---|
| `UCombatComponent::ApplyPendingLoadout()` (`Components/CombatComponent.cpp:1354-1371`) | Resequence: spawn and attach the new weapon/tool *before* destroying the previously-equipped one, and fix the `break`-after-first-occupied-slot bug (already tracked as Finding B9) so tool/consumable slots aren't silently skipped — both are needed for the atomic-swap guarantee, not just the second one. |
| `UCombatComponent::EquippedWeapon` (`Components/CombatComponent.h`) | Document the invariant directly on the property: never null after the character's first tick. UE has no non-null pointer type to enforce this structurally — enforce it by construction (Section 5) plus a debug-only assertion/check where convenient, not by changing the property's type. |
| `UCombatComponent::GetPlayerWeaponStance()` (`Components/CombatComponent.cpp:1159-1172`) | Its `!EquippedWeapon → EPlayerWeaponStance::EPWS_Unarmed` branch becomes unreachable in normal play once the invariant holds. Leave the branch in place as defensive/unreachable code (matches this project's existing convention of leaving deliberate stub/guard branches untouched — see `PROGRESS.md`'s note on `Projectile.cpp`/`DownedComponent.cpp`'s friendly-fire and self-revive stubs) rather than removing it; it costs nothing to keep and remains correct as a guard. |
| `UCombatComponent::SpawnDefaultWeapon()` (`Components/CombatComponent.h/.cpp`) | No change identified as required — this is already the correct existing mechanism for "spawn always starts with primary weapon equipped." Confirm (during implementation, not this pass) that it always completes before the character's first rendered/ticked frame, since that's the concrete guarantee "default spawn state is always primary weapon equipped" depends on. |
| `EPlayerWeaponStance`/`EPWS_Unarmed` and `ShooterAnimInstanceBase::bWeaponEquipped` (`Types/PlayerWeaponStance.h`, `Player/Animation/ShooterAnimInstanceBase.cpp:156`) | Same treatment as `GetPlayerWeaponStance()` above — the anim layer's unarmed branch becomes defensive/unreachable, not deleted. No animation or IK work is implied by this decision (see Section 7). |
| `PlayerAnimInstance` (`Player/Animation/PlayerAnimInstance.h/.cpp`) | This class (flagged in `01_REFACTOR_FINDINGS.md` Finding H6 as ⚠ needing runtime verification for possibly being dead code) also independently encodes `EPWS_Unarmed`/`bWeaponEquipped`/`EquippedWeapon`. This decision doesn't change H6's status or urgency — it's still a "verify whether anything references this class" task — but if it turns out to be live, the same "leave the unarmed branch as defensive/unreachable" treatment applies to it too. |
| `CombatComponent::CurrentGrip` / `EWeaponGrip` (`Components/CombatComponent.h/.cpp:99`) | **Not amended — not the relevant property.** Recorded here only to correct the record: this is the foregrip-pose selector (sourced from `WeaponConfig->WeaponGrip`), unrelated to held-item identity. Nothing about this decision touches it. |

---

## 4. Rule: the "currently held item" property is never nullable

- **Property:** `UCombatComponent::EquippedWeapon` (an `AWeapon*`, replicated via `OnRep_EquippedWeapon`).
- **Invariant:** non-null from the end of the character's (or squad member's) first server tick onward, for the rest of that character's lifetime. There is no gameplay-reachable path that sets it to `nullptr` and leaves it that way.
- **Default:** if no other loadout/save/preset data is available at spawn, `SpawnDefaultWeapon()` (using `DefaultWeaponClass`/`DefaultWeaponConfig`) equips the primary weapon. An empty/misconfigured default loadout is a content error to fix in data, not a valid runtime state to design around.
- **Tools/consumables occupy the same slot conceptually:** equipping a syringe, mortar marker, or grenade *replaces* `EquippedWeapon`'s contents (whatever actor is currently in-hand) exactly like switching from a rifle to a pistol does — it is not a parallel "off-hand" or "no active item" state. The Infima Equip/Holster/EquipQuick animation set already models item-to-item transitions this way; this decision simply confirms every one of those transitions has a defined destination, never `None`.

---

## 5. Rule: atomic kit swaps — no empty-hands frame

- Any operation that changes what's equipped (mission-start spawn, mid-mission kit preset swap, weapon pickup swap) must be ordered so the **new** item is spawned and attached before the **old** item is destroyed or detached.
- Concretely, for `ApplyPendingLoadout()`/`SpawnAndEquipWeaponFromSlot()`: spawn-and-attach-new happens first; destroy-old happens after the new item is confirmed equipped (not before, as the code does today).
- The existing 0.05s coalescing timer in `OnLoadoutUpdated`→`ApplyPendingLoadout` (which exists to merge rapid successive loadout-change broadcasts into one apply) is unaffected by this rule — that delay happens *before* any visible change, not between destroying and re-equipping. The rule is about the ordering of the two steps inside the apply itself, not about removing the coalescing delay.
- This rule applies identically to mission-start spawn, in-mission kit preset swap (File 06 §6), and Temporary Squad Member outfitting (File 07 §1.1) — one rule, three call sites.

---

## 6. Interaction with Finding B9

`docs/RefactorAudit/01_REFACTOR_FINDINGS.md` Finding B9 ("`SpawnAndEquipWeaponFromSlot` only equips the first occupied loadout slot") is **not superseded as a task** — the underlying bug still needs fixing, and it is still scheduled and budgeted (1.5-2 dev-days, Week 6, per `docs/RefactorAudit/02_REFACTOR_SCHEDULE.md`). What this decision closes is the **ambiguity in B9's own recommended fix**: that section explicitly offered two options ("sequenced so a new weapon is fully attached before the old one is torn down... **or accept a one-frame gap if that's judged acceptable**"). The second option is now off the table. `docs/RefactorAudit/02_REFACTOR_SCHEDULE.md` is annotated accordingly (see that file's Week 6 table) rather than rewritten, per instruction.

---

## 7. Temporary Squad Member — explicit confirmation

This decision applies to `ATemporarySquadMember` identically to the player character: it is outfitted via `Server_OutfitFromPreset` → `ApplyKitPresetToCharacter` (the same function the player uses) before it becomes visible or commandable, and it must never exist in a bare-handed state at any point in its lifecycle — spawn, outfit, deploy, and eventual death/despawn all occur with a non-null `EquippedWeapon`-equivalent. No separate rule or exception is introduced for the squad member.

---

## 8. Future Exceptions

Bare-hands animation, IK, and the underlying "no item equipped" gameplay state remain entirely out of scope. If a specific future feature genuinely needs it — melee-only stealth takedowns, a surrender/hands-up mechanic, climbing that requires both hands free — that feature's own design pass is the right place to reopen this decision, including whatever anim/IK work `ShooterAnimInstanceBase`'s existing (currently-unreachable) `bWeaponEquipped`/`EPWS_Unarmed` branches would need to actually support. Nothing in the Wave Mission Mode plan should be built anticipating that need now.

---

## 9. Estimated Effort

This is a scope cut, not new work — the estimate reflects documentation and a small amount of verification, not implementation.

| Task | Estimate |
|---|---|
| Amend `06_Kit_Presets_and_Loadout.md` §4.1 and §6 | 1 hour |
| Amend `07_Temporary_Squad_Member.md` §1.1 | 0.5 hours |
| Annotate `02_REFACTOR_SCHEDULE.md`'s B9 row (this pass) | 0.25 hours |
| **Total, this pass** | **~1.75 developer-hours** |

**Not additive to the schedule:** Finding B9's existing 1.5-2 dev-day (12-16 hour) budget in `docs/RefactorAudit/02_REFACTOR_SCHEDULE.md` Week 6 already covers the one real code change this decision requires (`ApplyPendingLoadout`'s resequencing + the all-slots loop fix) — this decision tightens what "done" means for that already-scheduled task, it does not add a second task on top of it.
