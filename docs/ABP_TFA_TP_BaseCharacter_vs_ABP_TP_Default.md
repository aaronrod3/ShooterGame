# ABP_TFA_TP_BaseCharacter vs ABP_TP_Default — AnimGraph/EventGraph Structural Diff

**Reference:** `/Game/Animation/InfimaAnimation/InfimaGames/TacticalFPSAnimations/Common/Core/Characters/ABP_TFA_TP_BaseCharacter.ABP_TFA_TP_BaseCharacter`
**Mine:** `/Game/Character/Animation/ABP_TP_Default.ABP_TP_Default` (current live state, not modified during this investigation)

Read-only investigation via UE5 MCP live editor inspection. No assets modified, no compiles run. Builds on two prior comparisons (character Blueprint diff; FP AnimBP diff). Per those findings: TFA's `Common/Core` reference assets are unconfigured templates with empty `SequencePlayer`/`BlendspacePlayer` pins, and TFA's own FABRIK usage turned out to be animation-authored (zero-offset, animator-posed helper bone) rather than procedural — both patterns repeat here and are noted without being treated as gaps.

---

## 1. Full AnimGraph structure of ABP_TFA_TP_BaseCharacter

~110–120 nodes total (~61 in `:AnimGraph` proper, plus a nested `SM_AimingTransitions` state machine with 3 states/3 transitions).

**Full bottom-to-top dataflow to Output Pose:**
1. Two aim-related `SequencePlayer` pairs (all unassigned) feed `BlendListByBool_0` (`bActiveValue = GetbIsCantedAiming`, 0.17s/0.17s) → **`Slot 'Aiming'`**.
2. A separate `SequencePlayer_0` (unassigned) → **`Slot 'DefaultSlot'`**.
3. `BlendListByBool_4` (`bActiveValue = GetbIsAiming`, 0.2s/0.2s) picks between the two slots' outputs.
4. → `LocalToComponentSpace_0` → `ModifyBone_0` (**recoil**, see below) → `ComponentToLocalSpace_1` → `ApplyAdditive_1` (**breathing/idle additive**, see below).
5. → `ApplyMeshSpaceAdditive_4`: Base = breathing-additive output, Additive = `StateMachine_0` (`SM_AimingTransitions`) output, Alpha = constant 1.0.
6. → `LayeredBoneBlend_4` (**grip overlay**, left-arm-scoped — same mechanism/branch filter as the FP sibling) → **`Slot 'AdditiveSlot'`**.
7. → `LocalToComponentSpace_1` → `Fabrik_2` (left arm) → `Fabrik_0` (right arm) → `ComponentToLocalSpace_0` → **Output Pose**.

