# Step 5 — Supporting Actors: Magazine, Physics Props, Laser

These are the smaller actors `BP_TFA_BaseWeapon` and `BP_TFA_BaseCharacter` spawn. Set them up once, before wiring the notify classes that trigger them (see [08_Animation_Notifies_And_States.md](08_Animation_Notifies_And_States.md)).

---

## Base Magazine (`BP_TFA_BaseMagazine`)

**Asset path:** `/Game/InfimaGames/TacticalFPSAnimations/Common/Core/Weapons/BP_TFA_BaseMagazine`

A cosmetic magazine prop the weapon spawns and attaches to its configured magazine sockets on BeginPlay. Builds its visuals from `WeaponConfig`, using a magazine skeletal mesh plus optional bullet meshes attached to bullet sockets. The **Hide Main Mag** and **Show Reserve Mag** notify states don't spawn new actors — they only toggle visibility of these already-attached props.

**Construction behavior** (`UserConstructionScript`):
1. Validates `WeaponConfig`.
2. Assigns the `SkeletalMesh` component from `WeaponConfig.MeshMagazineSK`, sets its Anim Instance Class from `WeaponConfig.ABP_Magazine`.
3. Scans all socket names on the `SkeletalMesh` component. For every socket whose name starts with `WeaponConfig.PrefixBulletSocket` (default `Bullet_`), creates a new `StaticMeshComponent`, disables its collision, sets its mesh to `WeaponConfig.MeshBullet`, and attaches it to the matching socket.

**Dependencies:** requires a valid `WeaponConfig`; reads `MeshMagazineSK`, `ABP_Magazine`, `MeshBullet`, `PrefixBulletSocket`.

**Customization notes:** bullet mesh count is directly driven by how many bullet sockets exist on the magazine skeleton — useful to know if you want more detailed magazine visuals. If the magazine mesh/animation looks wrong, check `MeshMagazineSK`/`ABP_Magazine` first; if bullet meshes are missing, verify socket names use the configured prefix.

---

## Base Physics Object (`BP_TFA_BasePhysicsObject`)

**Asset path:** `/Game/InfimaGames/TacticalFPSAnimations/Common/Core/Weapons/BP_TFA_BasePhysicsObject`

Shared base class for physics props (dropped magazines, ejected casings) — provides destroy-timer handling and impact-sound behavior so child Blueprints stay lightweight.

**Key variables:** `ImpactSound` (`SoundBase`), `Time Until Destroyed` (`float`), `bPlaySoundOnce` (`bool`), `MinimumForceToPlaySound` (`float`, default `100.0`).

**BeginPlay:** calls `SetLifeSpan(Time Until Destroyed)`. If `Time Until Destroyed = 0.0`, Unreal clears the lifespan timer — the actor persists indefinitely unless something else destroys it.

**Impact sound logic:** hit event bound to `SM_PshyicsObject` (this exact, misspelled component name is baked into the shipped asset). If `bPlaySoundOnce = true`, uses `DoOnce` to avoid spam and plays `ImpactSound` at the object's world location. **If `bPlaySoundOnce = false`, the force-threshold branch (`NormalImpulse` vs. `MinimumForceToPlaySound`) currently does not play anything** — in the shipped default, only the `bPlaySoundOnce = true` path is functional. Add your own threshold-based logic in a child class if you want that behavior.

---

## Physics Magazine (`BP_TFA_PhysicsMagazine`)

**Asset path:** `/Game/InfimaGames/TacticalFPSAnimations/Common/Core/Weapons/BP_TFA_PhysicsMagazine`

Dropped-magazine physics prop, spawned by **Drop Magazine** (`AN_TFA_DropMagazine`) and by `BP_TFA_BaseWeapon::SpawnDroppedMagazine`. Exists so reload animations can throw a magazine into the world without baking it into the weapon mesh.

**Behavior:** intentionally thin — its own events are disabled; all shared behavior (destroy timing, hit audio) comes from `BP_TFA_BasePhysicsObject`.

**Defaults:** `InitialLifeSpan = 120s`; `ImpactSound = None` by default (the weapon assigns it at spawn time).

**Runtime assignment (done by the weapon at spawn):** `Mesh = CurrentConfig.MeshMagazine`; `ImpactSound = CurrentConfig.SoundCue_WEP_MagDrop`.

---

## Physics Casing (`BP_TFA_PhysicsCasing`)

**Asset path:** `/Game/InfimaGames/TacticalFPSAnimations/Common/Core/Weapons/BP_TFA_PhysicsCasing`

Ejected-casing physics prop, spawned by **Eject Casing** (`AN_TFA_EjectCasing`) and by `BP_TFA_BaseWeapon::EjectCasing`. Spawned at the receiver's ejection socket, then pushed with linear + angular impulse.

**Behavior:** local event logic disabled — inherits from `BP_TFA_BasePhysicsObject` exactly like the magazine does.

**Defaults:** `InitialLifeSpan = 30s`; `ImpactSound = None` by default.

**Runtime assignment (done by the weapon at spawn):** `Mesh = CurrentConfig.MeshBulletCasing`; `ImpactSound = CurrentConfig.SoundCue_WEP_CasingEject`.

---

## Laser Attachment (`BP_TFA_Attachment_Laser`)

**Asset path:** `/Game/InfimaGames/TacticalFPSAnimations/Common/Core/Weapons/BP_TFA_Attachment_Laser`

Cosmetic laser attachment, spawned by `BP_TFA_BaseWeapon` on BeginPlay only when `SocketLaserAttachment` exists.

**BeginPlay:** gets the player character, casts to `BP_TFA_BaseCharacter`, copies `WeaponConfig` from it, assigns `SM_LaserAttachment` from `WeaponConfig.MeshLaserAttachment`, reads the beam-start transform from `SM_LaserAttachment.GetSocketTransform(WeaponConfig.SocketLaserStart, RTS_Component)`.

**Tick:** if `WeaponConfig` is valid, traces forward `15000` units from `SocketLaserStart`, scales `SM_LaserBeam` on its X axis using the hit distance, moves the point light to the impact point, enables/disables the light based on whether the trace hit anything. If `WeaponConfig` is invalid, runs a `DoOnce` and disables both beam and light.

**Config fields used:** `MeshLaserAttachment`, `SocketLaserStart`.

**Notes:** socket placement drives beam direction — if the laser looks misaligned, tune the socket on the attachment mesh first.

## Checklist

- [ ] Confirm magazine actors correctly build their bullet-socket visualization from the magazine skeleton.
- [ ] Confirm dropped-magazine and casing physics props inherit lifespan/impact-audio correctly from `BasePhysicsObject`.
- [ ] Test the laser attachment only spawns when the relevant socket exists, and that the beam correctly scales to hit distance.
- [ ] If you want threshold-based (rather than always-on) impact sounds, extend `BasePhysicsObject`'s hit-event logic yourself — the shipped default doesn't implement that branch.
