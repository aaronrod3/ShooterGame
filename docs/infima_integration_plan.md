# ShooterGame — Infima Pack Integration Checklist (Project-Mapped)

> This checklist has been adapted from the original Infima Games Tactical FPS Animation Pack documentation to match ShooterGame's actual asset names, folder structure, and skeleton. Items marked **[TRANSFER]** need to be moved/duplicated from the Infima Pack into your own project folders. Items marked **[CREATE]** don't exist yet in your project and need to be built. Items marked **[VERIFY]** exist in some form and need a live in-editor confirmation pass.

## 1. Skeleton and Character Retarget

- [x] Your character uses **SK_Mannequin** — the standard UE5 skeleton. This is the same skeleton/bone hierarchy the Infima Pack targets, so no skeleton reassignment or bone-matching pass is required. **[VERIFIED BY USER]**
- [x] Confirm your player character mesh is correctly assigned to `SK_Mannequin` in the Skeletal Mesh asset (Skeleton field), and that `BP_PlayerCharacter` references that mesh. **[VERIFY]**
- [x] Required FABRIK IK bone chains (standard on SK_Mannequin, should already be present):
  - Right arm: `clavicle_r` → `hand_r` → `ik_hand_r` (effector)
  - Left arm: `clavicle_l` → `hand_l` → `ik_hand_l` (effector) **[VERIFY]**
