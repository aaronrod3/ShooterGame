# Phase 8 — Combat Content Expansion (Melee, Healing, Interactions, Malfunctions, Grenade)

Infima ships animation assets for all five of these categories, but explicitly **no demo gameplay logic** beyond two generic notify classes (`AN_SpawnObjectAttached`, `AN_ThrowPhysicsObject`) illustrated with a syringe as their example use case. Building real logic here is genuinely new work — but ShooterGame already has more scaffolding in place than expected. **Confirm Phase 1's ownership decisions are settled before starting any of this** — building logic against fields that get deleted in the config cleanup is wasted work.

---

## Scaffolding that already exists (confirmed 2026-07-11)

`CombatTypes.h`'s `ECombatAction` enum already has: `None`, `Firing`, `Reloading`, `Inspecting`, `MagCheck`, `FireModeSwitch`, `Interacting`, `Meleeing`, `ThrowingGrenade`, `Healing`. Four of this phase's five systems already have their state-machine slot — only malfunction/jam-clearing has no enum value at all yet.

`ShooterGameCharacter.h` already has montage field slots: `Montage_Melee_Bash`, `Montage_Melee_Swing_L`, `Montage_Melee_Swing_R`, `Montage_Grenade_Throw`, `Montage_ClearJam_MagSwipe`, `Montage_ClearJam_Rack`, `InteractionMontage_Unarmed`, `InteractionMontage_Pistol`, `InteractionMontage_Rifle`. None of these currently have logic behind them — they're declared and presumably assignable in BP defaults, but unconsumed.

`WeaponConfig.h` already has parallel per-weapon arrays: `FP_Melee`/`TP_Melee`, `FP_Healing`/`TP_Healing`, `FP_Interactions`/`TP_Interactions`, `FP_Malfunctions`/`TP_Malfunctions`/`FP_WEP_Malfunctions`/`TP_WEP_Malfunctions`.

This means Phase 1's ownership decision (category-based character fields vs. per-weapon config arrays) directly determines which of these two already-built scaffolds this phase should wire logic against.

---

## Per-system build notes

### Melee
- `ECombatAction::Meleeing` exists; wire a `CombatComponent::Melee()` analog to the existing `Fire()`/`HandleFire()` pattern — same gating conventions (`!bIsBusy`, movement-state checks), same `Server_X()`/`Multicast_X()` RPC shape.
- Damage application: per `CLAUDE.md`, **always route through `TakeDamage()`, never apply directly**. A melee hit needs a short-range detection sweep (capsule/box trace or overlap check in front of the character) at the montage's impact notify, then `TakeDamage()` on whatever it hits — respecting `UHitZoneComponent`'s per-bone multiplier the same way ranged damage already does.
- Uses whichever montage source Phase 1 designates (`Montage_Melee_Bash`/`Swing_L`/`Swing_R` vs. `WeaponConfig.FP_Melee`/`TP_Melee`).

### Healing
- `ECombatAction::Healing` exists. This is the system with the most direct Infima guidance — the syringe use case is Infima's own worked example for both `AN_SpawnObjectAttached` (spawn/attach the syringe prop to the hand, with a `VisibilityDelay` so the hand can "reach" for it off-screen first) and `AN_ThrowPhysicsObject` (destroy the static syringe prop via its `DisposableItem` tag at the exact instant a physics syringe spawns in its place, creating the illusion of one continuous object).
- Actual heal effect (instant heal or heal-over-time) applies to whatever health/downed system exists — cross-reference `UDownedComponent`/`UReviveComponent` per `CLAUDE.md` to confirm healing interacts correctly with bleedout state rather than being a fully separate system.
- Classify per [Phase 2](02_Multiplayer_Replication_Principles.md): the heal amount/health change is gameplay-affecting (server-authoritative), the syringe prop spawn/throw is one-shot cosmetic (multicast).

