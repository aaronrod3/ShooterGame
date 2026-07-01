# ShooterGame — Infima Pack Integration Checklist (Project-Mapped)

> This checklist has been adapted from the original Infima Games Tactical FPS Animation Pack documentation to match ShooterGame's actual asset names, folder structure, and skeleton. Items marked **[TRANSFER]** need to be moved/duplicated from the Infima Pack into your own project folders. Items marked **[CREATE]** don't exist yet in your project and need to be built. Items marked **[VERIFY]** exist in some form and need a live in-editor confirmation pass.

## 1. Skeleton and Character Retarget

- [x] Your character uses **SK_Mannequin** — the standard UE5 skeleton. This is the same skeleton/bone hierarchy the Infima Pack targets, so no skeleton reassignment or bone-matching pass is required. **[VERIFIED BY USER]**
- [ ] Confirm your player character mesh is correctly assigned to `SK_Mannequin` in the Skeletal Mesh asset (Skeleton field), and that `BP_PlayerCharacter` references that mesh. **[VERIFY]**
- [ ] Required FABRIK IK bone chains (standard on SK_Mannequin, should already be present):
  - Right arm: `clavicle_r` → `hand_r` → `ik_hand_r` (effector)
  - Left arm: `clavicle_l` → `hand_l` → `ik_hand_l` (effector) **[VERIFY]**
