# Step 2 — Core Data Asset: Base Config (`BP_TFA_BaseConfig`)

**Asset path:** `/Game/InfimaGames/TacticalFPSAnimations/Common/Core/Configs/BP_TFA_BaseConfig`
**Class default:** `PrimaryDataAsset`

Set this up before touching the character or weapon Blueprints — nearly everything else in this pack reads from an instance of this asset. It centralizes mesh references, socket names, first-person and third-person animation assets, sound cues, pose offsets, and a handful of cosmetic runtime values.

Referenced by:
- **Base Weapon** (`BP_TFA_BaseWeapon`) through `CurrentConfig`
- **Base Character** (`BP_TFA_BaseCharacter`) through `WeaponConfig`
- **Base Magazine** (`BP_TFA_BaseMagazine`) through `WeaponConfig`

---

## Field groups to populate, in a sensible fill-in order

### 1. Mesh and class references
`MeshReceiver`, `MeshMagazineSK`, `MeshMagazine`, `MeshBullet`, `MeshBulletCasing`, `MeshHandguard`, `MeshScope`, `MeshSightFront`, `MeshSightRear`, `MeshLaserAttachment`, `MeshGripVertical`, `MeshGripAngled`, `MeshSilencer`, `FP_Mesh`, `TP_Mesh`, `ABP_Weapon`, `ABP_Magazine`.

### 2. Socket names
`SocketGunAttachment`, `SocketMagazineAttachment`, `SocketMagazineReserveAttachment`, `SocketLaserAttachment`, `SocketGripAttachment`, `SocketCasingEject`, `SocketLaserStart`, `SocketHandguard`, `SocketMuzzle`, `SocketBulletChambered`, `SocketCasingJammed`, `SocketScope`, `SocketSightFront`, `SocketSightRear`, `SocketGunCamera`, `SocketHelmetCamera`, `SocketChestCamera`.

> **Gotcha, straight from the docs:** in the pack's own shipped defaults, `SocketChestCamera` is set to the same value as `SocketHelmetCamera` — meaning chest-cam and helmet-cam resolve to the identical view unless you deliberately change one of them.

### 3. First-person character montages
`FP_Equip`, `FP_FireSemi`, `FP_FireAuto`, `FP_FireEmpty`, `FP_Reload`, `FP_ReloadEmpty`, `FP_ReloadQuick`, `FP_Inspect`, `FP_InspectEmpty`, `FP_MagCheck`, `FP_GrenadeThrowQuick`, `FP_FireMode`, `FP_JumpFull`, `FP_Randomization`.

### 4. Third-person character montages
`TP_Equip`, `TP_Fire`, `TP_FireEmpty`, `TP_Reload`, `TP_ReloadEmpty`, `TP_ReloadQuick`, `TP_Inspect`, `TP_InspectEmpty`, `TP_MagCheck`, `TP_GrenadeThrowQuick`, `TP_FireMode`, `TP_Randomization`.

### 5. First-person and third-person weapon-mesh montages
FP: `FP_WEP_Equip`, `FP_WEP_Fire`, `FP_WEP_Reload`, `FP_WEP_ReloadEmpty`, `FP_WEP_ReloadQuick`, `FP_WEP_Inspect`, `FP_WEP_MagCheck`.
TP: `TP_WEP_Equip`, `TP_WEP_Fire`, `TP_WEP_Reload`, `TP_WEP_ReloadEmpty`, `TP_WEP_ReloadQuick`, `TP_WEP_Inspect`, `TP_WEP_MagCheck`.

### 6. Locomotion, poses, and transitions
FP: `FP_Locomotion_Standing`, `FP_Locomotion_Crouching`, `FP_Locomotion_Aiming` (blend spaces), `FP_RunLoop`, `FP_SprintLoop`, `FP_IdlePose`, `FP_AimPose`, `FP_IdlePoseGripAngled`, `FP_IdlePoseGripVertical`, `FP_AimPoseGripAngled`, `FP_AimPoseGripVertical`, `FP_Transition_WalkEnd`, `FP_Transition_RunEnd`, `FP_Transition_CrouchStart`, `FP_Transition_CrouchEnd`.
TP: `TP_IdleLoop`, `TP_IdlePose`, `TP_AimPose`, `TP_IdlePoseGripAngled`, `TP_IdlePoseGripVertical`, `TP_AimPoseGripAngled`, `TP_AimPoseGripVertical`, `TP_Transition_AimStart`, `TP_Transition_AimEnd`.
Weapon-side: `WEP_ReferencePose`, `WEP_FireModeStates`, `WEP_MagazineDepletion`.

