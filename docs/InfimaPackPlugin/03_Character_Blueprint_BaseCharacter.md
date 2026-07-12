# Step 3 — Character Blueprint: Base Character (`BP_TFA_BaseCharacter`)

**Asset path:** `/Game/InfimaGames/TacticalFPSAnimations/Common/Core/Character/BP_TFA_BaseCharacter`

This is the demo's central gameplay hub — camera switching, aiming, recoil, weapon ownership, and temporary animation props. Most animation notifies target this Blueprint directly. If an action is blocked, a prop won't spawn, or a camera mode misbehaves, this is the first place to check.

## Key dependencies

- **Base Config** (`BP_TFA_BaseConfig`) — assigned to `WeaponConfig`; drives meshes, sockets, montages, offsets, sounds.
- **Base Weapon** (`BP_TFA_BaseWeapon`) — spawned into `CurrentWeaponActor`; provides weapon sockets (muzzle, gun camera).
- UI widgets: `WBP_TFA_Demo` (demo warnings/status), `WBP_TFA_BodycamOverlay` (bodycam angle text).

## Key state and default values

| Category | Values |
|---|---|
| Third-person camera distance | `InitialCameraDistance = 140`, `MinCameraDistance = 65`, `MaxCameraDistance = 270`, `ZoomStep = 50` |
| First-person FOV | `DefaultFOV = 100`, `AimedFOV = 78` |
| Action state flags | `bIsBusy` blocks most actions; `bIsRunning`/`bIsSprinting` gate ADS/firing/crouch/jump; `bIsAimingBlocked` is driven by notify states during ADS lock windows |
| Procedural transform pairs | ADS: `TargetAimDownSightsOffset` → `CurrentAimDownSightsOffset`; Recoil: `TargetRecoil` → `CurrentRecoil`; Crouch: `TargetCrouchOffset` → `CurrentCrouchOffset` |

## BeginPlay and setup

1. **`BeginPlay` → `StartScreenFaded`**: calls Start Camera Fade with `FromAlpha = 1`, `ToAlpha = 1`, `bHoldWhenFinished = true` — the camera starts fully faded out; something else in your demo flow is expected to fade it back in.
2. **`InitialSetup`** (bootstrap helper):
   - Disables input at startup.
   - Runs `t.MaxFPS 999` and `r.ScreenPercentage 100`.
   - Adds the enhanced input mapping context `IMC_TFA_Default`.
   - Validates `WeaponConfig.FP_Mesh` before assigning it to `Mesh` — prints a hard error if invalid.
3. **`SpawnUI`**: creates and adds `WBP_TFA_Demo` (stored as `WidgetBP`) and `WBP_TFA_BodycamOverlay` (stored as `WidgetBodycamOverlay`) to the viewport.

## Per-frame tick flow (order matters)

Runs this fixed sequence every tick, in this order, to keep the camera and procedural offsets stable during perspective changes:

1. **`SimulateVelocity`** — since the demo character is intentionally stationary, this simulates a movement velocity for FP animation preview: builds a target direction from `RawInputVector` (as `FVector2D`) via `Normalize2D`, uses speed `100` or `200` depending on `bIsRunning`, forces `Z = 250`, smooths into `SimulatedVelocity` via `VInterpTo` with `InterpSpeed = 15`.
2. **`ThirdPersonCameraUpdate`** — when `CurrentCameraPerspective == ThirdPerson`, smoothly interpolates `SpringArm.TargetArmLength` toward `InitialCameraDistance` and moves `Camera_Pivot_Point` toward the weapon attachment socket.
3. **`SetCapsuleHalfHeight`** — despite the name, adjusts the **mesh's relative location**, not the capsule. In first person, interpolates the mesh location over time; in other modes, snaps the mesh to `Z = -88`.
4. **Procedural Offsets** — updates three spring-driven layers in sequence via `CalculateSpring`: Crouch (`TargetCrouchOffset → CurrentCrouchOffset`), ADS (`TargetAimDownSightsOffset → CurrentAimDownSightsOffset`), Recoil (`TargetRecoil → CurrentRecoil`).

## Weapon setup

