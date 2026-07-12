# Step 7 — Third-Person AnimBP (`ABP_TFA_TP_BaseCharacter`)

**Asset path:** `/Game/InfimaGames/TacticalFPSAnimations/Common/Core/Characters/ABP_TFA_TP_BaseCharacter`

The third-person runtime bridge between `BP_TFA_BaseCharacter` and the pack's third-person pose stack. Pulls stance/weapon state from the owning character, blends hip-fire vs. aiming stances, routes montages through stance-aware slots, pins both hands to the weapon with FABRIK, and handles left-hand grip blending — same overall responsibilities as the FP AnimBP, adapted for a full-body third-person view.

> **Important scope note, from the pack's own documentation:** this AnimBP (and the pack generally) does **not** include third-person locomotion/movement animation content. Only upper-body/aiming behavior is covered here. See [13_FAQ_Scope_And_Constraints.md](13_FAQ_Scope_And_Constraints.md) for the official reasoning and the recommended workaround (pair this pack's upper-body content with an external full-body locomotion/mocap pack for the lower body).

## Dependencies (same three as the FP AnimBP)

1. Owning pawn is `BP_TFA_BaseCharacter` or a child class.
2. AnimBP implements `BPI_TFA_AnimationState`.
3. Skeleton includes the FABRIK IK chain bones (`ik_hand_r`/`hand_r`/`clavicle_r`, `ik_hand_l`/`hand_l`/`clavicle_l`).

Missing any of these degrades aim-state updates and hand placement stability, but doesn't hard-fail evaluation.

## Variables

- **Reference:** `CharacterBP` (`BP_TFA_BaseCharacter`), cached on init.
- **State pulled from CharacterBP:** `bIsAiming`, `CurrentGrip` (`E_TFA_GripAttachment`).
- **Procedural data pulled from CharacterBP:** `RecoilTransform`.
- **Grip state driven by notify states:** `bIsLeftHandOnWeapon`, `CurrentGripAlpha`, `GripBlendSpeed`, `GripPoseBlendSpeed`.

## Initialization and per-frame update

- **`BlueprintInitializeAnimation`**: `TryGetPawnOwner` → cast to `BP_TFA_BaseCharacter` → store in `CharacterBP`. Cast failure means `bIsAiming` never changes, the aim pose never blends, grip alpha never updates.
- **`BlueprintUpdateAnimation`**: copies `CharacterBP.bIsAiming → bIsAiming`, `CharacterBP.CurrentGrip → CurrentGrip`, `CharacterBP.CurrentRecoil → RecoilTransform`. Also interpolates `CurrentGripAlpha` every frame toward `1.0`/`0.0` based on `bIsLeftHandOnWeapon`, using `GripBlendSpeed` with `FInterpTo`.

## Unlock Actions notify hook

Implements `AnimNotify_AN_UnlockActions`; on fire, `CharacterBP.bIsBusy = false`.

## Left Hand Grip flow

Identical mechanism to the FP AnimBP: `ANS_TFA_LeftHandGrip` calls `BPI_TFA_AnimationState.UpdateLeftHandGrip(bool, float)`, this AnimBP stores the values, then smooths via `CurrentGripAlpha`.

## AnimGraph, build in this order

### 1. Stance blend
Blends aimed vs. hip-fire stances using `bIsAiming`, via a `BlendListByBool` with **aim-in blend time `0.2`** and **aim-out blend time `0.2`**.

### 2. Aiming transitions (layered on top, doesn't replace the stance pose)
A mesh-space additive state machine, **`SM_AimingTransitions`**, adds smoother raise/lower motion:

| State | Content |
|---|---|
| `Default` | Effectively the reference pose |
| `Aim Start` | Bound to `CharacterBP.WeaponConfig.TP_Transition_AimStart` |
| `Aim End` | Bound to `CharacterBP.WeaponConfig.TP_Transition_AimEnd` |

Transitions:
| From → To | Condition | Crossfade | Blend mode |
|---|---|---|---|
| `Default → Aim Start` | `bIsAiming == true` | `0.05` | Sinusoidal |
| `Aim Start → Aim End` | `bIsAiming == false` | `0.15` | QuadraticInOut |
| `Aim End → Aim Start` | `bIsAiming == true` | `0.05` | Sinusoidal |

If `WeaponConfig` doesn't provide valid TP aim-transition assets, this state machine has nothing meaningful to play.

### 3. Breathing/idle additive
A looping idle sequence bound to `CharacterBP.WeaponConfig.TP_IdleLoop`, applied additively for subtle motion even when no montage is playing.

### 4. Montage slots
Two slot nodes — one for montages while aiming, one while hip-firing — so an aiming upper-body action doesn't blend against the wrong base stance. The aiming-stance slot is named **`Aiming`**.

### 5. Hand IK with FABRIK
At the end of evaluation: convert to component space, run FABRIK for both arms — right arm `clavicle_r → hand_r`, effector `ik_hand_r`; left arm `clavicle_l → hand_l`, effector `ik_hand_l`. Both FABRIK nodes use: Effector Transform Space = Bone Space; Precision = `0.01`; Effector Rotation Source = CopyFromTarget; `LODThreshold` exposed on both.

## Integration checklist

- [ ] Mesh using this AnimBP belongs to `BP_TFA_BaseCharacter`.
- [ ] Owning pawn is `BP_TFA_BaseCharacter` or a child.
- [ ] AnimBP implements `BPI_TFA_AnimationState`.
- [ ] Montages include `ANS_TFA_LeftHandGrip` windows where expected.
- [ ] `WeaponConfig` provides: `TP_IdlePose`, `TP_IdleLoop`, `TP_AimPose`, `TP_Transition_AimStart`, `TP_Transition_AimEnd`.

## Troubleshooting

| Symptom | Check |
|---|---|
| `CharacterBP` is None | Mesh belongs to `BP_TFA_BaseCharacter`; mesh uses this AnimBP as its Anim Class |
| Grip blending does nothing | AnimBP implements `BPI_TFA_AnimationState`; montages include `ANS_TFA_LeftHandGrip`; `GripBlendSpeed` isn't `0` |
| Hands look floaty or detached | Skeleton has `ik_hand_r`/`ik_hand_l`; weapon IK targets are authored consistently against those bones |
