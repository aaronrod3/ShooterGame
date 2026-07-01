\# Infima Games Tactical FPS Animation Pack — UE5 Integration Checklist

## 1. Skeleton and Character Retarget

- \[ ] Confirm your custom character mesh is rigged to UE5 Manny with a matching bone hierarchy before attempting retarget \[file:3].
- \[ ] If using Fab content, enable the plugin first: Edit → Plugins → Fab \[file:3].
- \[ ] Locate your custom skeletal mesh (e.g. AAAModularSoldier > Modules > ModularSoldierVol\_1 > Meshes\_UE5, or your own import path) \[file:3].
- \[ ] Duplicate the skeletal mesh before editing it, to preserve a clean rollback copy \[file:3].
- \[ ] Right-click the skeletal mesh asset → Skeleton > Assign Skeleton \[file:3].
- \[ ] Search for and select SKEL\_TFA\_Mannequin in the skeleton picker \[file:3].
- \[ ] Verify that all bones match between your custom mesh and SKEL\_TFA\_Mannequin before clicking Accept. If bones are missing or mismatched, the mesh is not Manny-compatible — fix the rig or match the hierarchy and retry \[file:3].
- \[ ] Required FABRIK IK bone chains that must exist on the assigned skeleton:
  - Right arm: clavicle\_r → hand\_r → ik\_hand\_r (effector)
  - Left arm: clavicle\_l → hand\_l → ik\_hand\_l (effector) \[file:2]
- \[ ] Open the weapon's config data asset at: Content/InfimaGames/TacticalFPSAnimations/Weapons/<WeaponName>/Demo/Data \[file:3].
- \[ ] Expand Meshes > Character and replace both the FP\_Mesh and TP\_Mesh references with your custom character mesh \[file:3]\[file:4].
- \[ ] If the mesh does not appear in the picker, assignment failed — repeat skeleton assignment and re-verify bone matching \[file:3].
- \[ ] Test the result by playing a demo map (e.g. L\_TFA\_AR\_Showcase, named per weapon product) \[file:3].
- \[ ] For custom weapon meshes: import the receiver skeletal mesh and select the matching weapon skeleton on import (e.g. SKEL\_TFA\_AR for assault rifle, SKEL\_TFA\_WeaponName pattern for other weapons), otherwise animations will not play without retargeting \[file:3].

## 2. Animation Blueprint Setup

### Shared AnimBP Dependencies (FP and TP)
- \[ ] Owning pawn must be BP\_TFA\_BaseCharacter or a child class \[file:2].
- \[ ] AnimBP must implement BPI\_TFA\_AnimationState (Animation State Interface) \[file:2].
- \[ ] Skeleton must include FABRIK chain bones: ik\_hand\_r, hand\_r, clavicle\_r, ik\_hand\_l, hand\_l, clavicle\_l. Missing bones cause floaty hands and non-functional grip blending \[file:2].

### ABP\_TFA\_FP\_BaseCharacter (First-Person)
- Asset path: /Game/InfimaGames/TacticalFPSAnimations/Common/Core/Characters/ABP\_TFA\_FP\_BaseCharacter \[file:2]
- \[ ] On BlueprintInitializeAnimation: call TryGetPawnOwner, Cast to BP\_TFA\_BaseCharacter, store result in CharacterBP variable \[file:2].
- \[ ] On BlueprintUpdateAnimation: build InputMoveVector from CharacterBP.SimulatedVelocity.XY, copy state booleans (bIsRunning, bIsWalking, bIsSprinting, bIsAiming, bAnimateCamera), copy Stance and CurrentGrip, copy procedural transforms (AimDownSightsTransform, RecoilTransform, CrouchTransform), then call Interpolate Grip Alpha \[file:2].
- \[ ] AnimGraph evaluation order (must be preserved):
  1. Build locomotion base pose (driven by InputMoveVector, bIsWalking/bIsRunning/bIsSprinting, Stance) — cache with UseCachedPose
  2. Apply mesh-space additive layers for ADS, recoil, crouch (Additive Type: Mesh Space, Base Pose: Reference Pose, Root Motion disabled)
  3. Run FABRIK hand IK node on both arms (Effector Transform Space: Bone Space, expose LODThreshold)
  4. Optionally blend camera/head motion via LayeredBoneBlend node on the head branch, controlled by bAnimateCamera (weight 0 = base pose head motion, weight 1 = reference pose override) \[file:2].