- **`SpawnWeapon`**: spawns `BP_TFA_BaseWeapon`, stores it in `CurrentWeaponActor`, passes `WeaponConfig` into the spawned weapon as `CurrentConfig`, attaches the weapon actor to the character mesh at `WeaponConfig.SocketGunAttachment`.

## Camera perspectives

Cycled via `ToggleCameraPerspective` (advances the enum) and applied via `UpdateCameraPerspective` (switches on `E_TFA_CameraPerspectives` and calls the matching `Enable...` function). When switching back to first person, also calls `StopWeaponAnimation` and `DestroyAttachedItem` to clear visual leftovers from other perspectives. `UpdateCameraPerspective` also refreshes the bodycam overlay angle text.

| Mode | What it does |
|---|---|
| **First Person** (`EnableFirstPersonPerspective`) | `Mesh = WeaponConfig.FP_Mesh`; activate `Camera_FP`, deactivate `Camera_TP`; assign `ABP_TFA_FP_BaseCharacter`; hide the head bone (avoid clipping); disable `PP_Bodycam`; remove the muffled submix effect `SEF_TFA_Muffled` |
| **Third Person** (`EnableThirdPersonPerspective`) | `Mesh = WeaponConfig.TP_Mesh`; activate `Camera_TP`, deactivate `Camera_FP`; assign `ABP_TFA_TP_BaseCharacter`; attach `Camera_TP` to the SpringArm; FOV `105`; unhide the head bone; remove the muffled submix effect |
| **Gun Camera** (`EnableGunCameraPerspective`) | Still uses the third-person camera component, but attaches it to `CurrentWeaponActor.SK_Receiver` at `WeaponConfig.SocketGunCamera`; enables `PP_Bodycam`; FOV `120`; adds the muffled submix effect |
| **Bodycam** (`EnableBodycamPerspective`) | Same lens treatment as gun cam, but attaches `Camera_TP` to the character mesh instead, at `WeaponConfig.SocketHelmetCamera` or `SocketChestCamera` depending on the selected angle; FOV `120`; adds the muffled submix effect |

Third-person camera helper functions (active only while in third person): `ThirdPersonCameraUpdate` (spring arm length + pivot interpolation), `ThirdPersonCameraLook` (pivot rotation, pitch clamped to `[-80, 80]`, accumulating yaw), `ThirdPersonCameraZoom` (adjusts `InitialCameraDistance` by `ZoomStep`, clamped to `[MinCameraDistance, MaxCameraDistance]`).

## Procedural pose offsets and the aiming flow

Target transforms plus spring smoothing drive ADS, recoil, and crouch — all updated in **Procedural Offsets** via **`CalculateSpring`**, which builds a transform from a Vector Spring Interp for location and another for rotation (converted to a rotator). Separate spring state is kept per layer: `SpringCrouch`, `SpringAimDownSights`, `SpringRecoil`.

**Aiming flow:** gated off if `bIsRunning`, `bIsSprinting`, or `bIsAimingBlocked` is true. If allowed: pulls the target transform from `WeaponConfig.OffsetAimDownSights`, runs `TL_ADS_FOV` to blend `Camera_FP.FieldOfView` from `DefaultFOV` to `AimedFOV`. On ADS start: sets `bIsAiming = true`, applies `TargetAimDownSightsOffset`, optionally clears crouch offsets to avoid stacking, plays `WeaponConfig.SoundCue_WEP_AimIn` through a `DoOnce` gate. On ADS stop (or when `ForceStopAiming` interrupts it): sets `bIsAiming = false`, resets ADS offsets to identity, reapplies `WeaponConfig.OffsetCrouch` if currently crouched.

## Firing, fire modes, recoil, VFX

