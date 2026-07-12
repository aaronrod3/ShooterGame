# Step 6 — First-Person AnimBP (`ABP_TFA_FP_BaseCharacter`)

**Asset path:** `/Game/InfimaGames/TacticalFPSAnimations/Common/Core/Characters/ABP_TFA_FP_BaseCharacter`

The main first-person animation hub. Reads gameplay state from `BP_TFA_BaseCharacter`, applies ADS/recoil/crouch offsets, drives grip and hand-IK behavior used by the notify-state system.

## Dependencies (all three required for full functionality)

1. Owning pawn must be `BP_TFA_BaseCharacter` or a child class.
2. The AnimBP must implement **`BPI_TFA_AnimationState`** (see [08_Animation_Notifies_And_States.md](08_Animation_Notifies_And_States.md)).
3. The skeleton must include the FABRIK IK chain bones: `ik_hand_r`/`hand_r`/`clavicle_r` and `ik_hand_l`/`hand_l`/`clavicle_l`.

If any of these is missing, the AnimBP still evaluates, but with degraded features — the clearest symptoms are floaty hands and grip blending that appears to do nothing.

## Variables this AnimBP exposes/consumes

- **Cached reference:** `CharacterBP` (`BP_TFA_BaseCharacter`), set during initialization.
- **State pulled from CharacterBP:** `bIsRunning`, `bIsWalking`, `bIsSprinting`, `bIsAiming`, `bAnimateCamera`, `Stance` (`E_TFA_Stance`), `CurrentGrip` (`E_TFA_GripAttachment`).
- **Animation data pulled from CharacterBP:** `AimDownSightsTransform`, `RecoilTransform`, `CrouchTransform`.
- **Grip state driven by notify states:** `bIsLeftHandOnWeapon`, `CurrentGripAlpha`, `GripBlendSpeed`, `GripPoseBlendSpeed`.

## Initialization and per-frame update

- **`BlueprintInitializeAnimation`**: calls `TryGetPawnOwner`, casts to `BP_TFA_BaseCharacter`, stores in `CharacterBP`. If the cast fails, all pull-from-character logic stays idle.
- **`BlueprintUpdateAnimation`**: validates `CharacterBP`, builds `InputMoveVector` from `CharacterBP.SimulatedVelocity.XY` (exists because the demo simulates movement rather than physically moving the capsule), copies state booleans and procedural transforms, copies `CurrentGrip`, calls **Interpolate Grip Alpha**.

## Unlock Actions notify hook

Implements `AnimNotify_AN_UnlockActions`. When it fires: `CharacterBP.bIsBusy = false`. Emitted by **Unlock Actions** (`AN_TFA_UnlockActions`).

## Left Hand Grip flow

**Left Hand Grip** (`ANS_TFA_LeftHandGrip`) calls `BPI_TFA_AnimationState.UpdateLeftHandGrip(bool IsLeftHandOnWeapon, float BlendSpeed)`. This AnimBP stores the incoming values in `bIsLeftHandOnWeapon`/`GripBlendSpeed`. `GripPoseBlendSpeed` is separate tuning for pose swaps when `CurrentGrip` changes — unused if your graph never reads it.

**Interpolate Grip Alpha:** every frame, interpolates `CurrentGripAlpha` toward `1.0` or `0.0` based on `bIsLeftHandOnWeapon`, using `GripBlendSpeed` with `FInterpTo` and `GetWorldDeltaSeconds`. The notify state sets the *target and speed*; the AnimBP performs the *smoothing*.

## AnimGraph, build in this order

### 1. Locomotion base pose
Driven by `InputMoveVector`, `bIsWalking`/`bIsRunning`/`bIsSprinting`, and `Stance` — lets the pack preview movement without moving the capsule. Treat zero input as idle; keep X/Y as forward/strafe. **If you switch to real movement, use actual velocity instead of the simulated value, but keep it a 2D vector** so blend spaces keep working correctly.

### 2. Mesh-space additive stack
Layered on top of locomotion: recoil, ADS offsets, subtle sway, stance overlays, grip overlays tied to `CurrentGrip`. **These must be Mesh Space additives, not Local Space** — get this wrong and you get double transforms, arm twisting during recoil, or ADS offsets sliding the wrong direction.

**Mesh-space additive authoring checklist** for any new clip: Additive Type = Mesh Space; Base Pose = Reference Pose (or whatever base pose the rest of the pack uses); Root Motion = disabled.

### 3. Hand IK with FABRIK
Convert to component space, run FABRIK on both arms to pin hands to weapon IK targets — this is what keeps hands stable once additive layers stack. Both FABRIK nodes expose `LODThreshold` (disable IK at distant LODs to save cost).

### 4. Camera/head animation toggle
A `LayeredBoneBlend` on the head branch, `bAnimateCamera` controlling blend weight. The blend pose here is the reference pose, so this toggle overrides head motion with a stable reference pose: weight `0` keeps head motion from the base pose; weight `1` forces the head branch to reference pose. Simple way to remove camera/head motion without rewriting the base pose.

### 5. Procedural offsets
Consumes character-driven transforms — `AimDownSightsTransform` (ADS), `RecoilTransform` (recoil), `CrouchTransform` (crouch). The AnimBP does **not** generate these values, only applies them as part of the additive stack, before IK.

### 6. Grip overlays
Two parts: `CurrentGrip` selects which grip pose to use; `CurrentGripAlpha` controls how much of the overlay is visible. `ANS_TFA_LeftHandGrip` drives the alpha window; `CurrentGrip` usually comes from weapon/config state on the character. Keeps the correct grip pose selected while smoothly fading the left-hand overlay in/out during reloads or interactions.

### Typical full evaluation order
1. Build the locomotion base pose.
2. Apply mesh-space additive layers (ADS, recoil, crouch).
3. Run hand IK to pin hands to the weapon.
4. Optionally blend camera/head motion.

This order keeps hands stable during ADS and recoil. **Cache the base pose** (`UseCachedPose`) and reuse it downstream for both additive layers and IK, to avoid duplicate evaluation.

## Integration checklist

- [ ] Mesh using this AnimBP belongs to `BP_TFA_BaseCharacter` (or a child).
- [ ] AnimBP implements `BPI_TFA_AnimationState`.
- [ ] Montages include `ANS_TFA_LeftHandGrip` windows where needed.
- [ ] `BP_TFA_BaseCharacter` is actually updating `SimulatedVelocity`, ADS/recoil/crouch transforms, and `CurrentGrip`.

## Troubleshooting

| Symptom | Check |
|---|---|
| `CharacterBP` is None | Mesh belongs to `BP_TFA_BaseCharacter`; mesh actually uses this AnimBP as its Anim Class |
| Grip blending does nothing | AnimBP implements `BPI_TFA_AnimationState`; montages include `ANS_TFA_LeftHandGrip`; `GripBlendSpeed` isn't `0` |
| IK looks broken | Skeleton has the required IK bones; weapon sets up IK targets on the intended bones/sockets |