- [ ] Open your weapon config data asset: `DA_RifleBaseConfig` (your project's equivalent of the Infima weapon Data Asset). **[VERIFY LOCATION]**
- [ ] Expand the Character Mesh fields on `DA_RifleBaseConfig` and confirm FP and TP mesh references point to your actual character mesh(es), not Infima demo meshes. **[VERIFY]**
- [ ] Test in a PIE session using `BP_PlayerCharacter` + `BP_Rifle` together, since there is no Infima demo map to rely on anymore. **[VERIFY]**
- [ ] For custom weapon meshes on `BP_Rifle`: confirm the receiver skeletal mesh was imported with the correct weapon skeleton selected on import (per your rifle's specific rig), so animations play without needing retarget. **[VERIFY]**

## 2. Animation Blueprint Setup

### Shared AnimBP Dependencies (FP and TP)
- [ ] Owning pawn must be `BP_PlayerCharacter` (your equivalent of `BP_TFA_BaseCharacter`). **[VERIFY]**
- [ ] AnimBP should implement an Animation State interface. You currently have no equivalent to `BPI_TFA_AnimationState` — either reuse the Infima interface directly, or recreate a minimal version with a single `UpdateLeftHandGrip(bool IsLeftHandOnWeapon, float BlendSpeed)` function. **[CREATE or REUSE]**
- [ ] Skeleton must include FABRIK chain bones: `ik_hand_r`, `hand_r`, `clavicle_r`, `ik_hand_l`, `hand_l`, `clavicle_l`. Since you're on standard `SK_Mannequin`, these should already exist. **[VERIFY]**

### ABP_FP_Default (First-Person — your equivalent of ABP_TFA_FP_BaseCharacter)
- [ ] On BlueprintInitializeAnimation: call TryGetPawnOwner, Cast to `BP_PlayerCharacter`, store result in a CharacterBP variable. **[VERIFY/BUILD]**
- [ ] On BlueprintUpdateAnimation: build InputMoveVector from your character's velocity, copy state booleans (bIsRunning, bIsWalking, bIsSprinting, bIsAiming, bAnimateCamera), copy Stance and CurrentGrip, copy procedural transforms (AimDownSightsTransform, RecoilTransform, CrouchTransform), then call an Interpolate Grip Alpha function. **[VERIFY/BUILD]**
- [ ] AnimGraph evaluation order (must be preserved):
  1. Build locomotion base pose (driven by InputMoveVector, bIsWalking/bIsRunning/bIsSprinting, Stance) — cache with UseCachedPose
  2. Apply mesh-space additive layers for ADS, recoil, crouch (Additive Type: Mesh Space, Base Pose: Reference Pose, Root Motion disabled)
  3. Run FABRIK hand IK node on both arms (Effector Transform Space: Bone Space, expose LODThreshold)
  4. Optionally blend camera/head motion via LayeredBoneBlend node on the head branch, controlled by bAnimateCamera **[CREATE — needs to be built in ABP_FP_Default]**
- [ ] Grip overlay nodes: CurrentGrip selects grip pose, CurrentGripAlpha (driven by FInterpTo using GripBlendSpeed) controls overlay blend weight. **[CREATE]**
- [ ] Notify hook: implement an Unlock Actions notify that fires `CharacterBP.bIsBusy = false`. Can reuse Infima's `AN_TFA_UnlockActions` logic pattern or recreate as your own notify class. **[CREATE or REUSE]**
- [ ] Integration checklist:
  - Mesh's Anim Class = `ABP_FP_Default`
  - AnimBP implements your Animation State interface
  - Montages include Left Hand Grip notify-state windows where needed
  - `BP_PlayerCharacter` updates SimulatedVelocity, ADS/recoil/crouch transforms, CurrentGrip **[VERIFY]**

### ABP_TP_Default (Third-Person — your equivalent of ABP_TFA_TP_BaseCharacter)
- [ ] On BlueprintInitializeAnimation: TryGetPawnOwner → Cast to `BP_PlayerCharacter` → store in CharacterBP. **[VERIFY/BUILD]**
- [ ] On BlueprintUpdateAnimation: copy CharacterBP.bIsAiming → bIsAiming, CharacterBP.CurrentGrip → CurrentGrip, CharacterBP.CurrentRecoil → RecoilTransform; interpolate CurrentGripAlpha via FInterpTo using GripBlendSpeed. **[VERIFY/BUILD]**
- [ ] Stance Blend node: BlendListByBool driven by bIsAiming (Aim-in blend time 0.2, Aim-out blend time 0.2). **[CREATE — confirm this node exists in ABP_TP_Default]**
- [ ] Aiming Transitions: mesh-space additive state machine layered on top of stance pose, with states Default / Aim Start / Aim End, bound to your rifle's aim-transition animation assets (retargeted or repurposed from Infima's TP_Transition_AimStart / TP_Transition_AimEnd). **[CREATE]**
- [ ] Breathing/Idle Additive: looping sequence applied additively, bound to your rifle's idle loop animation. **[CREATE]**
- [ ] Montage Slot nodes: two slots — one for aiming montages (slot name: Aiming), one for hip-fire montages. **[VERIFY/BUILD]**
- [ ] Hand IK with FABRIK node (component space):
  - Right arm: `clavicle_r` → `hand_r`, effector `ik_hand_r`
  - Left arm: `clavicle_l` → `hand_l`, effector `ik_hand_l`
  - Both nodes: Effector Transform Space = Bone Space, Precision = 0.01, Effector Rotation Source = CopyFromTarget, expose LODThreshold **[VERIFY node exists and settings match once built]**
- [ ] Notify hook: Unlock Actions notify sets `CharacterBP.bIsBusy = false`. **[CREATE or REUSE]**
- [ ] Integration checklist:
  - Mesh's Anim Class = `ABP_TP_Default`
  - Owning pawn is `BP_PlayerCharacter`
  - AnimBP implements your Animation State interface
  - Montages include Left Hand Grip notify-state windows where expected
  - `DA_RifleBaseConfig` provides TP idle pose, TP idle loop, TP aim pose, TP aim-start/aim-end transitions **[VERIFY these fields exist on DA_RifleBaseConfig]**

### Weapon and Magazine AnimBPs
- [ ] Weapon AnimBP (the `ABPWeapon` field on `DA_RifleBaseConfig`): none exists yet in your project. Either reuse an Infima weapon AnimBP as a starting template, or create a new one from scratch and assign it. **[CREATE]**
- [ ] Magazine AnimBP (the `ABPMagazine` field): same situation — none exists yet. Reuse or recreate, then assign. **[CREATE]**

### Left Hand Grip Flow (both AnimBPs)
- [ ] A Left Hand Grip notify-state should call `UpdateLeftHandGrip(bool IsLeftHandOnWeapon, float BlendSpeed)` on the active anim instance through your Animation State interface. **[CREATE — pattern from Infima's ANS_TFA_LeftHandGrip]**
- [ ] AnimBP stores incoming values into bIsLeftHandOnWeapon / GripBlendSpeed, then smooths CurrentGripAlpha toward 1.0 or 0.0 using FInterpTo and GetWorldDeltaSeconds. **[CREATE]**

## 3. Notify and Notify-State Wiring

All notify and notify-state classes below can be **recreated as your own classes** (recommended for clean project ownership) rather than reusing Infima's directly, using the same logic pattern. Naming suggestion follows your project convention (`AN_` / `ANS_` prefix, no `TFA` tag needed since these become yours).

### AnimNotify (One-Shot) Classes — recreate as your own

| Suggested Name | What It Should Do | Calls Into |
|---|---|---|
| `AN_DropMagazine` | Casts owner to weapon, calls Weapon.SpawnDroppedMagazine(ImpulseForce, RotationForce) at the release frame; weapon spawns a physics magazine actor with linear/angular impulse | `BP_Rifle` |
| `AN_EjectCasing` | Casts owner to weapon, builds randomized RotationOffset, calls Weapon.EjectCasing(RotationOffset, MinEjectForce, MaxEjectForce, RotationSpeed) | `BP_Rifle` |
| `AN_SpawnObjectAttached` | Casts owner to character, spawns and attaches a prop tagged DisposableItem (default socket ik_hand_l) | `BP_PlayerCharacter` |
| `AN_ThrowPhysicsObject` | Casts owner to character, applies linear/angular impulse and optionally clears DisposableItem-tagged actors | `BP_PlayerCharacter` |
| `AN_UnlockActions` | Casts owner to character, sets Character.bIsBusy = false, reopening gameplay input | `BP_PlayerCharacter` |

### AnimNotifyState (Time-Window) Classes — recreate as your own

| Suggested Name | What It Should Do | Calls Into |
|---|---|---|
| `ANS_BlockADS` | Begin: casts to character, sets bIsAimingBlocked = true, calls ForceStopAiming(). End: sets bIsAimingBlocked = false | `BP_PlayerCharacter` |
| `ANS_HideMainMag` | Begin: casts to weapon, hides main magazine mesh. End: shows it again | `BP_Rifle` |
| `ANS_ShowReserveMag` | Begin: casts to weapon, shows reserve magazine mesh. End: hides it | `BP_Rifle` |
| `ANS_LeftHandGrip` | Begin: sets IsLeftHandOnWeapon=false via your Animation State interface. End: sets it back to true | Your Animation State interface (implemented on `ABP_FP_Default` / `ABP_TP_Default`) |

### Interface
- [ ] Create your own Animation State interface (equivalent to `BPI_TFA_AnimationState`) with a single function `UpdateLeftHandGrip(bool IsLeftHandOnWeapon, float BlendSpeed)`. Implement it on both `ABP_FP_Default` and `ABP_TP_Default`. **[CREATE]**

### Owner Requirements (apply same logic to your recreated notifies)
- [ ] `AN_DropMagazine`, `AN_EjectCasing`, `ANS_HideMainMag`, `ANS_ShowReserveMag` — should only function on montages playing on a mesh owned by `BP_Rifle`.
- [ ] `AN_SpawnObjectAttached`, `AN_ThrowPhysicsObject`, `AN_UnlockActions`, `ANS_BlockADS` — should only function on montages playing on a mesh owned by `BP_PlayerCharacter`.
- [ ] `ANS_LeftHandGrip` — should only function if the active anim instance implements your Animation State interface.

### Default Notify Parameter Values (starting reference — tune to your rifle's feel)
- [ ] `AN_DropMagazine`: ImpulseForce = -50.0, RotationForce = 150.0
- [ ] `AN_EjectCasing`: MinEjectForce = 50.0, MaxEjectForce = 65.0, RotationSpeed = 0.0
- [ ] `AN_SpawnObjectAttached`: SpawnSocketName = ik_hand_l, LocationOffset = (0,0,0), RotationOffset = (0,0,0), VisibilityDelay = 0.0
- [ ] `AN_ThrowPhysicsObject`: SpawnSocketName = ik_hand_l, ThrowForce = 80.0, ThrowRotationForce = 80.0, DestroySocketItem = true
- [ ] `ANS_LeftHandGrip`: BlendOutSpeed = 15.0, BlendReturnSpeed = 15.0

## 4. Supporting Actor Classes to Transfer

These Infima Pack Blueprints have no equivalent in your project yet and should be **copied/migrated directly into your own folder structure** (e.g. `Content/ShooterGame/Weapons/Physics/`) rather than left inside the Infima plugin folder, so they survive a future demo-content cleanup pass:

- [ ] `BP_TFA_BaseMagazine` → migrate into your project folder, rename if desired (e.g. `BP_MagazineCosmetic`) **[TRANSFER]**
- [ ] `BP_TFA_BasePhysicsObject` → migrate, this is the shared base class for dropped physics props **[TRANSFER]**
- [ ] `BP_TFA_PhysicsMagazine` → migrate, used by `AN_DropMagazine` **[TRANSFER]**
- [ ] `BP_TFA_PhysicsCasing` → migrate, used by `AN_EjectCasing` **[TRANSFER]**
- [ ] `BP_TFA_AttachmentLaser` → migrate if you plan to support laser attachments on `BP_Rifle` **[TRANSFER]**

When migrating, use Unreal's **Migrate** tool (right-click asset → Asset Actions → Migrate) to bring each Blueprint and its dependencies into your project folder cleanly, then fix up any broken references in `BP_Rifle` and `DA_RifleBaseConfig` afterward.

## 5. Bones, Sockets, and AnimGraph Verification

- [x] All bones match standard UE5 Manny (`SK_Mannequin`) — no bone remapping needed. **[VERIFIED BY USER]**
- [ ] Sockets referenced throughout this checklist (`SocketMagazineAttachment`, `SocketMagazineReserveAttachment`, `SocketCasingEject`, `SocketGripAttachment`, `SocketLaserAttachment`, `SocketGunAttachment`, `SocketGunCamera`, `SocketHelmetCamera`, `SocketChestCamera`, `SocketMuzzle`) either already exist on your rifle/character mesh or need to be added via the Skeleton/Skeletal Mesh Editor socket tools. **[VERIFY or CREATE per-socket]**
- [ ] Every AnimGraph node referenced in Section 2 (FABRIK hand IK, Layered Blend Per Bone, additive stack, state machines, montage slots) needs to be manually built and verified inside `ABP_FP_Default` and `ABP_TP_Default`, since these are new AnimBPs rather than the Infima originals. **[CREATE + VERIFY]**

## 6. UE5.7 IK Retargeter Workflow (Reference Only — Likely Not Needed)

Since your character uses the standard `SK_Mannequin` skeleton — the same skeleton the Infima Pack targets — **you likely do not need to run an IK Retargeter pass at all**. Retargeting is only required when moving animations between two *different* skeletons with mismatched bone hierarchies. Keep this section only as a fallback reference if you later introduce a custom character skeleton that diverges from standard Manny.

- [ ] If a custom skeleton is introduced later: create IK Rig assets for both source and target skeletons, set Retarget Root to pelvis, and explicitly bind the Arm_L/Arm_R IK Goals to `ik_hand_l`/`ik_hand_r` rather than `hand_l`/`hand_r`.
- [ ] If retargeting becomes necessary: use "Blend To Source" = 1.0 on Arm chains for weapon-holding animations specifically, to preserve exact hand-to-weapon-grip alignment, and reset to 0 for pure locomotion clips.