- \[ ] Grip overlay nodes: CurrentGrip selects grip pose, CurrentGripAlpha (driven by FInterpTo using GripBlendSpeed) controls overlay blend weight \[file:2].
- \[ ] Notify hook: AnimNotify\_AN\_UnlockActions implemented — fires CharacterBP.bIsBusy = false \[file:2].
- \[ ] Integration checklist:
  - Mesh's Anim Class = ABP\_TFA\_FP\_BaseCharacter
  - AnimBP implements BPI\_TFA\_AnimationState
  - Montages include ANS\_TFA\_LeftHandGrip windows where needed
  - BP\_TFA\_BaseCharacter updates SimulatedVelocity, ADS/recoil/crouch transforms, CurrentGrip \[file:2]

### ABP\_TFA\_TP\_BaseCharacter (Third-Person)
- Asset path: /Game/InfimaGames/TacticalFPSAnimations/Common/Core/Characters/ABP\_TFA\_TP\_BaseCharacter \[file:2]
- \[ ] On BlueprintInitializeAnimation: TryGetPawnOwner → Cast to BP\_TFA\_BaseCharacter → store in CharacterBP \[file:2].
- \[ ] On BlueprintUpdateAnimation: copy CharacterBP.bIsAiming → bIsAiming, CharacterBP.CurrentGrip → CurrentGrip, CharacterBP.CurrentRecoil → RecoilTransform; interpolate CurrentGripAlpha via FInterpTo using GripBlendSpeed \[file:2].
- \[ ] Stance Blend node: BlendListByBool driven by bIsAiming (Aim-in blend time 0.2, Aim-out blend time 0.2) \[file:2].
- \[ ] Aiming Transitions: mesh-space additive state machine named SM\_AimingTransitions, layered on top of stance pose, with states:
  - Default (reference pose)
  - Aim Start (bound to WeaponConfig.TP\_Transition\_AimStart)
  - Aim End (bound to WeaponConfig.TP\_Transition\_AimEnd)
  - Transition rules: Default→Aim Start (bIsAiming==true, crossfade 0.05, Sinusoidal); Aim Start→Aim End (bIsAiming==false, crossfade 0.15, QuadraticInOut); Aim End→Aim Start (bIsAiming==true, crossfade 0.05, Sinusoidal) \[file:2].
- \[ ] Breathing/Idle Additive: looping sequence bound to WeaponConfig.TP\_IdleLoop applied additively \[file:2].
- \[ ] Montage Slot nodes: two slots — one for aiming montages (slot name: Aiming), one for hip-fire montages — to prevent upper-body actions blending against the wrong base stance \[file:2].
- \[ ] Hand IK with FABRIK node (component space):
  - Right arm: clavicle\_r → hand\_r, effector ik\_hand\_r
  - Left arm: clavicle\_l → hand\_l, effector ik\_hand\_l
  - Both nodes: Effector Transform Space = Bone Space, Precision = 0.01, Effector Rotation Source = CopyFromTarget, expose LODThreshold \[file:2].
- \[ ] Notify hook: AnimNotify\_AN\_UnlockActions implemented — sets CharacterBP.bIsBusy = false \[file:2].
- \[ ] Integration checklist:
  - Mesh's Anim Class = ABP\_TFA\_TP\_BaseCharacter
  - Owning pawn is BP\_TFA\_BaseCharacter or child
  - AnimBP implements BPI\_TFA\_AnimationState
  - Montages include ANS\_TFA\_LeftHandGrip windows where expected
  - WeaponConfig provides: TP\_IdlePose, TP\_IdleLoop, TP\_AimPose, TP\_Transition\_AimStart, TP\_Transition\_AimEnd \[file:2].

### Left Hand Grip Flow (both AnimBPs)
- \[ ] ANS\_TFA\_LeftHandGrip calls BPI\_TFA\_AnimationState.UpdateLeftHandGrip(bool IsLeftHandOnWeapon, float BlendSpeed) on the active anim instance \[file:2].
- \[ ] AnimBP stores incoming values into bIsLeftHandOnWeapon / GripBlendSpeed, then smooths CurrentGripAlpha toward 1.0 or 0.0 using FInterpTo and GetWorldDeltaSeconds \[file:2].

## 3. Notify and Notify-State Wiring

