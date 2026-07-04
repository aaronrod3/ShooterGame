# ABP_TFA_FP_BaseCharacter vs ABP_FP_Default — AnimGraph/EventGraph Structural Diff

**Reference:** `/Game/Animation/InfimaAnimation/InfimaGames/TacticalFPSAnimations/Common/Core/Characters/ABP_TFA_FP_BaseCharacter.ABP_TFA_FP_BaseCharacter`
**Mine:** `/Game/Character/Animation/ABP_FP_Default.ABP_FP_Default` (state captured post today's edits: grip-alpha `LayeredBoneBlend` restructure + camera blend gate wiring, both already applied and verified compiling)

Read-only investigation via UE5 MCP live editor inspection. No assets modified, no compiles run beyond the earlier same-day work already confirmed. Builds on the prior character-Blueprint diff (`BP_TFA_BaseCharacter` vs `BP_PlayerCharacter`) — the note that this project has no perspective-switching/CameraManager system yet is treated as expected, not a gap.

---

## 1. Full AnimGraph structure of ABP_TFA_FP_BaseCharacter

~110–120 nodes total (79 at the top `:AnimGraph` level, plus six state-machine sub-graphs).

**Locomotion — three parallel state machines, not one unified machine:**
- `SM_LocomotionStanding` (Stand/Running/Sprinting) → cached as `CachedLocomotionStanding`
- `SM_LocomotionAimed` (Walking only) → cached as `CachedLocomotionAimed`
- `SM_LocomotionCrouched` (Crouch only) → cached as `CachedLocomotionCrouched`

Selection: `BlendListByEnum(E_TFA_Stance)` picks Standing-cache vs Crouched-cache → feeds `Slot'DefaultSlot'`. The Aimed-cache feeds `Slot'Aiming'` directly (no stance branch). A `BlendListByBool` on `bIsAiming` then picks between the two slots' outputs → cached as `CurrentStancePose`.

**Additive layer A — mesh-space transition polish, stacked via `ApplyMeshSpaceAdditive` ×3:**
`CurrentStancePose` is wrapped three times, each adding a separate transition state machine (`SM_CrouchingTransitions`, `SM_RunningTransitions`, `SM_WalkingTransitions` — each with Default/Start/End states) as a mesh-space additive layer → cached as `AdditivePose`. This is a distinct technique from cross-fade blend times: transition "settle" poses play as their own additive layer on top of the base loop.

**Additive layer B — single-bone chained `Transform(Modify)Bone` ×3, all targeting `ik_hand_gun`:**
`AdditivePose` → `LocalToComponentSpace` → `ModifyBone(Recoil, Additive/Additive/Ignore, ComponentSpace)` → `ModifyBone(AimDownSights, Additive/Additive/Additive, **translation in WorldSpace** — the other two are ComponentSpace, a minor inconsistency in the reference asset)` → `ModifyBone(Crouch, Additive/Additive/Ignore, ComponentSpace)` → `ComponentToLocalSpace`. All three chain onto one dedicated weapon-attach bone (`ik_hand_gun`), not the skeleton root.

**Grip pose blend:**
`LayeredBoneBlend_4` — `BasePose` = the ik_hand_gun-modified chain above, `BlendPoses_0` = `UseCachedPose 'CurrentGripPose'`, `BlendWeights_0 = Get CurrentGripAlpha`. **Branch filter confirmed readable via `ObjectTools.get_properties`: `[{boneName:"clavicle_l", blendDepth:1}, {boneName:"ik_hand_l", blendDepth:1}]`** — i.e. this blend is scoped to the **left arm only**, leaving the right (trigger) arm and weapon untouched by grip-alpha blending. → feeds `Slot'AdditiveSlot'`.

**Grip selection mechanism (confirmed pose-swap, not IK):**
Two parallel, structurally identical `BlendListByEnum(E_TFA_GripAttachment)` nodes (one per aim state), each with 4 branches: `None→LocalRefPose`, `Vertical/Angled/4th→SequencePlayer`. `ActiveEnumValue = Get CurrentGrip`, blend time from `Get GripPoseSwitchSpeed`. A `BlendListByBool` on `bIsAiming` then picks between the two enum-blend outputs → cached as `CurrentGripPose`. **Caveat: all 6 SequencePlayer nodes feeding these branches have an empty/unassigned `Sequence` pin** — the mechanism is fully wired but carries no actual animation content in this template asset.

**FABRIK — exists, but is animation-authored, not procedural (corrects the prior assumption of "no IK at all"):**
Two chained `FABRIK` nodes do exist: `Fabrik_1` (left: `clavicle_l→hand_l`, effector `ik_hand_l`) then `Fabrik_0` (right: `clavicle_r→hand_r`, effector `ik_hand_r`), both `EffectorTransformSpace=BoneSpace` with a **zero-offset** `EffectorTransform` and constant `Alpha=1`. Since the offset is zero and bone-space, FABRIK simply snaps the real hand bones onto wherever the animator posed the helper `ik_hand_l`/`ik_hand_r` bones *inside the clip itself* — there is no Blueprint-variable or C++ input driving these effectors at all. This is "IK as a rigging convenience for animators," fundamentally different from a runtime procedural system.

**Full chain to Output Pose:** `AdditiveSlot` → `LocalToComponentSpace` → `Fabrik_1` → `Fabrik_0` → `ComponentToLocalSpace` → `LayeredBoneBlend_1` (head-freeze, see §6) → `Output Pose`.

**Montage slots:** three — `AdditiveSlot`, `Aiming`, `DefaultSlot`.

**Dead node found:** `LayeredBoneBlend_2` is orphaned (both `BasePose` input and `Pose` output unconnected), carrying the same `head`/blendDepth-0 branch filter and a static `BlendWeights_0=0.5` as the live `LayeredBoneBlend_1` — an unwired leftover duplicate, not part of the active graph.

---

## 2. Full AnimGraph structure of ABP_FP_Default (current, post-edit)

~59 top-level nodes — a flatter, simpler graph than the reference.

**Locomotion — one unified state machine:** `SM_FP_Locomotion` (states: Standing, Crouching, Aiming, with transitions between all three) → cached as the base pose feeding everything downstream. No separate parallel machines, no additive transition-polish layer.

**Procedural offsets — single-bone chained `Transform(Modify)Bone` ×3, all targeting `root`:**
`UseCachedPose` (locomotion) → `LocalToComponentSpace_0` → `ModifyBone_2` (**CrouchTransform**, translation-only additive, Alpha = `ToFloat(EnumEquality_0)` — a boolean-gated alpha, not constant) → `ModifyBone_1` (**AimDownSightsTransform**, translation+rotation additive, Alpha = a **product of two float sources** (`VariableGet_11 × CallFunction_8`) — dynamically modulated, not constant) → `ModifyBone_0` (**RecoilTransform**, translation-only additive, Alpha = 1.0 constant). All three target the **`root`** bone in component space, not an isolated weapon-attach bone. See §3(a) for why this is likely fine on this rig specifically.

**Hand IK — real FABRIK driven by C++-computed effectors, not animation-authored:**
→ `Fabrik_1` (left arm, `EffectorTransform = Get LeftHandTransform`, Alpha = 1.0 constant) → `Fabrik_0` (right arm, `EffectorTransform = Get RightHandTransform`, Alpha = `Get bWeaponEquipped` as float) → `ComponentToLocalSpace_0`. `LeftHandTransform`/`RightHandTransform` are computed every tick in C++ (`UShooterAnimInstanceBase::UpdateIKData()`) by reading `SOCKET_Grip`/`SOCKET_Grip_R` off the equipped weapon mesh — this is genuine runtime procedural IK, not a pass-through.

**Grip selection cascade** (built pre-crash, confirmed untouched by today's edits): nested `BlendListByBool` ×4 —
- `_4`: `CurrentGrip==GripAngled` → true: `_0`'s result / false: `_3`'s result
- `_3`: `CurrentGrip==GripVertical` → true: `_1`'s result / false: `ComponentToLocalSpace_0` (the raw FABRIK pose — the implicit "no grip overlay" fallback; no explicit `GripNone` branch exists)
- `_0`/`_1`: each `CurrentStance==Aiming` → Aim-pose vs Idle-pose SequencePlayer, for Angled/Vertical respectively (`AS_FP_AR_Aim/Idle_Pose_Grip_Angled/Vertical` — these clips **are** assigned, unlike TFA's empty template)

**Grip-alpha blend (added today):** `LayeredBoneBlend_2` — `BasePose = ComponentToLocalSpace_0` (raw IK fallback), `BlendPoses_0 = BlendListByBool_4`'s output (the Angled/Vertical cascade result), `BlendWeights_0 = Get CurrentGripAlpha`. **Branch filter confirmed via `ObjectTools.get_properties`: `[{boneName:"root", blendDepth:-1}]`** (full-body, set manually in-editor today) — see §3(a) for the direct comparison against TFA's left-arm-scoped equivalent.

**Camera/head blend (wired today):** `LayeredBoneBlend_0` — `BasePose = LayeredBoneBlend_2`'s Pose output, `BlendPoses_0 = ComponentToLocalSpace_1 → MeshRefPose_0` (mesh-space reference pose), `BlendWeights_0 = ToFloat(Get bAnimateCamera)`. **Branch filter: `[{boneName:"head", blendDepth:1}]`** — see §3(d) and §6 for the depth-1-vs-depth-0 comparison against TFA.

**Montage slot selection:** `BlendListByBool_2` picks between `Slot'Aiming'` and `Slot'DefaultSlot'` (both reading from a `'MainPose'` cached pose fed by `LayeredBoneBlend_0`'s output) based on `bIsAiming AND (Firing OR Reloading OR MagCheck)` → `Output Pose`. Only two slots total (`DefaultSlot`, `Aiming`) vs TFA's three.

---

## 3. Techniques in ABP_TFA_FP_BaseCharacter worth considering (not gaps)

**(a) Grip-alpha branch-filter scope: left-arm-only (TFA) vs whole-body (ours).** TFA's grip `LayeredBoneBlend_4` is scoped to just `clavicle_l`+`ik_hand_l` (depth 1) — its grip-alpha blend only ever touches the support-hand arm chain, guaranteeing the right/trigger hand and weapon are never affected by grip-pose blending. Today's new `LayeredBoneBlend_2` is scoped to `root`, full depth — a whole-body blend. This is worth a direct look: if the `AS_FP_AR_Aim/Idle_Pose_Grip_Angled/Vertical` clips were authored as left-arm-only poses (just repositioning the support hand on the foregrip), a whole-body branch filter is functionally harmless but wasteful; if they were authored as full-body poses with any incidental right-arm/weapon/body variation baked in, a whole-body filter will blend that variation in too as `CurrentGripAlpha` changes, which could cause subtle right-hand/weapon drift during the grip-swap blend. Narrowing the new node's branch filter to the left-arm bones (matching TFA's approach) would eliminate that risk entirely if it turns out to matter.

**(b) Separate additive "transition polish" layer for locomotion.** TFA runs three parallel state machines purely for crouch/run/walk start-stop transitions and layers them on as a genuinely separate mesh-space additive (`ApplyMeshSpaceAdditive` ×3) on top of the base locomotion loop, rather than relying only on cross-fade blend times inside one state machine (which is what `SM_FP_Locomotion` does today). If stance/speed transitions ever read as too abrupt or "sliding," this is a proven, isolated pattern to borrow — it doesn't require restructuring the existing single state machine, just adding an additive transition-pose layer downstream of it.

**(c) A montage slot positioned *before* FABRIK.** TFA's `AdditiveSlot` sits structurally between the grip-pose blend and the FABRIK arm-IK step, meaning any montage played through it (upper-body actions like reload/inspect) gets its hand placement IK-corrected onto the current grip target regardless of how the montage itself was animated. Both of `ABP_FP_Default`'s slots (`DefaultSlot`, `Aiming`) sit downstream of FABRIK, near Output Pose — montages played through them do **not** get IK-corrected hand placement. Worth considering whether reload/inspect/interaction montages should route through a slot inserted pre-FABRIK (mirroring TFA's `AdditiveSlot` placement) so their hands stay glued to the weapon grip sockets regardless of clip authoring accuracy.

**(d) Head-freeze branch-filter depth.** Covered in detail in §6 — TFA uses `blendDepth 0` (unlimited/whole branch under `head`) where `ABP_FP_Default` uses `blendDepth 1` (shallow). Worth verifying against the actual skeleton hierarchy in case anything relevant sits more than one level below `head`.

---

## 4. EventGraph comparison (BlueprintInitializeAnimation / BlueprintUpdateAnimation)

**TFA side:** `BlueprintInitializeAnimation` does `TryGetPawnOwner → Cast to BP_TFA_BaseCharacter → store CharacterBP`. `BlueprintUpdateAnimation` copies AnimBP-relevant state from `CharacterBP` each tick — confirmed directly for at least `bAnimateCamera` (`SetbAnimateCamera(GetbAnimateCamera(GetCharacterBP))`, a pure passthrough), and by pattern (per the sibling character-Blueprint's variable list) almost certainly mirrors `bIsAiming`, `CurrentGrip`, `CurrentStance`, and the procedural transforms the same way, since TFA keeps essentially all animation-relevant state on the character Blueprint and just copies it into the AnimInstance every frame.

**Native side (checked directly in source):** `UShooterAnimInstanceBase::NativeInitializeAnimation()` performs the exact same `TryGetPawnOwner → Cast → store` pattern in C++ (storing both the character and its `CombatComponent`). `NativeUpdateAnimation()` calls private `UpdateMovementData()` / `UpdateAimData()` / `UpdateCombatData()` / `UpdateIKData()` / `UpdateWeaponConfigData()`, which collectively perform the same per-tick copy of `bIsAiming`, `CurrentGrip`, `CrouchTransform`/`AimDownSightsTransform`/`RecoilTransform`, and compute the IK effector transforms — all in C++, none of it in `ABP_FP_Default`'s own EventGraph.

**Conclusion:** nothing found in TFA's EventGraph is missing on the native side — it's implemented in C++ rather than Blueprint, which is the project's stated, intentional architecture (per CLAUDE.md's native-reimplementation pillar), not a gap. `ABP_FP_Default`'s own `BlueprintInitializeAnimation`/`BlueprintUpdateAnimation` graphs are expected to be near-empty by design.

One open question surfaced by this comparison: TFA's `bAnimateCamera` is driven by copying a character-side value every tick (implying something *does* set it dynamically somewhere in TFA's ecosystem, likely tied to `CurrentCameraPerspective`). This project's `UShooterFPAnimInstance::bAnimateCamera` is a plain bool with no character-side counterpart found in the prior character diff, and no perspective system exists yet to toggle it — worth keeping in mind that when a CameraManager is eventually built, it will need its own mechanism to flip this flag (there's no existing "driver" to repurpose from either side).

---

## 5. Notify/notify-state cross-reference against infima_integration_plan.md §3

The investigation found that `ABP_TFA_FP_BaseCharacter` and its entire declared config chain (`BP_TFA_BaseCharacter.WeaponConfig` → `BP_TFA_BaseConfig`) are **unconfigured templates**: every `SequencePlayer`/`BlendspacePlayer` pin in the AnimGraph is empty, every field on `BP_TFA_BaseConfig`'s CDO (100+ montage/pose reference fields) reads `None`, and no concrete subclass/instance of `BP_TFA_BaseConfig` exists anywhere under `/Game/Animation/InfimaAnimation`. This is a genuine content gap in the pack's `Common/Core` reference folder (base classes only) — not a limitation of the investigation.

**Result:** no Fire/Reload/magazine montages were reachable from this AnimBP to inspect for notify tracks. The only concrete AnimNotify class found anywhere in this pass was **`AN_UnlockActions`**, discovered baked directly into the AnimBP's own EventGraph as an implemented custom event (`CharacterBP.bIsBusy = false`) — this is already listed in `infima_integration_plan.md` §3 (marked `[CREATE]`). **Nothing new to add to that list.** A fuller notify audit (to find `AN_DropMagazine`, `AN_EjectCasing`, etc. in actual use) would need to target wherever the pack's *configured* weapon content lives, outside `Common/Core` — not reachable from this AnimBP.

---

## 6. Camera-bone / head-tracking groundwork

TFA's entire "camera follows head bone" mechanism is one `LayeredBoneBlend_1` node, positioned as the **last node before Output Pose**, isolating the `head`-rooted branch (`blendDepth 0` = the whole branch, unlimited) and blending it to a static reference pose (`LocalRefPose`) whenever `bAnimateCamera` is true (via `ToFloat(Get bAnimateCamera)` as the blend weight). When true, animation stops touching everything under `head` — presumably so a camera/control-rig system elsewhere (not present in this AnimBP or its EventGraph) can drive that branch directly. **No head-tracking math — no control-rotation read, no aim-offset, no look-at node — exists anywhere inside this AnimBP.** `bAnimateCamera` is a pure passthrough boolean from the character Blueprint; whatever actually *animates* the head bone when the flag is true must live outside this graph entirely (character BP tick logic, or native code, neither of which was reachable/relevant to inspect here).

This maps almost exactly onto what's already built in `ABP_FP_Default`: `LayeredBoneBlend_0` does the identical job (freeze the `head` branch to reference pose, gated by `ToFloat(Get bAnimateCamera)`), wired today. The only structural difference found is branch-filter depth: TFA uses `head`/blendDepth **0** (whole branch, unlimited), `ABP_FP_Default` uses `head`/blendDepth **1** (shallow — only `head` plus its immediate children). If anything relevant to camera-driving sits more than one bone below `head` in this project's skeleton, depth 1 won't reach it — worth a direct check against the skeleton hierarchy before relying on it.

**Groundwork takeaway for a future CameraManager:** neither AnimBP contains a reusable look-at/head-tracking node to build on — both only provide the "get animation out of the way" half of the mechanism (the `LayeredBoneBlend` freeze-to-refpose gate). The actual bone-driving logic for a real first-person camera system will need to be built from scratch (e.g. a Control Rig graph, or a C++-side `SetBoneTransformByName` call driven by camera rotation each tick) — this is a feature to design, not a piece to port from the reference pack. No dedicated camera/eye bone reference was found in either graph; both operate purely at the `head` bone/branch level.