### 7. Grouped/randomized animation arrays
`FP_Melee`, `TP_Melee`, `FP_Malfunctions`, `TP_Malfunctions`, `FP_WEP_Malfunctions`, `TP_WEP_Malfunctions`, `FP_Interactions`, `TP_Interactions`, `FP_Healing`, `TP_Healing`.

### 8. Offsets, cosmetic values, and sounds
`OffsetAimDownSights`, `OffsetCrouch`, `PrefixBulletSocket` (defaults to `Bullet_` — used by Base Magazine to find bullet sockets on the magazine mesh), `TotalAmmoCount` (defaults to `0`), `SoundCue_WEP_MagDrop`, `SoundCue_WEP_CasingEject`, `SoundCue_WEP_AimIn`.

---

## Which fields each downstream Blueprint actually reads

| Consumer | Fields it reads |
|---|---|
| **Base Weapon** | `MeshReceiver`, `ABP_Weapon`, `MeshHandguard`, `MeshScope`, `MeshSightFront`, `MeshSightRear`, `MeshSilencer`, `MeshBulletCasing`, `MeshGripVertical`, `MeshGripAngled`, `MeshMagazine`, `SocketMagazineAttachment`, `SocketMagazineReserveAttachment`, `SocketGripAttachment`, `SocketLaserAttachment`, `SocketCasingEject`, `SocketHandguard`, `SocketMuzzle`, `SocketBulletChambered`, `SocketCasingJammed`, `SocketScope`, `SocketSightFront`, `SocketSightRear`, `TotalAmmoCount` |
| **Base Character** | `FP_Mesh`, `TP_Mesh`, `SocketGunAttachment`, `SocketGunCamera`, `SocketHelmetCamera`, `SocketChestCamera`, `SocketMuzzle`, `OffsetAimDownSights`, `OffsetCrouch`, `FP_JumpFull`, `SoundCue_WEP_AimIn` |
| **Base Magazine** | `MeshMagazineSK`, `ABP_Magazine`, `MeshBullet`, `PrefixBulletSocket` |
| **Laser Attachment** | `MeshLaserAttachment`, `SocketLaserAttachment`, `SocketLaserStart` |

## Configuration tips (straight from the docs)

- If a socket name is wrong, most spawn/visibility logic still runs — the usual symptom is a prop or effect appearing at the mesh origin instead of erroring outright.
- If grip variants don't look right, check both the grip mesh fields **and** the grip-specific pose assets — a mismatch in either produces the same visual symptom.
- If reload props look wrong, check both `MeshMagazine` (static, for the dropped physics prop) and `MeshMagazineSK` (skeletal, for the cosmetic magazine actor) — these are two different assets serving two different purposes.
- If aim alignment feels wrong, verify `OffsetAimDownSights` before touching any camera code.
- Double-check `SocketChestCamera` isn't still equal to `SocketHelmetCamera` if you want distinct bodycam angles.

## Checklist

- [ ] Create a new `BP_TFA_BaseConfig` instance for your weapon.
- [ ] Populate mesh and class references.
- [ ] Populate all socket names, and give `SocketChestCamera` its own distinct value if you want a real chest-cam angle.
- [ ] Populate FP/TP character montages and FP/TP weapon-mesh montages.
- [ ] Populate locomotion/pose/transition assets.
- [ ] Populate the grouped animation arrays (melee/malfunctions/interactions/healing) for both FP and TP.
- [ ] Set offsets, `PrefixBulletSocket`, `TotalAmmoCount`, and the three sound cues.
- [ ] Confirm this config asset is assigned as `WeaponConfig` on the character and `CurrentConfig` on the weapon before moving on.
