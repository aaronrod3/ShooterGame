# Phase 4 — Physics Props & Attachments

Builds the actual spawned actors that Phase 3's `AWeapon` functions call into. One of these (casing ejection) is already done and just needs wiring; the other two are genuinely net-new.

---

## Casing ejection — already done, cross-reference only

`ACaseEject` (`Source/ShooterGame/Items/Weapon/CaseEject.h/.cpp`) already exists and is **more capable than Infima's own `BP_TFA_PhysicsCasing`**:

| Feature | Infima `BP_TFA_PhysicsCasing` (via `BP_TFA_BasePhysicsObject`) | ShooterGame `ACaseEject` |
|---|---|---|
| Lifespan | Fixed `InitialLifeSpan = 30s` | `MaxLifetime` (hard cap) **and** `LifetimeAfterRest` (shorter timer once the casing stops moving) — strictly better, avoids casings sitting rendered for the full 30s after they've already landed |
| Impact audio | Single `ImpactSound`, plays once via `DoOnce` | `UShellAudioComponent` — surface-aware, assign per-surface cues in the `BP_CaseEject_*` Details panel |
| Rest detection | None | `CheckIfAtRest()`, velocity-threshold-based, checked every `RestCheckInterval` |
| World casing count cap | None — Infima has no pooling/limit concept at all | `MaxCasingCount` + `EnforceCasingLimit()` — destroys oldest casing once the cap is hit, a perf consideration Infima's reference doesn't even attempt |

**Nothing needs to be built here.** All that's left is Phase 3's `AWeapon::EjectCasing()` actually spawning `CasingClass` instead of logging a warning.

---

## Dropped magazine physics prop — net new

No equivalent exists yet. Build a new actor mirroring `ACaseEject`'s pattern for consistency (same base behaviors: `OnHit` audio, rest detection, lifetime, world-count cap) rather than a bare port of Infima's thin `BP_TFA_PhysicsMagazine` (which only provides lifespan + a single impact sound with no rest-detection or count cap — ShooterGame's own casing actor is already a better template to copy than Infima's original).

Suggested shape:
```cpp
UCLASS()
class SHOOTERGAME_API ADroppedMagazine : public AActor
{
    // Same structural pattern as ACaseEject:
    // - UStaticMeshComponent (mesh assigned at spawn time from WeaponConfig->MeshMagazine)
    // - UShellAudioComponent or a similar reusable impact-audio component
    // - Rest detection + LifetimeAfterRest + MaxLifetime
    // - A shared "dropped prop count cap" — consider whether this should be the
    //   same global cap as casings or a separate one; magazines are visually
    //   larger and rarer, a smaller independent cap is probably right
};
```

Weapon-side assignment at spawn (mirrors Infima exactly):
- `Mesh = WeaponConfig->MeshMagazine` (the *static* mesh field — not `MeshMagazineSK`, which is for the skeletal/animated cosmetic magazine, a different asset entirely)
- `ImpactSound`/audio config `= WeaponConfig->SoundCue_WEP_MagDrop`

Classify per [Phase 2](02_Multiplayer_Replication_Principles.md): one-shot cosmetic spawn → `NetMulticast` trigger from `AWeapon::SpawnDroppedMagazine()`, client-local actor with no ongoing replicated transform.

---

## Laser attachment — net new

No existing equivalent. Infima's `BP_TFA_Attachment_Laser` is small enough to port close to verbatim:

**On spawn/attach** (only spawn if `WeaponConfig->SocketLaserAttachment` resolves on the weapon mesh — same conditional pattern as the other attachments in Phase 3):
- Assign the static mesh component from `WeaponConfig->MeshLaserAttachment`
- Cache the beam-start transform via `GetSocketTransform(WeaponConfig->SocketLaserStart, RTS_Component)`

**On Tick:**
- Line trace forward ~15000 units from the beam-start socket
- Scale the beam mesh on its length axis to the hit distance
- Move an attached point light to the impact point, enable/disable it based on whether the trace hit anything
- If the weapon config becomes invalid (e.g. weapon unequipped), disable both the beam and the light (Infima gates this with a `DoOnce` to avoid redundant calls every frame — worth keeping, it's cheap and avoids a tick-rate's worth of no-op `SetVisibility` calls)

Classify per Phase 2: **derivable from already-replicated state** (whether this weapon's config has a laser is baked content, not per-instance runtime state) → spawn/attach locally on each client from the same equip-response path used for handguard/scope/sights, no new replicated state needed. The Tick-based trace is inherently client-local cosmetic VFX and should never be server-authoritative or replicated frame-by-frame.

---

## Bullet-socket magazine visualization — verify before building

Infima's `BP_TFA_BaseMagazine` scans the skeletal magazine mesh (`MeshMagazineSK`) for sockets whose name starts with `PrefixBulletSocket` (defaults `"Bullet_"`) and spawns a small static bullet mesh (`MeshBullet`) on each match — this is what makes the animated magazine visually show rounds depleting as `WEP_MagazineDepletion` plays.

- [ ] Confirm whether this bullet-socket-scan behavior exists anywhere in ShooterGame already, or whether the cosmetic magazine mesh currently shows a static, non-depleting bullet stack (or nothing).
- [ ] If building it: this is a one-time construction-time scan (equivalent to Infima's `UserConstructionScript`), not per-frame logic — do it once when the cosmetic magazine actor/component is created, not every tick.
- [ ] Verify the actual imported magazine mesh's socket names match whatever `PrefixBulletSocket` resolves to post-Phase-1-cleanup.

---

## Checklist

- [ ] Wire `AWeapon::EjectCasing()` to spawn `CasingClass` (Phase 3 — no new actor work needed here).
- [ ] Build `ADroppedMagazine` (or equivalent name) mirroring `ACaseEject`'s structural pattern.
- [ ] Wire `AWeapon::SpawnDroppedMagazine()` to spawn it.
- [ ] Build a laser attachment actor per the notes above.
- [ ] Wire laser attachment spawn into the same conditional-attachment pass as handguard/scope/sights (Phase 3).
- [ ] Verify/build bullet-socket magazine visualization, matching whatever `PrefixBulletSocket` resolves to after Phase 1.
- [ ] Apply Phase 2's replication classification to every new actor spawn added in this phase.

## Exit criteria

- A completed reload visibly ejects a magazine with physics, matching casing-eject's existing quality bar (rest detection, lifetime, count cap).
- A weapon whose config has a laser attachment shows a working, traced laser beam in PIE; a weapon without one shows nothing and costs no extra Tick work.
- Every new actor introduced here has an explicit, written replication classification per Phase 2 — no silent single-client-only spawns.