### AnimNotify (One-Shot) Classes
| Notify Class | What It Does | Calls Into |
|---|---|---|
| AN\_TFA\_DropMagazine | Casts owner to weapon, calls Weapon.SpawnDroppedMagazine(ImpulseForce, RotationForce) at the release frame; weapon spawns BP\_TFA\_PhysicsMagazine at SocketMagazineAttachment with linear/angular impulse \[file:3]. | BP\_TFA\_BaseWeapon |
| AN\_TFA\_EjectCasing | Casts owner to weapon, builds randomized RotationOffset, calls Weapon.EjectCasing(RotationOffset, MinEjectForce, MaxEjectForce, RotationSpeed); spawns BP\_TFA\_PhysicsCasing at SocketCasingEject \[file:3]. | BP\_TFA\_BaseWeapon |
| AN\_TFA\_SpawnObjectAttached | Casts owner to character, calls Character.SpawnObjectAttached(ClassToSpawn, AnimationToPlay, SpawnSocketName, LocationOffset, RotationOffset, DelayBeforeVisible); spawns and attaches a prop tagged DisposableItem (default socket ik\_hand\_l) \[file:2]\[file:3]. | BP\_TFA\_BaseCharacter |
| AN\_TFA\_ThrowPhysicsObject | Casts owner to character, calls Character.ThrowPhysicsObject(ClassToSpawn, SpawnSocketName, LocationOffset, RotationOffset, ThrowForce, ThrowRotationForce, bClearSocketItem); applies linear/angular impulse and optionally clears DisposableItem-tagged actors \[file:2]\[file:3]. | BP\_TFA\_BaseCharacter |
| AN\_TFA\_UnlockActions | Casts owner to character, sets Character.bIsBusy = false, reopening gameplay input \[file:2]\[file:3]. | BP\_TFA\_BaseCharacter (also implemented as AnimNotify\_AN\_UnlockActions in both ABP\_TFA\_FP\_BaseCharacter and ABP\_TFA\_TP\_BaseCharacter) |

### AnimNotifyState (Time-Window) Classes
| Notify State Class | What It Does | Calls Into |
|---|---|---|
| ANS\_TFA\_BlockADS | Begin: casts to character, sets bIsAimingBlocked = true, calls Character.ForceStopAiming(). End: sets bIsAimingBlocked = false \[file:2]\[file:3]. | BP\_TFA\_BaseCharacter |
| ANS\_TFA\_HideMainMag | Begin: casts to weapon, calls Weapon.SetMagazineVisibility(bVisible=false, bIsReserve=false). End: calls SetMagazineVisibility(bVisible=true, bIsReserve=false) \[file:3]. | BP\_TFA\_BaseWeapon |
| ANS\_TFA\_ShowReserveMag | Begin: casts to weapon, calls Weapon.SetMagazineVisibility(bVisible=true, bIsReserve=true). End: calls SetMagazineVisibility(bVisible=false, bIsReserve=true) \[file:3]. | BP\_TFA\_BaseWeapon |
| ANS\_TFA\_LeftHandGrip | Begin: gets AnimInstance, calls UpdateLeftHandGrip(IsLeftHandOnWeapon=false, BlendSpeed=BlendOutSpeed). End: calls UpdateLeftHandGrip(IsLeftHandOnWeapon=true, BlendSpeed=BlendReturnSpeed) \[file:3]. | BPI\_TFA\_AnimationState interface (implemented on ABP\_TFA\_FP\_BaseCharacter / ABP\_TFA\_TP\_BaseCharacter) |

### Interface
- \[ ] BPI\_TFA\_AnimationState — contract with single function UpdateLeftHandGrip(bool IsLeftHandOnWeapon, float BlendSpeed); must be implemented by any AnimBP that should respond to ANS\_TFA\_LeftHandGrip \[file:2].

### Owner Requirements (verify before wiring notifies into your project)
- \[ ] AN\_TFA\_DropMagazine, AN\_TFA\_EjectCasing, ANS\_TFA\_HideMainMag, ANS\_TFA\_ShowReserveMag — only function on montages playing on a mesh owned by BP\_TFA\_BaseWeapon \[file:3].
- \[ ] AN\_TFA\_SpawnObjectAttached, AN\_TFA\_ThrowPhysicsObject, AN\_TFA\_UnlockActions, ANS\_TFA\_BlockADS — only function on montages playing on a mesh owned by BP\_TFA\_BaseCharacter \[file:3].
- \[ ] ANS\_TFA\_LeftHandGrip — only functions if the active anim instance implements BPI\_TFA\_AnimationState \[file:3].

