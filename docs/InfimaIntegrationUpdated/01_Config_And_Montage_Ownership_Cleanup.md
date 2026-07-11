# Phase 1 — Config & Montage Ownership Cleanup

**Do this first.** Every later phase reads fields off `UWeaponConfig` or `AShooterGameCharacter`; building on top of a field set that has three competing versions of the same concept guarantees rework. Nothing here is exotic — it's consolidation, not new systems.

---

## The core problem: three parallel montage/config ownership systems

Confirmed directly from source (2026-07-11), not from memory:

### System A — `WeaponConfig.h`'s "TEMP LEGACY COMPATIBILITY" block (lines 39–146)
Bare-named fields: `ABPWeapon`/`ABPMagazine`, `TPFire`/`TPFireADS`/`TPFireEmpty`/`TPReload`/`TPReloadEmpty`/`TPReloadQuick`/`TPEquip`/`TPInspect`/`TPInspectEmpty`/`TPMagCheck`/`TPFireMode`/`TPGrenadeThrowQuick`/`TPRandomization`, `FPEquip`/`FPFireSemi`/`FPFireADS`/`FPFireAuto`/`FPFireEmpty`/`FPReload`/`FPReloadEmpty`/`FPReloadQuick`/`FPInspect`/`FPInspectEmpty`/`FPMagCheck`/`FPFireMode`/`FPGrenadeThrowQuick`/`FPJumpFull`/`FPRandomization`, plus `ADSLocationOffset`/`ADSRotationOffset`/`RecoilRotationPerShot`/`RecoilTranslationPerShot`. The file's own comment says: *"Keep these while migrating existing gameplay code to the BP-style names."*

### System B — `WeaponConfig.h`'s current fields (lines 231–431, 489–601)
`ABP_Weapon`/`ABP_Magazine`, `FP_FireSemi`/`FP_Reload`/... (character montages), `TP_Inspect`/`TP_Reload`/... (character montages), `FP_WEP_Reload`/`FP_WEP_Fire`/... (weapon-mesh-specific montages), `TP_WEP_MagCheck`/... , plus arrays: `FP_Melee`, `TP_Melee`, `FP_Malfunctions`, `TP_Malfunctions`, `FP_WEP_Malfunctions`, `TP_WEP_Malfunctions`, `FP_Interactions`, `TP_Interactions`, `FP_Healing`, `TP_Healing`. Also `OffsetAimDownSights`/`OffsetCrouch`/`OffsetCantedAim` as `FTransform`.

### System C — `ShooterGameCharacter.h`'s own direct montage fields (lines 385–438)
`Montage_Fire`, `Montage_Fire_Empty`, `Montage_Reload`, `Montage_Reload_Empty`, `Montage_Reload_Quick`, `Montage_FireModeSwitch`, `Montage_MagCheck`, `Montage_ClearJam_MagSwipe`, `Montage_ClearJam_Rack`, `Montage_Grenade_Throw`, `Montage_Melee_Bash`, `Montage_Melee_Swing_L`, `Montage_Melee_Swing_R` — followed immediately by a comment `// Keep these for non-Infima paths` and a second group: `HitReactMontage`, `SuppressorMontage`, `InteractionMontage_Unarmed`, `InteractionMontage_Pistol`, `InteractionMontage_Rifle`.

System C is genuinely interesting, not just legacy cruft: `InteractionMontage_Unarmed`/`Pistol`/`Rifle` is a **weapon-category** pattern (fewer montages needed than one-per-weapon-instance, more variety than one-for-everything) that doesn't exist anywhere in Infima's own design. It may be a deliberate, sensible choice for interactions/melee specifically, distinct from WeaponConfig's per-weapon-instance arrays which suit per-model content like a rifle-specific bash animation.

---

## Decision to make before touching code

For each montage category, decide **one** owner:

| Category | Candidate owner | Reasoning |
|---|---|---|
| Fire/Reload/Equip/Inspect/MagCheck/FireMode | `WeaponConfig` (System B) | Already weapon-instance-specific by nature (a pistol reloads differently from a rifle) — matches `CLAUDE.md`'s "new weapons = new data asset, no C++ changes" principle |
| Melee | **Open decision** — `WeaponConfig.FP_Melee`/`TP_Melee` arrays (per-weapon-model bash) vs. `ShooterGameCharacter.Montage_Melee_*` (category-based) | If melee should look different swinging a pistol vs. a rifle butt-stroke, category (System C) is right-sized; if every rifle has its own unique bash, System B is right. Recommend **category-based (System C)** to start — fewer assets to author, and Infima's own docs show no evidence melee needs per-weapon-instance granularity |
| Malfunctions/jam-clearing | `ShooterGameCharacter.Montage_ClearJam_*` (System C) — but only 2 montages exist (`MagSwipe`, `Rack`) vs. `WeaponConfig`'s per-weapon `FP_WEP_Malfunctions` array which supports *multiple* randomized jam types per weapon | Recommend `WeaponConfig` arrays as the real per-weapon detail source (a weapon's specific jam animations belong to its own asset), with `ShooterGameCharacter`'s fields either removed or repurposed as unarmed/fallback |
| Healing, Interactions, Grenade | `WeaponConfig` arrays (System B) for weapon-specific variants, `ShooterGameCharacter` category fields (System C) for the unarmed case | Healing/interacting while unarmed still needs *some* animation — keep both, but document which is authoritative for which case rather than leaving it implicit |

**Whatever is decided, delete the other representation.** Don't leave dead fields "just in case" — `CLAUDE.md` explicitly calls for no commented-out/dead code, and a config asset with three ways to express the same idea is exactly the kind of ambiguity that produces silent bugs (the AnimBP reading one field while a designer populates a different one).

---

## Checklist

- [ ] Confirm which of System A/B/C fields the live AnimBPs (`ABP_FP_Default`, `ABP_TP_Default`) and `UShooterAnimInstanceBase::UpdateWeaponConfigData()` actually read today — via MCP live inspection, not assumption. Whatever they read is the de facto current owner regardless of what looks "more current" in the header.
- [ ] Delete System A (`TEMP LEGACY COMPATIBILITY` block) once B is confirmed as the live path — includes `ABPWeapon`/`ABPMagazine` duplicate of `ABP_Weapon`/`ABP_Magazine`.
- [ ] Resolve the dual ADS/Recoil/Crouch offset representation: `CombatComponent.h` documents `ADSLocationTarget`/`ADSRotationTarget` as "Read from WeaponConfig on equip" — check which struct that actually pulls from (`ADSLocationOffset`/`ADSRotationOffset` most likely, given the type match). If `OffsetAimDownSights`/`OffsetCrouch`/`OffsetCantedAim` (`FTransform`) are truly unconsumed, either wire them in as the sole source (deleting the `Vector`/`Rotator` pair) or delete them and keep the simpler pair — do not keep both.
- [ ] Fix the empty `"Locomotion — First-Person Poses & Transitions"` section header in `WeaponConfig.h` (~line 507) — it's a dead header with no properties; the real FP pose fields (`FP_IdlePose`, `FP_AimPose`, etc.) live 60 lines further down under a different category comment. Either move the fields under the correct header or delete the stray header — purely a documentation/organization fix, no functional risk.
- [ ] Decide and document Melee/Malfunction/Healing/Interaction ownership per the table above. Once decided:
  - [ ] If `ShooterGameCharacter`'s category fields lose ownership of a category, remove those specific `UPROPERTY` fields rather than leaving them unused.
  - [ ] If `WeaponConfig` arrays lose ownership of a category, remove those specific arrays.
- [ ] Confirm `PrefixBulletSocket` (defaults `"Bullet_"`) actually matches the socket naming on any imported/authored magazine mesh — Infima's own troubleshooting guide calls this out explicitly as config-driven, not a hardcoded convention to assume.
- [ ] Confirm `SocketCasingEject`'s value is intentional vs. Infima's own convention name `SOCKET_Eject` (see [00_Overview.md](00_Overview.md) naming table) — no code change needed if the socket itself is correctly named in the actual weapon mesh, just confirm the `FName` default/DataAsset value points at whatever socket really exists on `SK_TFA_AR`.

## Exit criteria

- Every montage/offset concept has exactly one field representing it, with the AnimBPs and `CombatComponent`/`AWeapon` all reading from that same field.
- No `UPROPERTY` field remains that nothing reads (verified via MCP `get_properties`/graph inspection on the live AnimBPs, and via source grep for C++ readers).
- The decision for Melee/Malfunction/Healing/Interaction ownership is written down somewhere durable (this file, updated in place, is fine) so Phase 8 doesn't have to re-litigate it.