Three montage slots total: `Aiming`, `DefaultSlot`, `AdditiveSlot` (the last positioned after the grip overlay, before the IK stage — same pattern as the FP sibling's `AdditiveSlot` placement).

**Recoil application** — `ModifyBone_0` targets **`ik_hand_gun`** (component-space, additive translation+rotation, Alpha=1.0 constant), driven by `RecoilTransform` (a plain variable copied from `CharacterBP.GetCurrentRecoil()` each tick) with **hardcoded per-axis scale multipliers**: translation × `(3.0, 0.5, 4.0)`, rotation = `MakeRotator(Roll×2.0, Pitch×4.0, Yaw×1.7)`. This is genuinely procedurally/externally driven (unlike the FABRIK effectors below) — TFA deliberately exaggerates/reshapes the same `RecoilTransform` differently for third-person than presumably for first-person, via fixed per-axis multipliers on one shared source value.

**Breathing/idle additive** — `ApplyAdditive_1` (local-space, not mesh-space): Base = recoil-modified pose, Additive = `SequencePlayer_4` (unassigned), Alpha = constant 1.0, completely ungated (no bool/curve controls it — it's always fully applied). No `bAnimateCamera`-style variable exists in this TP graph at all (TP doesn't need the FP-specific head-freeze mechanism).

**"SM_AimingTransitions" state machine** — 3 states (`Default` = `LocalRefPose` i.e. additive-zero; `Aim Start` = one unassigned `SequencePlayer`; `Aim End` = one unassigned `SequencePlayer`), layered as a **mesh-space additive** on top of the breathing-additive pose (not blended as a full pose replacement). Transition conditions: `bIsAiming` (raw, 0.05s crossfade) ×2, `NOT bIsAiming` (0.15s crossfade) ×1 — asymmetric fast-in/slow-out timing. Exact from/to state pairing couldn't be confirmed via the reflection API (no `PrevState`/`NextState` accessor exposed on `AnimStateTransitionNode`), but content and timing strongly imply fast entry into Aim Start/Aim End on the aiming-state flipping, slower return to Default.

**Grip overlay** — `LayeredBoneBlend_4`, branch filter `[{clavicle_l, depth 1}, {ik_hand_l, depth 1}]` (left-arm-scoped, identical mechanism/values to the FP sibling), `BlendWeights_0 = GetCurrentGripAlpha`. Feeds from a `CurrentGripPose` cache built the same way as the FP sibling (nested `BlendListByEnum(CurrentGrip)` ×2 for hip-fire/aiming variants, gated by `BlendListByBool(bIsAiming)`, all leaf `SequencePlayer` pins unassigned).

**Hand IK (FABRIK) — same verdict as FP sibling: animation-authored, not procedural.** Both `Fabrik_0` (right: `clavicle_r→hand_r`, effector `ik_hand_r`) and `Fabrik_2` (left: `clavicle_l→hand_l`, effector `ik_hand_l`) use `EffectorTransformSpace=BoneSpace` with **identity/zero-offset** `EffectorTransform` (unconnected) and **`Alpha` hardcoded to constant `1.0`** (unconnected) — `precision=0.01`, `maxIterations=10`, `EffectorRotationSource=CopyFromTarget` on both. No Blueprint variable or external input drives either effector; FABRIK here just snaps the real hand bones onto wherever the animator posed the `ik_hand_l`/`ik_hand_r` helper bones inside whichever clip is currently playing. Left arm resolves before right in the chain (`Fabrik_2` upstream of `Fabrik_0`) — a single sequential pass, both arms always contributing to the final pose.

---

## 2. Full AnimGraph structure of ABP_TP_Default (current state)

~110 nodes total (28 at top `:AnimGraph` level, ~40 each inside two locomotion-related state machines, plus a near-empty EventGraph).

**Two parallel state machines, fused via LayeredBoneBlend:**
- **`SM_TP_Locomotion`** (lower/full-body): 3 states — `Unarmed_IdleWalkRun` (`BlendSpacePlayer 'BS_UnequippedIdleWalkRun'`, X=Direction/Y=Speed), `Armed_Walk` (`BlendSpacePlayer 'BS_EquippedIdleWalkRun'`, same axes), `Sprint` (`SequencePlayer 'Sprint_Forward_Anim'`). Transitions gated on `bWeaponEquipped`/`bIsSprinting` combinations (6 transitions covering the 3×3 pairing, all populated with real assigned assets — unlike TFA's empty template).
- **`SM_TP_UpperBody`**: 3 states — `Relaxed` (`SequencePlayer 'Rifle_Idle_Anim'`), `CombatReady` (`SequencePlayer 'AS_TP_AR_Idle_Pose'`), `ADS` (`SequencePlayer 'AS_TP_AR_Aim_Pose'` feeding the `BasePose` of an **`AimOffsetPlayer 'AO_ADS'`** keyed on `Get AimOffset_Yaw`/`Get AimOffset_Pitch`, Alpha=1.0 constant). Transitions gated on `bHighReady`/`bInCombatState`/`bIsAiming` combinations, all `0.2s` `StandardBlend` crossfades.
- **`LayeredBoneBlend_0`** fuses them: `BasePose = SM_TP_Locomotion` result, `BlendPoses_0 = SM_TP_UpperBody` result, branch filter `[{spine_01, blendDepth: 1}]`, weight constant 1.0 → cached as `'PreSlotPose'`.

**No ADS/Recoil/Crouch additive stack exists at all.** No `ApplyAdditive`, `ApplyMeshSpaceAdditive`, or `ModifyBone` node targeting a procedural offset is present anywhere in the graph. `LayeredBoneBlend_0` (above) is the only per-bone blend node, and its job is fusing the two locomotion/upper-body state machines, not applying recoil/ADS/crouch.

**Montage slots:** `'DefaultSlot'` and `'Aiming'`, both sourced from `UseCachedPose'PreSlotPose'`, feeding `BlendListByBool_0` (gated `bIsAiming`, **0.1s/0.1s** blend) → into the IK stack.

**Hand IK (FABRIK) — configured differently from both TFA and the FP sibling, and currently non-functional on one side:**
- `Fabrik_0` (right arm): `clavicle_r→hand_r`, effector `ik_hand_r`, `BoneSpace`, precision 0.01, `CopyFromTarget` — matches spec. `Alpha=1.0` constant, `EffectorTransform` = hardcoded identity constant (**not** wired to the C++ `RightHandTransform` variable that the FP sibling already uses).
- `Fabrik_1` (left arm): `clavicle_l→hand_l`, but **effector target bone is `hand_r`, not `ik_hand_l`** — confirmed via `ObjectTools.get_properties`. Same `Alpha`/`EffectorTransform` hardcoded-constant pattern as the right arm (**not** wired to `LeftHandTransform`).
- `LODThreshold` is not exposed as a pin on either node (the plan's checklist calls for it to be exposed).
- An orphaned Blueprint variable, `Effector Transform` (Transform, identity default), exists but is referenced by zero Get/Set nodes anywhere.
- **Structural issue found**: the left-arm chain's upstream `LocalToComponentSpace_2` node has a completely disconnected `LocalPose` input (feeds from nothing). Downstream, the two arms' results are meant to be selected by `BlendListByBool_1`, but its `bActiveValue` is a **hardcoded, unconnected `False` constant** — it never toggles at runtime, permanently selecting one fixed branch (`'RightArmPinned'`, a cached round-trip of `Fabrik_0`'s output) over the other. The branch carrying `Fabrik_1`'s (left-arm) result is present in the graph but structurally unreachable in the final output as currently wired.

**EventGraph is functionally dead.** `BlueprintInitializeAnimation` is not implemented at all (`bIsImplemented: false`). `BlueprintUpdateAnimation` is marked implemented but contains only a bare event node and one floating, fully-disconnected `TryGetPawnOwner` call — no exec pins connected, no logic runs. All state-driving variables seen throughout the state machines (`bIsAiming`, `bWeaponEquipped`, `bIsSprinting`, `bHighReady`, `bInCombatState`, `Direction`, `Speed`, `AimOffset_Yaw`/`Pitch`) are read directly from the C++ base class via embedded `Get` nodes throughout `AnimGraph` itself, not staged through a Blueprint-side per-tick copy.

---

## 3. Techniques in ABP_TFA_TP_BaseCharacter worth considering (not gaps)

**(a) Per-axis-scaled recoil reuse for third person.** TFA reuses the exact same `RecoilTransform` source (presumably shared with the FP side) but reshapes it for third-person legibility via fixed per-axis multipliers (translation ×(3.0, 0.5, 4.0), rotation components ×2.0/×4.0/×1.7) rather than authoring a second, independent TP-specific recoil value. Since `ABP_TP_Default` currently has **no recoil/ADS/crouch additive stack at all** (§2), this is directly relevant groundwork for whenever that gets built: reusing the FP-side `RecoilTransform` (and `AimDownSightsTransform`/`CrouchTransform`) with TP-specific axis scaling, rather than computing a second independent set of values, would mirror a proven, low-effort pattern from the reference pack.

**(b) A single, always-on, local-space breathing additive, positioned right after recoil and before the aiming-transition additive.** TFA's structural slot for this (`ApplyAdditive_1`, local-space, Alpha=1.0 constant, ungated) is empty of content in this template, but the *position and wiring pattern* is exactly the still-`[CREATE]` item from the plan. Worth reusing the exact placement (immediately after the recoil `ModifyBone`, immediately before the mesh-space aiming-transition additive) once a breathing/idle sequence asset is authored, rather than re-deriving where in the graph it should sit.

**(c) Mesh-space additive "Aiming Transitions" layered on top of the stance pose, vs. a separate full-pose state machine.** TFA's `SM_AimingTransitions` is layered as an *additive* on top of the already-composited base pose — the base pose keeps playing unmodified, and the state machine only contributes a transient additive delta during the Aim-Start/Aim-End moments, snapping back to a zero/reference-pose `Default` state the rest of the time. `ABP_TP_Default`'s `SM_TP_UpperBody` instead is a **full pose-replacement** state machine (`Relaxed`/`CombatReady`/`ADS`, each a complete `SequencePlayer`/`AimOffsetPlayer` pose, fused into the base via `LayeredBoneBlend_0`'s `spine_01` branch filter) — a different, and already more capable, mechanism (it already carries the aim-offset blend space, which TFA's version doesn't have at all — see §6). This isn't a case where TFA's approach is obviously better; it's a genuinely different design (additive transition-only vs. full replacement states) worth being aware of if `SM_TP_UpperBody`'s current transitions ever feel like they're missing a distinct "start/end" flourish that a thin additive layer could add on top without altering the existing state machine.

**(d) Simple sequential single-pass FABRIK chain vs. a split-then-select structure.** Both TFA AnimBPs (FP and TP) and this project's own `ABP_FP_Default` use one straightforward pattern: left-arm FABRIK feeds directly into right-arm FABRIK in series, both nodes always contributing to the one output pose. `ABP_TP_Default` instead computes the two arms somewhat independently and tries to select between them via `BlendListByBool_1` — and as found in §2, that selector is currently hardcoded to a constant `False` with a disconnected upstream input on the other branch, so the pattern isn't functioning as (presumably) intended right now. The simpler sequential single-chain approach already proven working on the FP side and used throughout the reference pack is worth considering as a reference point for what a working version of this stage should look like structurally.

---

## 4. EventGraph comparison (BlueprintUpdateAnimation / BlueprintInitializeAnimation)

**TFA side:** `BlueprintInitializeAnimation` casts `TryGetPawnOwner` to `BP_TFA_BaseCharacter`, caches as `CharacterBP`. `BlueprintUpdateAnimation` copies exactly four things from `CharacterBP` each tick — `CurrentGrip`, `RecoilTransform` (via `GetCurrentRecoil()`), `bIsAiming`, and a derived `bIsCantedAiming = CharacterBP.bIsCantedAiming AND NOT CharacterBP.bIsBusy` — and calls a custom `InterpolateGripAlpha` event (which FInterpTo's `CurrentGripAlpha` toward 1.0/0.0 based on `bIsLeftHandOnWeapon`, mirroring this project's own `ANS_LeftHandGrip`-driven grip-alpha smoothing). Notably leaner than the FP sibling's EventGraph — no `InputMoveVector`, `Stance`, or crouch/ADS transform copies happen here; `Speed`/`Direction`/`AimOffset_Yaw`/`Pitch` etc. are read directly by `Get` nodes embedded in the state machines instead.

**Native side (checked in source across this session's investigations):** `UShooterAnimInstanceBase::NativeUpdateAnimation()` performs the equivalent per-tick work in C++ (`UpdateMovementData`/`UpdateAimData`/`UpdateCombatData`/`UpdateIKData`), and `UShooterAnimInstanceBase` already owns `CurrentGripAlpha` smoothing via `FInterpTo` inside `UpdateCombatData()`, driven by `ANS_LeftHandGrip`'s notify-pushed overrides — functionally the same role as TFA's `InterpolateGripAlpha` custom event, just implemented in C++ and shared between FP/TP rather than duplicated per-AnimBP.

**`ABP_TP_Default`'s own EventGraph does nothing at runtime** (§2) — `BlueprintInitializeAnimation` isn't implemented at all, and `BlueprintUpdateAnimation` contains only disconnected stub nodes. This is consistent with the project's intended architecture (C++ owns this logic, shared base class with FP), and all the state-machine variables observed (`bIsAiming`, `bWeaponEquipped`, `bIsSprinting`, `bHighReady`, `bInCombatState`, `Direction`, `Speed`, `AimOffset_Yaw/Pitch`) are indeed being read directly from C++ getters inside `AnimGraph`, confirming the native side is supplying this data. Nothing found in TFA's EventGraph appears to be missing from native coverage — it's a "different location for the same responsibility," not a gap.

---

## 5. Notify/notify-state cross-reference against infima_integration_plan.md §3

Identical finding to the FP-side comparison: every `SequencePlayer`/`BlendspacePlayer` pin reachable in `ABP_TFA_TP_BaseCharacter` is unassigned (empty template content, consistent with `Common/Core` being base-class scaffolding only), so no montage/sequence `Notifies` arrays were reachable to inspect. The only concrete AnimNotify class found is, again, **`AN_UnlockActions`** — baked into this AnimBP's own EventGraph as `AnimNotify_AN_UnlockActions` doing `CharacterBP.bIsBusy = false`, identical in name and behavior to the FP sibling's handler. This is already listed in `infima_integration_plan.md` §3 (marked `[CREATE]`). **Nothing new to add.**

---

## 6. Camera-bone / spine-lean / aim-offset technique for third-person aim representation

**TFA has no aim-offset, spine-lean, or counter-rotation mechanism at all.** Its only per-bone techniques in this graph are the recoil `ModifyBone` (targeting `ik_hand_gun`, §1) and the grip-overlay `LayeredBoneBlend` (targeting `clavicle_l`/`ik_hand_l`, §1) — neither is aim-offset-related. No `AimOffsetPlayer`/`AimOffset` blend-space node of any kind exists anywhere in TFA's TP graph, assigned or not, and no spine/chest/head bone is targeted by any skeletal-control node. Third-person aim representation in TFA is handled purely through **pose selection** (the `bIsAiming`-gated slot/state-machine chain), not through procedural bone rotation.

**This is a case where `ABP_TP_Default` already exceeds the reference pack**, not the reverse: it has a real **`AimOffsetPlayer 'AO_ADS'`** node inside `SM_TP_UpperBody`'s `ADS` state, keyed on `Get AimOffset_Yaw`/`Get AimOffset_Pitch` — a genuine 2D aim-offset blend-space mechanism, which is the standard, purpose-built UE technique for exactly this problem (representing aim pitch/yaw visually on a third-person body) and which TFA's reference simply doesn't include anywhere in this asset. (Whether the `AO_ADS` blend-space asset itself is fully populated with clips wasn't explicitly re-verified in this pass — worth a direct check if not already known.)

The one loosely-related structural note: `LayeredBoneBlend_0` (§2), which fuses lower-body locomotion with the upper-body/aim pose, uses a `spine_01` branch filter with `blendDepth: 1` — a shallow depth that may not extend far enough up the spine/arm chain to guarantee the full upper body (including hands) is sourced from the aim-driven pose rather than the locomotion pose, depending on the skeleton's bone depth under `spine_01`. This is adjacent to (not itself) an aim-offset/lean mechanism, but worth being aware of since it's the seam between "body driven by movement" and "body driven by aim" in the current graph.

**Conclusion:** for this specific concern (third-person aim visual correctness), the reference pack offers no technique to borrow — `ABP_TP_Default`'s existing `AimOffsetPlayer` setup is already the more sophisticated of the two.