### Default Notify Parameter Values (for reference during placement)
- \[ ] AN\_TFA\_DropMagazine: ImpulseForce = -50.0, RotationForce = 150.0 \[file:3].
- \[ ] AN\_TFA\_EjectCasing: MinEjectForce = 50.0, MaxEjectForce = 65.0, RotationSpeed = 0.0 \[file:3].
- \[ ] AN\_TFA\_SpawnObjectAttached: SpawnSocketName = ik\_hand\_l, LocationOffset = (0,0,0), RotationOffset = (0,0,0), VisibilityDelay = 0.0 \[file:3].
- \[ ] AN\_TFA\_ThrowPhysicsObject: SpawnSocketName = ik\_hand\_l, ThrowForce = 80.0, ThrowRotationForce = 80.0, DestroySocketItem = true \[file:3].
- \[ ] ANS\_TFA\_LeftHandGrip: BlendOutSpeed = 15.0, BlendReturnSpeed = 15.0 \[file:3].
```

## 4. UE5.7 IK Retargeter Workflow (FP Upper-Body → TP Skeleton)

### Why This Matters for the Infima Pack
- [ ] Infima's FP animations are authored as mesh-space additive layers on top of a FABRIK hand-pinned base, and the TP skeleton uses its own FABRIK chain (clavicle_l/r → hand_l/r → ik_hand_l/r). Retargeting FP clips directly onto the TP skeleton without correcting IK settings will desync the hand from the weapon grip the moment additive layers stack [file:2].

### IK Rig Setup (per skeleton)
- [ ] Create two IK Rig assets: one for SKEL_TFA_Mannequin (or FP skeleton variant) and one for your TP character skeleton, via Content Browser → right-click → Animation > IK Rig [web:19].
- [ ] Set the Retarget Root to pelvis on both rigs [web:19].
- [ ] Define matching bone chains on both rigs for: Spine, Head, Arm_L, Arm_R, Leg_L, Leg_R — chain names must match between source and target rigs for auto-mapping to work [web:19][web:16].
- [ ] On the Arm_L / Arm_R chains, explicitly set the IK Goal to the ik_hand_l / ik_hand_r bones (not hand_l/hand_r), since these are the effector bones the Infima FABRIK nodes read at runtime [file:2].

### IK Retargeter Asset Setup
- [ ] Create the IK Retargeter asset, assign the FP IK Rig as Source and the TP IK Rig as Target [web:19].
- [ ] In UE5.7, use the overhauled Chain Mapping tab — drag-and-drop or auto-detect common chains, then manually verify Arm_L/Arm_R and Spine mappings rather than trusting full auto-detection, since auto-detect can mismatch clavicle vs. shoulder roots on military-rig skeletons [web:8].
- [ ] Set the Retarget Pose on both source and target to match the actual bind/reference pose used by the Infima rig (not just a generic A-pose), to avoid arm-twist artifacts during retarget preview [web:19].

### Preventing Hand-to-Weapon IK Breakage
- [ ] Select the Arm_L and Arm_R chains in the Chain Mapping tab, then in the Details panel under IK Adjustments, set Blend To Source to 1.0. This forces the retargeted hand position to match the source (FP) animation's exact hand placement rather than an interpolated/scaled position — critical for keeping ik_hand_l/ik_hand_r aligned with weapon grip sockets [web:7].
- [ ] Use the Static Offset field under IK Adjustments to nudge residual clipping (e.g., trigger-finger or grip clipping) after Blend To Source is applied, rather than re-editing the source animation [web:7].
- [ ] Reset Blend To Source back to 0 for any retargeted clips that are pure locomotion or non-weapon poses — leaving it at 1 on non-grip animations can produce unnatural hand placement [web:7].
- [ ] After retargeting, re-verify in the ABP_TFA_TP_BaseCharacter AnimGraph that the FABRIK node's effector target still resolves to ik_hand_l / ik_hand_r in Bone Space, since a retarget pass can occasionally reset effector transform space to World if the retargeter output pose changed root scale [file:2].
- [ ] Test retargeted upper-body clips specifically during ADS and recoil additive stacking (not just idle), since Blend To Source errors typically only become visible once mesh-space additive layers are applied on top of the retargeted base pose [file:2][web:8].

### UE5.7-Specific Improvements to Leverage
- [ ] Enable the new Pose Correction Layer (introduced in UE5.7) to automatically reduce shoulder collapse and elbow hyperextension artifacts that are common when retargeting narrow FP-mannequin proportions onto a bulkier military-style TP skeleton [web:8].
- [ ] Take advantage of UE5.7's faster runtime retargeting (~30-40% faster than UE5.4) if you plan to retarget at runtime for multiple TP character variants sharing the same FP animation set, rather than baking out per-character retargeted sequences [web:8].

### Troubleshooting
- [ ] Hands not following source position after retarget → confirm Retarget IK is enabled on the chain and Blend To Source is not left at 0 [web:7].
- [ ] IK bones not following source at all → verify the IK Goal on the Arm chain is bound to ik_hand_l/ik_hand_r and not an unrelated bone; this is a common cause of retargeted IK simply not moving [web:18].
- [ ] Jello/broken elbow or shoulder poses post-retarget → check joint constraints on the IK Rig's arm chain and confirm the Retarget Pose matches the actual bind pose, not a generic T/A-pose [web:19][web:8].

