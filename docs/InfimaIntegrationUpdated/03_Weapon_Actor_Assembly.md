# Phase 3 — Weapon Actor Cosmetic Assembly

**Good news up front:** this phase is further along than expected. `AWeapon` already replicates Infima's `BP_TFA_BaseWeapon` construction-script pattern for most static attachments. This phase is about **completing** the pattern for the two pieces it's missing (grip mesh, laser) and **wiring the three functions that currently just log a warning**, not building the whole system from scratch.

---

## What already exists (confirmed 2026-07-11, `Weapon.h`/`Weapon.cpp`)

- `AWeapon::AttachStaticMeshFromConfig(const TSoftObjectPtr<UStaticMesh>& Mesh, FName Socket)` — a private helper that creates, registers, and attaches a static mesh component at a config-driven socket. This is the exact equivalent of Infima's `AssignNewStaticMesh(Target, Parent, NewMesh, SocketName)`.
- Component slots already wired through it: `MeshComp_Handguard`, `MeshComp_Scope`, `MeshComp_SightFront`, `MeshComp_SightRear`, `MeshComp_Silencer` — all "created at runtime by `InitFromConfig` — null if config field was empty," matching Infima's conditional-attach behavior exactly.
- `InitFromConfig(UWeaponConfig* InConfig)` already exists as the assembly entry point (Infima's equivalent is the Construction Script + `BeginPlay`).
- `CasingClass` (`TSubclassOf<ACaseEject>`) already exists as a property — the eject-casing wiring in Phase 4 has almost nothing left to build structurally.
- Three notify-driven stub functions already have the **correct signatures**, matching Infima's Blueprint functions field-for-field:
  ```cpp
  virtual void SpawnDroppedMagazine(float ImpulseForce, float RotationForce);
  virtual void EjectCasing(FRotator RotationOffset, float MinEjectForce, float MaxEjectForce, float RotationSpeed);
  virtual void SetMagazineVisibility(bool bVisible, bool bReserve);
  ```
  Currently each just does `UE_LOG(..., Warning, TEXT("... not yet implemented"))` and returns.

## What's missing

1. **Grip mesh assembly.** No `MeshComp_Grip` (or equivalent) exists in the private component list. `WeaponConfig.MeshGripVertical`/`MeshGripAngled` and `WeaponGrip` (`EWeaponGrip`) exist but nothing consumes them to actually attach a grip mesh at `SocketGripAttachment`.
2. **Laser attachment assembly.** No component/actor consumes `MeshLaserAttachment`/`SocketLaserAttachment`/`SocketLaserStart` yet (this is really Phase 4's laser *actor*, but the "does the socket exist, should we spawn one" check belongs here alongside the other conditional attachments).
3. **The three stub function bodies** — see per-function notes below.
4. **Whether cosmetic magazine mesh actors/components exist at all.** Infima spawns two persistent `BP_TFA_BaseMagazine` actors on `BeginPlay` (main + reserve) so that later notify-states only need to toggle visibility, never spawn/destroy. Nothing found in `Weapon.h`/`Weapon.cpp` yet does this — confirm during this phase before wiring `SetMagazineVisibility()`, since there's nothing to make visible/invisible without it.

---

## Per-function implementation notes

### Grip mesh (new)
Follow the exact same pattern as handguard/scope/sights: add a `MeshComp_Grip` slot, and in `InitFromConfig` (or a small helper called from it), switch on `WeaponConfig->WeaponGrip`:
- `EWeaponGrip::None` → don't attach anything (or hide if a component already exists from a previous config)
- `EWeaponGrip::GripVertical` → `AttachStaticMeshFromConfig(WeaponConfig->MeshGripVertical, WeaponConfig->SocketGripAttachment)`
- `EWeaponGrip::GripAngled` → `AttachStaticMeshFromConfig(WeaponConfig->MeshGripAngled, WeaponConfig->SocketGripAttachment)`

Unlike Infima's `RandomizeGripAttachment()` (a runtime-cycling custom event), ShooterGame's `WeaponGrip` is config-driven and set once per equip — confirm this is the intended design (see [02_Multiplayer_Replication_Principles.md](02_Multiplayer_Replication_Principles.md)'s `CurrentGrip` note) rather than adding a cycling action nobody asked for.

### `SpawnDroppedMagazine(float ImpulseForce, float RotationForce)`
Needs a physics magazine actor first — see [04_Physics_Props_And_Attachments.md](04_Physics_Props_And_Attachments.md). Once that exists, the body mirrors Infima's implementation almost exactly:
- Spawn transform: `WeaponMesh->GetSocketTransform(WeaponConfig->SocketMagazineAttachment, RTS_World)`
- Assign the spawned actor's mesh from `WeaponConfig->MeshMagazine`, impact sound from `WeaponConfig->SoundCue_WEP_MagDrop`
- Linear impulse along the socket's up vector × `ImpulseForce` (Infima's default is **-50.0**, i.e. deliberately negative — the impulse direction is inverted relative to the socket's up vector, so match sign convention or the magazine will fly the wrong way)
- Angular impulse: random axis × `RotationForce` (Infima default **150.0**)
- Both impulses `bVelChange = true`
- Classify per [Phase 2](02_Multiplayer_Replication_Principles.md): one-shot cosmetic → `NetMulticast`, not new replicated state.

### `EjectCasing(FRotator RotationOffset, float MinEjectForce, float MaxEjectForce, float RotationSpeed)`
This one is nearly free — `CasingClass` already exists as a property:
- Spawn `CasingClass` (already typed as `TSubclassOf<ACaseEject>`) at `WeaponMesh->GetSocketTransform(WeaponConfig->SocketCasingEject, RTS_World)`, rotated by `RotationOffset`
- Assign `ImpactSound` if `ACaseEject` exposes a settable one (it does — `ShellAudioComp`, though check whether it's per-surface already and doesn't need weapon-side assignment at all)
- Linear impulse along the composed forward vector, magnitude randomized in `[MinEjectForce, MaxEjectForce]`, `bVelChange = false` (mass matters, matching Infima's own choice here — casings should feel like they have weight, unlike the dropped magazine which uses `bVelChange = true`)
- Angular impulse: random axis × `RotationSpeed`, `bVelChange = true`
- **`ACaseEject` already has its own rest-detection, lifetime, and world-count-cap logic built in** — don't duplicate any of that here, this function's only job is to spawn it and apply the initial kick.
- Classify per Phase 2: one-shot cosmetic → `NetMulticast`.

### `SetMagazineVisibility(bool bVisible, bool bReserve)`
Depends on resolving the open question above (do persistent magazine mesh actors/components exist?). Once they do, mirror Infima's socket-driven approach exactly — it is **not** name-based:
- Iterate child components of the weapon mesh (including descendants)
- Compare each component's `AttachSocketName` against `bReserve ? SocketMagazineReserveAttachment : SocketMagazineAttachment`
- `SetHiddenInGame(!bVisible, /*PropagateToChildren=*/true)` on matches

This means whatever spawns the magazine actor(s) must attach them at exactly `SocketMagazineAttachment`/`SocketMagazineReserveAttachment` for this lookup to find them later — don't invent a different attach convention.

---

## Checklist

- [ ] Confirm/build persistent main + reserve cosmetic magazine actors, spawned once (likely from `InitFromConfig` or `BeginPlay`), attached at `SocketMagazineAttachment`/`SocketMagazineReserveAttachment`. Reserve hidden by default, matching Infima's `SetMagazineVisibility(false, true)` on spawn.
- [ ] Add `MeshComp_Grip`, wire it through `WeaponGrip` switch using the existing `AttachStaticMeshFromConfig` helper.
- [ ] Implement `SpawnDroppedMagazine()` once the physics magazine actor exists (Phase 4).
- [ ] Implement `EjectCasing()` — spawn `CasingClass`, apply impulses per the notes above.
- [ ] Implement `SetMagazineVisibility()` — socket-driven visibility toggle per the notes above.
- [ ] Classify each of the above per [Phase 2](02_Multiplayer_Replication_Principles.md) and implement the correct multicast/local-derivation pattern, not ad-hoc replicated bools.
- [ ] Confirm `SOCKET_Eject` vs. `SocketCasingEject` naming (see [01](01_Config_And_Montage_Ownership_Cleanup.md)) is resolved before wiring `EjectCasing()`, so the spawn transform actually resolves to a real socket instead of falling back to the component origin.

## Exit criteria

- A weapon equipped in PIE shows the correct grip mesh (or none) per its config.
- Firing produces a `ACaseEject` casing with correct spawn location/impulse.
- A reload (once its notify hooks land in Phase 5) visibly drops a magazine and toggles main/reserve mag visibility correctly on the weapon mesh, on every connected client.