- [x] Open your weapon config data asset: `DA_RifleBaseConfig` (your project's equivalent of the Infima weapon Data Asset). **[VERIFY LOCATION]**
- [x] Expand the Character Mesh fields on `DA_RifleBaseConfig` and confirm FP and TP mesh references point to your actual character mesh(es), not Infima demo meshes. **[VERIFY]**
- [x] Test in a PIE session using `BP_PlayerCharacter` + `BP_Rifle` together, since there is no Infima demo map to rely on anymore. **[VERIFY]**
- [x] For custom weapon meshes on `BP_Rifle`: confirm the receiver skeletal mesh was imported with the correct weapon skeleton selected on import (per your rifle's specific rig), so animations play without needing retarget. **[VERIFY]**

## 2. Animation Blueprint Setup

### Shared AnimBP Dependencies (FP and TP)
- [x] Owning pawn must be `BP_PlayerCharacter` (your equivalent of `BP_TFA_BaseCharacter`). **[VERIFY]**
- [x] AnimBP Animation State interface exists as `IShooterAnimStateInterface` (`Source/ShooterGame/Player/Animation/ShooterAnimStateInterface.h`), implemented by `UShooterAnimInstanceBase`. Exposes `UpdateLeftHandGrip(bool, float)` plus two extra hooks (`SetAimingBlocked`, `OnAnimStateMessage`) not originally in this checklist. **[DONE]**
- [x] Skeleton must include FABRIK chain bones: `ik_hand_r`, `hand_r`, `clavicle_r`, `ik_hand_l`, `hand_l`, `clavicle_l`. Since you're on standard `SK_Mannequin`, these should already exist. **[VERIFY]**
- [x] Right-hand IK independence bug fixed: `UpdateIKData()` previously returned early when `SOCKET_Grip` (left) was missing, which also blocked the `SOCKET_Grip_R` (right) block from ever running. Left and right hand socket checks are now fully decoupled — confirmed via Rider audit and diff review 2026-07-04. **[DONE]**
- [x] Grip overlay structure clarified 2026-07-04: CurrentGripAlpha only models the None ↔ any-grip-attached transition (`GripTarget = CurrentGrip != None ? 1 : 0`, smoothed via FInterpTo) — it cannot distinguish Vertical from Angled grip on its own. Correct structure: inner `BlendListByBool` chooses Vertical vs Angled (discrete, no alpha involved), outer `LayeredBoneBlend` blends None/main pose against that result using `CurrentGripAlpha` as `BlendWeights_0`. Branch filter on the new LayeredBoneBlend requires a manual in-editor bone entry (root, full depth) — not settable via MCP. **[DONE — see MCP session 2026-07-04]**

### ABP_FP_Default (First-Person — your equivalent of ABP_TFA_FP_BaseCharacter)
- [x] On BlueprintInitializeAnimation / BlueprintUpdateAnimation: confirmed handled natively in C++ (`UShooterAnimInstanceBase::NativeInitializeAnimation`/`NativeUpdateAnimation`) — no Blueprint EventGraph copy logic needed. Blueprint EventGraph is intentionally near-empty by design. **[VERIFIED — handled natively]**
- [x] AnimGraph evaluation order — confirmed present and matches required structure: locomotion base pose (`SM_FP_Locomotion`, cached), mesh-space additive stack for Crouch/ADS/Recoil (three chained `Transform(Modify)Bone` on `root`), FABRIK hand IK (both arms, wired to `LeftHandTransform`/`RightHandTransform`), LayeredBoneBlend for camera/head gated by `bAnimateCamera`. **[DONE — completed 2026-07-04]**
- [x] Grip overlay nodes: nested `BlendListByBool` cascade (Angled/Vertical/None) feeding a `LayeredBoneBlend` gated by `CurrentGripAlpha`. **[DONE — completed 2026-07-04, see Section 2.5 for branch-filter scope note]**
- [ ] Notify hook: implement an Unlock Actions notify that fires `CharacterBP.bIsBusy = false`. Can reuse Infima's `AN_TFA_UnlockActions` logic pattern or recreate as your own notify class. **[CREATE or REUSE]**
- [ ] Integration checklist:
  - Mesh's Anim Class = `ABP_FP_Default`
  - AnimBP implements your Animation State interface
  - Montages include Left Hand Grip notify-state windows where needed
  - `BP_PlayerCharacter` updates SimulatedVelocity, ADS/recoil/crouch transforms, CurrentGrip **[VERIFY]**

### ABP_TP_Default (Third-Person — your equivalent of ABP_TFA_TP_BaseCharacter)
- [x] On BlueprintInitializeAnimation / BlueprintUpdateAnimation: confirmed handled natively in C++, same as FP side. EventGraph is intentionally non-functional (stub nodes only) — all state read directly via C++ `Get` nodes embedded in AnimGraph. **[VERIFIED — handled natively]**
- [x] Stance Blend node: confirmed present (`BlendListByBool_0`, gated `bIsAiming`, 0.1s/0.1s). **[DONE — confirmed via comparison 2026-07-04]**
- [ ] Aiming Transitions: mesh-space additive state machine layered on top of stance pose, with states Default / Aim Start / Aim End, bound to your rifle's aim-transition animation assets. Confirmed genuinely absent — TFA's own reference implementation of this (`SM_AimingTransitions`) is empty template content (unassigned SequencePlayer pins), so there is no usable content to port, only the structural pattern (see Section 2.5). **[CREATE — build from scratch]**
- [ ] Breathing/Idle Additive: looping sequence applied additively, bound to your rifle's idle loop animation. Same situation as above — TFA's structural slot (`ApplyAdditive_1`, local-space, always-on) is empty; the wiring pattern is worth reusing but the content must be authored. **[CREATE — build from scratch]**
- [x] Montage Slot nodes: confirmed present — `DefaultSlot` and `Aiming`, both sourced from `PreSlotPose`. **[DONE — confirmed via comparison 2026-07-04]**
- [ ] Hand IK with FABRIK node (component space): **currently non-functional — see Section 2.5 for full bug detail and proposed fix.** **[CREATE/FIX — highest priority, see 2.5]**
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
- [x] `UANS_LeftHandGrip` (`Source/ShooterGame/Player/Animation/ANS_LeftHandGrip.h/.cpp`) calls `UpdateLeftHandGrip(bool IsLeftHandOnWeapon, float BlendSpeed)` through the interface on NotifyBegin/NotifyEnd. **[DONE]**
- [x] `UShooterAnimInstanceBase` stores the override into `bLeftHandOnWeaponOverride` / `GripBlendSpeedOverride` and smooths `CurrentGripAlpha` toward 1.0/0.0 via `FInterpTo` inside `UpdateCombatData()`. **[DONE]**

## 2.5 Findings from Infima Source Comparison (2026-07-04)

Comparison audits against the original `ABP_TFA_FP_BaseCharacter`, `ABP_TFA_TP_BaseCharacter`, and `BP_TFA_BaseCharacter` surfaced the following. Full reports saved to `docs/comparisons/`.

### Confirmed bugs requiring fixes
- [ ] **ABP_TP_Default FABRIK is fully non-functional** — three stacked issues: (1) neither `Fabrik_0` (right) nor `Fabrik_1` (left) is wired to `RightHandTransform`/`LeftHandTransform` — both hardcoded identity/constant `Alpha=1.0`; (2) `Fabrik_1`'s effector target bone is incorrectly set to `hand_r` instead of `ik_hand_l`; (3) `BlendListByBool_1` (meant to select between the two arms' results) has a hardcoded unconnected `False` `bActiveValue`, permanently locking output to a cached right-arm-only pose (`RightArmPinned`) — left arm's branch is unreachable regardless of wiring. **[CREATE/FIX — highest priority in this section]**
- [ ] Recommended TP FABRIK fix approach: remove `BlendListByBool_1` selector entirely, chain `Fabrik_1` → `Fabrik_0` sequentially (left into right, matching the working `ABP_FP_Default` and both TFA reference patterns), fix `Fabrik_1`'s tip bone to `ik_hand_l`, wire both effectors to `Get LeftHandTransform`/`Get RightHandTransform`. **[PROPOSED FIX]**

### Verify before trusting existing FP work
- [ ] **FPArms mesh is hidden via `SetVisibility(false, true)` in `BP_PlayerCharacter`'s `BeginPlay`, with no code path anywhere that unhides it** — confirmed intentional (no FP/TP switching exists yet, see Section 7). This means today's `ABP_FP_Default` FABRIK/grip-alpha/camera-blend work has only been verified via compile, not via visible PIE test. **[BLOCKED ON SECTION 7]**
- [ ] Check whether grip pose clips (`AS_FP_AR_Aim/Idle_Pose_Grip_Angled/Vertical`) contain any keyframed right-arm/weapon movement. `ABP_FP_Default`'s grip-alpha `LayeredBoneBlend_2` is currently scoped `root`/full-depth; TFA's equivalent is scoped `clavicle_l`+`ik_hand_l` only (left-arm-only). If clips have incidental right-arm movement, narrow the branch filter to match TFA's scope to prevent right-hand/weapon drift during grip blends. **[VERIFY, LOW PRIORITY]**

### Techniques worth adopting (not gaps — backlog if issues arise)
- [ ] TP recoil: TFA reuses one shared `RecoilTransform` source but reshapes it per-view via fixed per-axis multipliers (translation ×(3.0, 0.5, 4.0), rotation ×(2.0, 4.0, 1.7)) rather than computing a second independent TP recoil value. Relevant once TP's additive stack (Aiming Transitions / Breathing Additive above) is built. **[BACKLOG]**
- [ ] Locomotion transition polish: TFA layers a separate mesh-space additive of transition-only state machines (crouch/run/walk start-stop) on top of the base locomotion loop, rather than relying only on state-machine crossfade times. Consider only if current transitions feel abrupt. **[BACKLOG]**
- [ ] Pre-FABRIK montage slot: TFA positions its `AdditiveSlot` (upper-body montages) before FABRIK, so any montage gets IK-corrected hand placement onto the grip target automatically. Both `ABP_FP_Default` and `ABP_TP_Default` currently place montage slots after FABRIK — montages don't get IK-corrected. Consider inserting a pre-FABRIK slot if reload/inspect animations show hands drifting off the weapon grip. **[BACKLOG]**

### Confirmed non-gaps (no action needed)
- [x] TFA's own FABRIK usage is animation-authored (zero-offset, animator-posed helper bones), not procedural — our C++-driven runtime IK is the correct, more advanced approach. Not a pattern to copy.
- [x] TFA's `Common/Core` reference assets are unconfigured templates (empty SequencePlayer pins, no concrete WeaponConfig instance) — no additional notify classes found beyond what's already listed in Section 3.
- [x] `ABP_TP_Default`'s `AimOffsetPlayer 'AO_ADS'` (aim pitch/yaw blend space) already exceeds TFA's reference, which has no aim-offset/spine-lean mechanism at all. No action needed.
- [x] EventGraphs on both `ABP_FP_Default` and `ABP_TP_Default` are intentionally near-empty — all per-tick state copying is correctly handled in C++ (`UShooterAnimInstanceBase`), matching project architecture. Not a gap.
- [x] No canted-aim state (`bIsCantedAiming`) exists natively — confirmed absent, not currently needed unless canted-aim becomes a design requirement. **[FUTURE DECISION]**
- [x] No recoil escalation/ramp system (TFA's `RecoilRampCount`) exists in `CombatComponent` — gameplay-feel gap, not animation-bridge. Log separately if sustained-fire recoil growth becomes a design goal. **[FUTURE DECISION — not this plan's scope]**

## 3. Notify and Notify-State Wiring

All notify and notify-state classes below can be **recreated as your own classes** (recommended for clean project ownership) rather than reusing Infima's directly, using the same logic pattern. Naming suggestion follows your project convention (`AN_` / `ANS_` prefix, no `TFA` tag needed since these become yours).

### AnimNotify (One-Shot) Classes — recreate as your own

| Suggested Name | What It Should Do | Calls Into | Status |
|---|---|---|---|
| `AN_DropMagazine` | Casts owner to weapon, calls Weapon.SpawnDroppedMagazine(ImpulseForce, RotationForce) at the release frame; weapon spawns a physics magazine actor with linear/angular impulse | `BP_Rifle` | [CREATE] |
| `AN_EjectCasing` | Casts owner to weapon, builds randomized RotationOffset, calls Weapon.EjectCasing(RotationOffset, MinEjectForce, MaxEjectForce, RotationSpeed) | `BP_Rifle` | [CREATE] |
| `AN_SpawnObjectAttached` | Casts owner to character, spawns and attaches a prop tagged DisposableItem (default socket ik_hand_l) | `BP_PlayerCharacter` | [CREATE] |
| `AN_ThrowPhysicsObject` | Casts owner to character, applies linear/angular impulse and optionally clears DisposableItem-tagged actors | `BP_PlayerCharacter` | [CREATE] |
| `AN_UnlockActions` | Casts owner to character, sets Character.bIsBusy = false, reopening gameplay input | `BP_PlayerCharacter` | [CREATE] |

### AnimNotifyState (Time-Window) Classes — recreate as your own

| Suggested Name | What It Should Do | Calls Into | Status |
|---|---|---|---|
| `ANS_BlockADS` | Begin: casts to character, sets bIsAimingBlocked = true, calls ForceStopAiming(). End: sets bIsAimingBlocked = false | `BP_PlayerCharacter` | **[DONE]** — implemented (`ANS_BlockADS.h/.cpp`), calls `SetAimingBlocked` via the interface. The character-side `ForceStopAiming()` hookup still deserves a live [VERIFY] pass |
| `ANS_HideMainMag` | Begin: casts to weapon, hides main magazine mesh. End: shows it again | `BP_Rifle` | [CREATE] |
| `ANS_ShowReserveMag` | Begin: casts to weapon, shows reserve magazine mesh. End: hides it | `BP_Rifle` | [CREATE] |
| `ANS_LeftHandGrip` | Begin: sets IsLeftHandOnWeapon=false via your Animation State interface. End: sets it back to true | Your Animation State interface (implemented on `ABP_FP_Default` / `ABP_TP_Default`) | **[DONE]** — implemented (`ANS_LeftHandGrip.h/.cpp`) |

### Interface
- [x] Animation State interface exists as `IShooterAnimStateInterface`, implemented in C++ on `UShooterAnimInstanceBase` (the shared base both `UShooterFPAnimInstance`/`ABP_FP_Default` and `UShooterTPAnimInstance`/`ABP_TP_Default` derive from) — no need to reimplement per-AnimBP. **[DONE]**

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
- [ ] Hand-IK grip sockets on the **weapon skeletal mesh**, read directly in `UShooterAnimInstanceBase::UpdateIKData()` (hardcoded there, not sourced from `WeaponConfig` like the sockets above):
  - `SOCKET_Grip` — left-hand (foregrip) target, transformed into `hand_r`'s bone space to produce `LeftHandTransform`. Already relied upon by the existing left-hand IK code. **[VERIFY exists on every weapon mesh]**
  - `SOCKET_Grip_R` — right-hand (primary grip) target, transformed into `hand_l`'s bone space to produce `RightHandTransform`. Added 2026-06-30 on the C++ side (`UShooterAnimInstanceBase.h`/`.cpp`) but does not exist on any weapon mesh yet — until it's added, `RightHandTransform` silently stays at its last computed value (identity by default) and the right-arm FABRIK target never moves. **[CREATE per weapon mesh]**
  - Note: the left/right hand IK coupling bug (right hand silently starved if left socket missing) was fixed in `UpdateIKData()` on 2026-07-04, prior to this section's per-weapon socket additions. See Section 2 Shared AnimBP Dependencies. **[FIXED]**
- [ ] Every AnimGraph node referenced in Section 2 (FABRIK hand IK, Layered Blend Per Bone, additive stack, state machines, montage slots) needs to be manually built and verified inside `ABP_FP_Default` and `ABP_TP_Default`, since these are new AnimBPs rather than the Infima originals. **[CREATE + VERIFY — FP largely done, TP FABRIK still broken per Section 2.5]**

## 6. UE5.7 IK Retargeter Workflow (Reference Only — Likely Not Needed)

Since your character uses the standard `SK_Mannequin` skeleton — the same skeleton the Infima Pack targets — **you likely do not need to run an IK Retargeter pass at all**. Retargeting is only required when moving animations between two *different* skeletons with mismatched bone hierarchies. Keep this section only as a fallback reference if you later introduce a custom character skeleton that diverges from standard Manny.

- [ ] If a custom skeleton is introduced later: create IK Rig assets for both source and target skeletons, set Retarget Root to pelvis, and explicitly bind the Arm_L/Arm_R IK Goals to `ik_hand_l`/`ik_hand_r` rather than `hand_l`/`hand_r`.
- [ ] If retargeting becomes necessary: use "Blend To Source" = 1.0 on Arm chains for weapon-holding animations specifically, to preserve exact hand-to-weapon-grip alignment, and reset to 0 for pure locomotion clips.

## 7. FP/TP Camera and Mesh Switching System

- [ ] Confirm whether BP_PlayerCharacter currently has any camera-switching logic at all (check for a SpringArm/Camera component pair, and whether FPArms/TPMesh are both attached simultaneously or need to be spawned/possessed separately). **[VERIFY — confirmed FPArms is hidden in BeginPlay with no unhide path anywhere, per Section 2.5; this system needs to be built as a real feature, not just verified]**
- [ ] Add an input action for toggling view (e.g. `IA_ToggleView`), bound in IMCGameplay, mapped to a player-chosen key. **[CREATE]**
- [ ] Add a replicated or locally-tracked `bIsFirstPerson` state on BP_PlayerCharacter (local-only is fine — this is a client-side camera preference, not gameplay-affecting state that needs server authority). **[CREATE]**
- [ ] Add a `bViewSwitchingAllowed` flag (default true) on BP_PlayerCharacter or a settings data asset — this is the hook to disable the feature later without ripping out the switching logic itself. **[CREATE]**
- [ ] On toggle input: if `bViewSwitchingAllowed` is false, do nothing. Otherwise flip `bIsFirstPerson` and:
  - Switch the active camera view target (FP camera socket vs TP boom camera)
  - Toggle FPArms mesh visibility off when TP is active, and vice versa — keep both meshes simulating/ticking even when hidden, so their AnimBPs don't desync from CharacterBP state
  - No AnimClass swap needed at runtime — ABP_FP_Default and ABP_TP_Default stay permanently assigned to their respective meshes; only visibility and camera view target change
- [ ] In multiplayer: confirm this toggle only affects the local (owning) player's own camera/mesh visibility. Remote clients should always see this player's TPMesh regardless of the owner's local FP/TP toggle state — verify via `SetOwnerNoSee` on FPArms and `SetOnlyOwnerSee` are NOT blocking TPMesh from remote view. **[VERIFY — this is separate from the local toggle and must not be broken by it]**
- [ ] Test in PIE with two clients: local player toggles view, confirms camera and mesh visibility change correctly on their own screen, and confirms the second client still sees this player in TP the entire time regardless of the first player's toggle state. **[VERIFY]**
- [ ] Note for later: if design decides to lock the game to FP-only or TP-only, set `bViewSwitchingAllowed = false` and hide/remove the toggle keybind from the input settings menu — no other code changes needed. **[FUTURE DECISION]**