- **`Fire`**: gated by `!bIsRunning && !bIsSprinting && !bIsBusy`. Plays a synced montage set keyed on `CurrentFireMode`. If `CurrentFireMode != Safety`: calls `AddRecoil` and `SpawnMuzzleFlash`, subtracts `1` from `CurrentWeaponActor.AmmoCount` (a cosmetic value driving the animated magazine, not a real ammo system). Safety mode plays the empty/dry-fire montage instead.
- **`FireModeSwitch`**: if not busy, cycles `Safety → Semi → Auto → Safety`, plays the fire-mode-switch montage, calls `WidgetBP.ShowFireModeText`.
- **`AddRecoil`**: builds the target transform later spring-smoothed in tick. `RecoilRampCount` increases per shot; translation/rotation pull from random ranges plus clamps; components scale with `Clamp(float(RecoilRampCount), 5..25)` — recoil ramps up over the first few shots then caps.
- **Recoil decay**: every tick, `TargetRecoil` interpolates back to identity via `TInterpTo` with `InterpSpeed = 22`; `DeltaTime` clamped to `<= 0.016` to avoid big jumps during frame hitches.
- **`SpawnMuzzleFlash`**: spawns `NS_TFA_MuzzleFlash` attached to `CurrentWeaponActor.SK_Receiver` at `WeaponConfig.SocketMuzzle`.

## Action state and notify hooks

- **`bIsBusy`** — global action lock; cleared by **Unlock Actions** (`AN_TFA_UnlockActions`).
- **`bIsAimingBlocked`** — gates ADS; set/cleared by **Block ADS** (`ANS_TFA_BlockADS`).

## Demo stance limitations (intentional, not bugs)

This Blueprint deliberately limits locomotion outside first person — a demo constraint, since the pack focuses on animation preview/presentation, not full third-person locomotion.

- **`ToggleCrouch`**: outside first person, calls `WidgetBP.ShowThirdPersonWarning` and exits. In first person, still blocks crouch while running/sprinting. When allowed, toggles stance, applying `WeaponConfig.OffsetCrouch` on crouch start and resetting to identity on stand.
- **`PlayJump`**: outside first person, shows the third-person warning. Otherwise allowed only when standing and not busy/running/sprinting; plays `WeaponConfig.FP_JumpFull`.

## Notify-driven functions

- **`ForceStopAiming`** — called by **Block ADS** (`ANS_TFA_BlockADS`) on notify begin. Routes into the aiming logic, sets `bIsAiming = false`, resets ADS procedural offsets. The begin of `ANS_TFA_BlockADS` both blocks ADS and forces an immediate exit from it.
- **`ThrowPhysicsObject(...)`** — called by **Throw Physics Object** (`AN_TFA_ThrowPhysicsObject`). Spawns the notify-provided actor class at a socket-derived transform, applies linear/angular impulse with `bVelChange = true`, and optionally destroys previously-attached actors tagged `DisposableItem` when `bClearSocketItem` is true (despite the notify parameter being named `DestroySocketItem`, the actual behavior removes *all* previously-attached disposable props, not one named socket item).
- **`DestroyAttachedItem`** — not itself a notify target, but called during camera perspective switches to remove attached actors tagged `DisposableItem`.
- **`StopWeaponAnimation`** — stops the montage currently playing on the weapon's anim instance and clears `bIsBusy = false`.

## Tag conventions

- **`DisposableItem`** — marks temporary spawned/attached props. Used by `DestroyAttachedItem` and the cleanup path inside `ThrowPhysicsObject`.

## Common gotchas

| Symptom | Likely cause |
|---|---|
| Bodycam or gun cam looks wrong (stuck at origin) | Bad `SocketGunCamera`/`SocketHelmetCamera`/`SocketChestCamera` |
| ADS offset feels wrong after switching perspectives | Missing call to `ForceStopAiming` during a montage window that should block ADS |
| Muzzle flash doesn't spawn | `WeaponConfig.SocketMuzzle` doesn't exist on the receiver skeleton |

## Checklist

- [ ] Confirm `WeaponConfig` and eventual `CurrentWeaponActor` references are set.
- [ ] Confirm `InitialSetup` successfully assigns `WeaponConfig.FP_Mesh` (no hard error in the log).
- [ ] Verify all four camera perspectives switch cleanly and the bodycam overlay text updates.
- [ ] Verify recoil ramps up over consecutive shots and decays back to identity between bursts.
- [ ] Verify `bIsBusy`/`bIsAimingBlocked` correctly gate actions and get cleared by their respective notify classes (see [08_Animation_Notifies_And_States.md](08_Animation_Notifies_And_States.md)).
