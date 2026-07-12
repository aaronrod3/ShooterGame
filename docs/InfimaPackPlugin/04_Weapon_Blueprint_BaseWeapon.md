# Step 4 — Weapon Blueprint: Base Weapon (`BP_TFA_BaseWeapon`)

**Asset path:** `/Game/InfimaGames/TacticalFPSAnimations/Common/Core/Weapons/BP_TFA_BaseWeapon`

The pack's main weapon actor. Assembles the runtime weapon from `CurrentConfig`, manages cosmetic props (magazines, laser attachment, grip mesh), and exposes the helper functions animation notifies rely on.

## Key properties

- **`CurrentConfig`** (`BP_TFA_BaseConfig`) — drives sockets, meshes, montages, sounds. See [02_Core_Data_Asset_BaseConfig.md](02_Core_Data_Asset_BaseConfig.md).
- **`CurrentGrip`** (`E_TFA_GripAttachment`) — tracks the currently active grip variant.
- **`AmmoCount`** (`int32`) — cosmetic ammo count driving the animated magazine.
- **`SK_Receiver`** (`SkeletalMeshComponent`) — the main weapon mesh, owner of the relevant sockets.

## Weapon setup flow

### Construction Script (runs whenever `CurrentConfig` is valid)

1. Sets the receiver skeletal mesh from `CurrentConfig.MeshReceiver`.
2. Assigns the weapon AnimBP from `CurrentConfig.ABP_Weapon`.
3. Attaches optional static meshes through config-defined sockets, via the helper **`AssignNewStaticMesh(Target, Parent, NewMesh, SocketName)`** — which sets the mesh, then attaches to the requested parent/socket. Optional parts handled this way: handguard, silencer, chambered bullet, jammed casing, scope, front sight, rear sight.

> **Correct socket names matter a lot here.** If a socket name is wrong, the mesh usually still assigns — it just attaches in the wrong place, silently.

### BeginPlay

1. Sets `AmmoCount = CurrentConfig.TotalAmmoCount`.
2. Scans mesh components for `SocketLaserAttachment` — if found, spawns `BP_TFA_Attachment_Laser` and attaches it there.
3. Scans mesh components for `SocketGripAttachment` — if found, attaches `SM_Grip_Attachment` there.
4. Spawns two cosmetic magazines via `SpawnMagazine` — one at `SocketMagazineAttachment`, one at `SocketMagazineReserveAttachment`.
5. Hides the reserve magazine by default: `SetMagazineVisibility(false, true)`.

## Runtime state and cosmetic logic

- **`AmmoCount`**: initialized from `TotalAmmoCount` on BeginPlay. On `Tick`, if `AmmoCount < 0`, it's reset back to `TotalAmmoCount` — keeps the cosmetic magazine logic from staying invalid after another shot fires while already empty.
- **`RandomizeGripAttachment()`** — custom event cycling `CurrentGrip` through three states:
  - `NewEnumerator0` — hides `SM_Grip_Attachment`
  - `NewEnumerator1` — shows `SM_Grip_Attachment`, assigns `CurrentConfig.MeshGripVertical`
  - `NewEnumerator2` — shows `SM_Grip_Attachment`, assigns `CurrentConfig.MeshGripAngled`
  - (i.e., rotates between no grip → vertical grip → angled grip)

## Sockets read from config and used at runtime

`SocketMagazineAttachment`, `SocketMagazineReserveAttachment`, `SocketCasingEject`, `SocketGripAttachment`, `SocketLaserAttachment`. Magazine spawn, magazine drop, and casing ejection all depend on these socket transforms — if missing, the spawned result appears at the weapon origin. Grip and laser setup work slightly differently: on BeginPlay, the weapon iterates mesh components and only attaches/spawns when the component actually reports the requested socket.

## Functions used by animation notifies

- **`AssignNewStaticMesh(Target, Parent, NewMesh, SocketName)`** — sets the mesh, attaches to parent at socket. Used heavily by the Construction Script.
- **`SpawnMagazine(SocketName)`** — spawns `BP_TFA_BaseMagazine` with `CurrentConfig` as `WeaponConfig`, validates, attaches to `SK_Receiver` at the given socket with snap rules. Called twice on BeginPlay (main + reserve).
- **`SetMagazineVisibility(bool bVisible, bool bIsReserve)`** — iterates child components of `SK_Receiver` (including descendants), compares each `AttachSocketName` against the selected magazine socket (`SocketMagazineAttachment` if `bIsReserve = false`, `SocketMagazineReserveAttachment` if `true`), calls `SetHiddenInGame(NewHidden = !bVisible, PropagateToChildren = true)` on matches. **Socket-driven, not name-driven** — it doesn't care which component type is attached, only which socket it's attached to.
- **`SpawnDroppedMagazine(float ImpulseForce, float RotationForce)`** — spawns `BP_TFA_PhysicsMagazine` at `SK_Receiver.GetSocketTransform(SocketMagazineAttachment, RTS_World)`, sets mesh to `CurrentConfig.MeshMagazine`, `ImpactSound = CurrentConfig.SoundCue_WEP_MagDrop`. Linear impulse along the magazine socket's up vector × `ImpulseForce` (`bVelChange = true`); angular impulse along a random axis × `RotationForce` (`bVelChange = true`).
- **`EjectCasing(RotationOffset, MinEjectForce, MaxEjectForce, RotationSpeed)`** — spawns `BP_TFA_PhysicsCasing` at `SK_Receiver.GetSocketTransform(SocketCasingEject, RTS_World)`, `ImpactSound = CurrentConfig.SoundCue_WEP_CasingEject` when supported, mesh set to `CurrentConfig.MeshBulletCasing`. Direction from the forward vector of `CombineRotators(SocketRotation, RotationOffset)`; force randomized in `[MinEjectForce, MaxEjectForce]`; linear impulse `bVelChange = false` (mass matters); spin via random axis scaled by `RotationSpeed`, `bVelChange = true`.

## Lifecycle notes

The weapon already spawns both magazine actors and hides the reserve one on BeginPlay — this is why the notify states only need to *toggle visibility*, never spawn/destroy the props themselves. On Tick, it guards against a negative cosmetic ammo count by restoring from `TotalAmmoCount`.

## Common customization points

| Symptom | First thing to check |
|---|---|
| Ejection direction feels wrong | `SocketCasingEject` transform on the receiver skeleton, or bias `RotationOffset` in the notify |
| Magazine visibility not working | Spawned magazine actors attached to the exact same sockets `SetMagazineVisibility` checks — socket name mismatches are the most common cause |
| Grip mesh wrong or not visible | `SocketGripAttachment` exists on the mesh component actually used at runtime; `MeshGripVertical`/`MeshGripAngled` are set in config |
| Ammo-driven cosmetic behavior looks stuck | Check whether another system is driving `AmmoCount`; this Blueprint resets negative values back to `TotalAmmoCount` on Tick |

## Checklist

- [ ] Confirm the Construction Script correctly assembles the receiver mesh and all optional attachment meshes for your weapon.
- [ ] Confirm both magazine actors spawn on BeginPlay, with the reserve one hidden by default.
- [ ] Confirm grip and laser attachments only spawn when their sockets actually exist.
- [ ] Test `SpawnDroppedMagazine` and `EjectCasing` in isolation (e.g. via a temporary debug key) before wiring them to real animation notifies.