### Interactions (grab/punch/push)
- `ECombatAction::Interacting` exists, plus the category-based `InteractionMontage_Unarmed/Pistol/Rifle` fields.
- **Needs a scope decision before building**: `CLAUDE.md` describes a dedicated `UReviveComponent` for reviving downed players — confirm "Interacting" here is meant for something else (world objects, doors, pickups) and doesn't overlap/conflict with revive's own action state. If revives already use a different `ECombatAction` value or bypass this enum entirely, that's fine; just confirm rather than assume.
- Infima's animation set for this category is generically named (`Interact_Grab`/`Interact_Punch`/`Interact_Push`) with no fixed gameplay meaning — ShooterGame gets to define what each actually does.

### Weapon malfunctions / jam-clearing
- **No `ECombatAction` value exists for this yet** — the one gap in an otherwise-scaffolded set. Add one (e.g. `ECombatAction::ClearingMalfunction`) following the existing enum's naming/`UMETA` convention.
- `Montage_ClearJam_MagSwipe`/`Montage_ClearJam_Rack` already exist on the character and are sitting unused — but `WeaponConfig.FP_WEP_Malfunctions`/`TP_WEP_Malfunctions` arrays suggest the *real* per-weapon variety (which jam animation plays, and how often) should live in config, not as two fixed character-level montages. Recommend: character-level fields become the unarmed/generic fallback (if jams even make sense unarmed — likely not, consider removing them if truly dead), config arrays become the authoritative per-weapon jam set.
- Needs a trigger condition: a random jam chance on fire (tunable per `WeaponConfig`, per `CLAUDE.md`'s "no magic numbers" rule — add a `JamChance` field to config, not a hardcoded constant in `CombatComponent`), blocking fire until the clear-jam montage completes and `AN_UnlockActions` (Phase 5) fires.

### Grenade throw
- `ECombatAction::ThrowingGrenade` and `Montage_Grenade_Throw` **already exist** — of everything in this phase, this is most likely to already have partial wiring. **Verify current state in PIE/source before assuming it needs to be built from scratch** — check for an existing `ThrowGrenade()`-style function on `CombatComponent` or `ShooterGameCharacter` before writing a new one.
- If genuinely unbuilt: mirrors `AN_ThrowPhysicsObject`'s exact mechanism (Infima explicitly calls this notify reusable "for things like grenades or magazines," not syringe-specific) — spawn the grenade actor, apply throw impulse, standard fuse/explosion logic elsewhere (likely already exists given zombie/AI damage systems are well underway per project state).

---

## Checklist

- [ ] Confirm Phase 1's Melee/Malfunction/Healing/Interaction ownership decision is finalized before starting.
- [ ] Melee: build `CombatComponent::Melee()`, hit-detection sweep, `TakeDamage()` routing through `UHitZoneComponent`.
- [ ] Healing: build syringe spawn/throw via `AN_SpawnObjectAttached`/`AN_ThrowPhysicsObject` (Phase 5), wire actual heal effect against `UDownedComponent`/health state.
- [ ] Interactions: resolve scope decision (relationship to `UReviveComponent`) before building generic grab/punch/push logic.
- [ ] Malfunctions: add the missing `ECombatAction` value, decide config-vs-character montage ownership, add a tunable `JamChance` field, wire trigger-on-fire logic.
- [ ] Grenade: **check existing wiring first** — this may already be partially or fully built.
- [ ] Apply Phase 2's replication classification to every new action added here.

## Exit criteria

- Each of the five systems has a testable action in PIE: meleeing damages a target through `TakeDamage()`, healing restores health/clears a bleedout state, an interaction plays its category-correct montage, a weapon occasionally jams and can be cleared, and a grenade throw spawns and detonates correctly.
- No new `ECombatAction` value duplicates an existing one, and no montage field exists in two owning systems simultaneously (Phase 1's cleanup should already guarantee this by the time this phase starts).
