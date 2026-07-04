# BP_TFA_BaseCharacter vs BP_PlayerCharacter — Structural Diff

**Reference:** `/Game/Animation/InfimaAnimation/InfimaGames/TacticalFPSAnimations/Common/Core/Characters/BP_TFA_BaseCharacter.BP_TFA_BaseCharacter`
**Mine:** `/Game/Character/BP_PlayerCharacter.BP_PlayerCharacter` (+ native C++ base `AShooterGameCharacter` and supporting components/AnimInstance)

Read-only investigation via UE5 MCP (live Blueprint inspection) + filesystem (C++ source). No assets modified, no compiles run. This is a structural comparison only — no fixes proposed.

Scope caveat: `BP_TFA_BaseWeapon` and `BP_Rifle` (the weapon actors on each side) were **not** inspected — this diff covers the *character* classes only. Any magazine/casing-eject notify comparison is therefore partial (see §3).

---

## 1. Variables/properties BP_TFA_BaseCharacter exposes to its AnimBPs

| Variable | Type | Purpose |
|---|---|---|
| `bIsRunning` | bool | Locomotion |
| `bAnimateCamera` | bool | Head-bone camera animation toggle |
| `bIsAiming` | bool | ADS state |
| `bIsAimingBlocked` | bool | ADS gate |
| `bIsCantedAiming` | bool | Canted-ADS variant |
| `bIsBusy` | bool | Action lock |
| `bIsSprinting` | bool | Locomotion |
| `bIsWalking` | bool | Locomotion |
| `bIsCrouching` | bool | Stance |
| `RawInputVector` | Vector2D | Keyboard input, feeds locomotion |
| `SimulatedVelocity` | Vector | Fake velocity (demo has no real movement) |
| `CurrentStance` | Enum `E_TFA_Stance` {Stand, Crouch} | Stance |
| `CurrentFireMode` | Enum `E_TFA_FireMode` {Safety, Semi, Auto} | Fire mode |
| `RecoilRampCount` | int | Escalating recoil while holding trigger |
| `CurrentRecoil` / `TargetRecoil` | FTransform | Procedural recoil (current + spring target) |
| `CurrentAimDownSightsOffset` / `TargetAimDownSightsOffset` | FTransform | Procedural ADS (current + spring target) |
| `CurrentCrouchOffset` / `TargetCrouchOffset` | FTransform | Procedural crouch (current + spring target) |
| `TargetCantedOffset` | FTransform | Canted-aim target transform |
| `SpringAimDownSights` / `SpringRecoil` / `SpringCrouch` | struct `S_TFA_SpringData` | Critically-damped spring state per procedural channel |
| `CurrentCameraPerspective` | Enum `E_TFA_CameraPerspectives` {FirstPerson, ThirdPerson, GunCamera, HelmetCamera, ChestCamera} | Active view mode; drives mesh/camera swap |
| `CurrentGrip` | Enum `E_TFA_GripAttachment` {None, Vertical, Angled} | Foregrip type |
| `WeaponConfig` / `CurrentWeaponActor` | Object refs | Weapon data asset + spawned weapon actor |
| Misc (`RandomAnimIndex`, `WidgetBP`, `AutoFireTimerHandle`, `InitialCameraDistance`/`Min`/`Max`/`ZoomStep`, `DefaultFOV`/`AimedFOV`, spotlight intensities, quit-game UX bools) | — | Demo-only scaffolding, not animation-relevant |

Notably absent even on the reference side: no hand-IK bone-space transforms and no grip blend-alpha — see §5.

---

## 2. Does BP_PlayerCharacter / native C++ expose an equivalent for each?

| TFA variable | Native equivalent | Status |
|---|---|---|
| `bIsRunning` | `UShooterAnimInstanceBase::bIsRunning` | ✅ Handled natively |
| `bAnimateCamera` | `UShooterFPAnimInstance::bAnimateCamera` | ✅ Handled natively — but hardcoded relevant-only-on-FP (SKM_FP_Arms has no head bone); functionally narrower than TFA's, see §5 |
| `bIsAiming` | `UShooterAnimInstanceBase::bIsAiming` (truth lives on `CombatComponent::bAiming`, replicated) | ✅ Handled natively |
| `bIsAimingBlocked` | `UShooterAnimInstanceBase::bIsAimingBlocked` + `CombatComponent::bIsAimingBlocked` (server) + `bIsAimingBlockedLocal` (notify-driven shadow) | ✅ Handled natively — more elaborate (server + local shadow) than TFA's single bool |
| `bIsCantedAiming` | — | ❌ **Missing.** No canted-aim state found anywhere in native code. |
| `bIsBusy` | `UShooterAnimInstanceBase::bIsBusy` | ✅ Handled natively — but driven indirectly via `CombatComponent::SetCombatAction()`, not a directly-settable flag (see §3, §5) |
| `bIsSprinting` | `AShooterGameCharacter::bIsSprinting` + `UShooterAnimInstanceBase::bIsSprinting` | ✅ Handled natively |
| `bIsWalking` | `UShooterAnimInstanceBase::bIsWalking` | ✅ Handled natively |
| `bIsCrouching` | `bIsCrouched` (standard ACharacter) + `EShooterStance CurrentStance` | ✅ Handled natively |
| `RawInputVector` | `AnimationInputMoveVector` (character) / `InputMoveVector` (AnimInstance) | ✅ Handled natively |
| `SimulatedVelocity` | N/A — real `CharacterMovementComponent` drives `Speed`/`Direction`/`bIsAccelerating` | ➖ Not applicable — TFA's hack exists only because its demo has no real movement; native doesn't need it |
| `CurrentStance` {Stand, Crouch} | `EShooterStance CurrentStance` {Standing, Crouching, **Aiming**} | ✅ Handled natively (superset — extra Aiming state) |
| `CurrentFireMode` | Implied by `CycleFireModeButtonPressed`/`ServerCycleFireMode` on `CombatComponent` | ✅ Present — exact storage location not confirmed in this pass |
| `RecoilRampCount` | — | ❌ **Possible gap.** No escalating ramp counter found in `CombatComponent` (only a flat `RecoilDecaySpeed`). Sustained-fire recoil escalation may not exist natively. |
| `CurrentRecoil`/`TargetRecoil` (spring FTransform) | `RecoilTransform` (character+AnimInstance) + `CombatComponent::RecoilRotationTarget`(FRotator)/`RecoilTranslationTarget`(FVector) | ✅ Handled natively — decomposed rotation/translation rather than one FTransform, and likely simple interp rather than spring physics (see §5) |
| `CurrentAimDownSightsOffset`/`TargetAimDownSightsOffset` | `AimDownSightsTransform` + `CombatComponent::ADSLocationTarget`/`ADSRotationTarget` + **`ADSAlpha`** (extra) | ✅ Handled natively — plus an explicit blend-alpha TFA doesn't have |
| `CurrentCrouchOffset`/`TargetCrouchOffset` | `CrouchTransform` | ✅ Handled natively — no separate current/target spring pair found; likely direct interpolation |
| `SpringAimDownSights`/`SpringRecoil`/`SpringCrouch` | — | ❌ **No spring-physics system found.** Native uses fixed-speed interpolation floats (`ADSInterpSpeed`, `RecoilDecaySpeed`) instead of critically-damped springs. Behavioral difference, not necessarily a defect. |
| `CurrentCameraPerspective` (5-way) | — | ❌ **Missing — biggest structural gap.** No perspective-switching system exists at all. See §4/§5. |
| `CurrentGrip` {None, Vertical, Angled} | `EWeaponGrip CurrentGrip` (AnimInstance + `CombatComponent::GetCurrentGrip`) | ✅ Handled natively — exact match, plus `CurrentGripAlpha` (extra, needed because native blends via IK rather than swapping meshes — see §5) |
| `WeaponConfig`/`CurrentWeaponActor` | `UWeaponConfig`/`DA_RifleBaseConfig` pattern + `CombatComponent::EquippedWeapon` (`AWeapon*`, replicated) | ✅ Handled natively |
| Demo-only scaffolding (zoom, FOV steps, spotlights, quit-UX, random-anim-index, widget refs) | — | ➖ Not applicable / out of scope for a shipping game, except `RandomAnimIndex`'s idle-variation purpose, which has no native equivalent (minor, cosmetic) |

**Native-only additions with no TFA counterpart** (not gaps — listed for completeness, see §5 for why): `LeftHandTransform`/`RightHandTransform`/`bLeftHandOnWeapon` (hand-IK effectors), `CurrentGripAlpha`/`bGripOverrideActive`/`bLeftHandOnWeaponOverride`/`GripBlendSpeedOverride` (grip blend system), `ADSAlpha`, `ETurningInPlace TurningInPlace`, `EPlayerWeaponStance WeaponStance`, `bInCombatState`/`bHighReady`, full prone system (`bIsProne`, `ProneArmLength`, `ProneCapsuleHalfHeight`, `ProneWalkSpeed`), `Health`/`MaxHealth`.

---

## 3. Notify/event hookups on BP_TFA_BaseCharacter vs native equivalents

| TFA function/event | Behavior | Native status |
|---|---|---|
| `ThrowPhysicsObject` | Spawns an actor at a socket, applies linear+angular impulse, optionally clears `"DisposableItem"`-tagged attachments | ⚠️ **Partial.** `AShooterGameCharacter::ThrowPhysicsObject(...)` exists with a matching signature but is a **stub** (per Agent inspection). No `AN_ThrowPhysicsObject` notify class exists yet to call it — confirmed `[CREATE]` in `infima_integration_plan.md`. |
| `SpawnObjectAttached` (custom event) | Spawns+attaches an actor at `ik_hand_l`, tags it `DisposableItem`, plays an animation on it, delays visibility | ⚠️ **Partial.** `AShooterGameCharacter::SpawnObjectAttached(...)` stub exists, matching params; no `AN_SpawnObjectAttached` notify class exists yet — confirmed `[CREATE]`. |
| `DestroyAttachedItem` | Destroys all `"DisposableItem"`-tagged attachments | ❌ **Missing.** No equivalent found; not yet needed since the two functions above aren't wired to notifies yet either. |
| `ForceStopAiming` (custom event) | Forces `bIsAiming=false`, resets ADS target transform — called by TFA's ADS-block logic | ⚠️ **Unverified/likely incomplete.** `ANS_BlockADS` exists and calls `SetAimingBlocked()` through the interface, but that only sets `bIsAimingBlockedLocal` on the AnimInstance — no `ForceStopAiming()`-named function was found on `AShooterGameCharacter`, and CLAUDE.md itself flags "`ForceStopAiming()` hookup still deserves a live [VERIFY] pass." It's unclear whether blocking ADS via this notify actually cancels an in-progress aim on the character/combat side, or only suppresses the anim-side visual. |
| `StopWeaponAnimation` | Stops the weapon mesh's active montage, sets `bIsBusy=false` | ⚠️ **Different mechanism, not missing.** Native's busy-clearing is state-driven: `CombatComponent::SetCombatAction(ECombatAction::None)` sets `bIsBusy=false` as a side effect of ending a combat action — it isn't triggered by an AnimNotify at all. No `AN_UnlockActions` notify class exists — confirmed `[CREATE]`. The closest notify-driven unlock that *does* exist is `AnimNotify_InteractionFinished()`, but it's private and scoped only to interaction montages, not general action-locking. |
| *(no TFA equivalent)* | — | `ANS_LeftHandGrip` and `ANS_BlockADS` exist natively with no TFA-side counterpart function found by name — these implement native's IK-blend system, which TFA's static-mesh-swap grip approach doesn't need (see §5). |

**Not comparable at the character level (weapon-actor scoped on both sides, out of this diff's inspection scope):** magazine-drop, casing-eject, and hide/show-magazine notify targets. TFA's demo doesn't appear to implement these on the character either (none found in Agent A's function/event catalog); the project's own plan already places `AN_DropMagazine`, `AN_EjectCasing`, `ANS_HideMainMag`, `ANS_ShowReserveMag` on `BP_Rifle`, not the character — consistent architecture, just unverified since `BP_TFA_BaseWeapon`/`BP_Rifle` weren't inspected in this pass.

---

## 4. Component structure differences (weapon-holding/aiming relevant)

| Aspect | TFA | Native (BP_PlayerCharacter + AShooterGameCharacter) |
|---|---|---|
| Body mesh | **One** `SkeletalMeshComponent` (`CharacterMesh0`) whose `SkeletalMesh` asset is **swapped at runtime** between FP arms and TP body depending on `CurrentCameraPerspective` | **Two permanent, separate components**: `CharacterMesh0` (native `Mesh`, always TP body — `SKM_Manny_Simple` + `ABP_TP_Default_C`) and `FPArms` (BP-only `SkeletalMeshComponent`, always FP arms — `SKM_FP_Arms` + `ABP_FP_Default_C`). Nothing is swapped; both exist simultaneously. |
| Camera | **One** `CameraFP` (fixed) + **one** `CameraTP` that is **dynamically re-parented** at runtime (spring arm in TP mode; weapon's `SocketGunCamera` in gun-cam mode; helmet/chest sockets in bodycam modes) | **Two permanent, separately-parented cameras**: `FollowCamera` (native, fixed on `CameraBoom` spring arm — always active, TPS) and `CameraFP` (BP-only, parented to `CharacterMesh0`, **static**, never reattached). No dynamic reattachment logic found anywhere. |
| **FP visibility (flagged finding)** | FP mesh is shown by `EnableFirstPersonPerspective()` whenever that perspective is active | **`FPArms` is hidden via `SetVisibility(false, true)` in the Blueprint's `BeginPlay` — and nothing else found in the inspected BP/C++ ever makes it visible again.** There is no perspective-switching system to trigger a reveal. This looks like either dormant/unfinished scaffolding, or camera-mode switching lives in code/BP not covered by this diff (e.g. a PlayerController camera manager). Worth a direct look before assuming the FP rig is "wired up." |
| Perspective/view-mode system | 5-way state machine (`E_TFA_CameraPerspectives`), each mode swaps mesh assignment, camera parent, FOV, post-process (bodycam), and audio muffling | **None found.** No equivalent enum, no `Enable*Perspective`-style functions on `AShooterGameCharacter` or in the BP graph. |
| Spotlight/post-process per mode | `SpotLight` + `PP_Bodycam` components toggled per perspective | Not present in `BP_PlayerCharacter`'s component list. |
| Weapon attachment | Weapon is a separate actor (`BP_TFA_BaseWeapon`), spawned by `SpawnWeapon()`, attached to `CharacterMesh0` at `WeaponConfig.SocketGunAttachment` | Weapon is also a separate actor (`BP_Rifle`), but equipped through `UCombatComponent::EquipWeapon()` driven by `ULoadoutComponent`/inventory data rather than a single `SpawnWeapon` call reading one `WeaponConfig` reference. Architecturally analogous (weapon ≠ character component) but routed through a richer inventory/loadout pipeline. Exact attach socket on the native side wasn't confirmed in this pass. |
| Grip visualization mechanism | Implied static-mesh swap per `CurrentGrip` value (no IK plumbing found at all — see §5) | **FABRIK-based hand IK.** `LeftHandTransform`/`RightHandTransform` are bone-space effector targets computed each tick by reading `SOCKET_Grip` (left) and `SOCKET_Grip_R` (right) sockets on the *weapon mesh*, feeding FABRIK nodes in the AnimGraph. Fundamentally different socket vocabulary and attachment philosophy — see §5. |

---

## 5. Other structural differences worth flagging

- **Grip philosophy: pose-swap vs IK-solve.** This is the single biggest architectural divergence underlying several of the above points. TFA appears to represent different grip attachments by changing which grip mesh/pose is shown (no hand-IK variables exist on the reference character at all). The native project instead computes real-time FABRIK IK targets from weapon sockets (`SOCKET_Grip`/`SOCKET_Grip_R`) and smoothly blends the effector influence in/out via `CurrentGripAlpha`, driven by `ANS_LeftHandGrip` notify windows. Neither approach is "more complete" than the other in the abstract, but it means TFA's Blueprint graphs are not a valid reference for how grip blending *should* work natively — the native system had to be designed from scratch, not ported.

- **Procedural motion math: spring-damper vs interp-to-speed.** TFA computes all procedural offsets (ADS, recoil, crouch) through a shared critically-damped spring solver (`CalculateSpring`, with mass/stiffness/damping parameters) evaluated every tick. Native's equivalents use fixed-speed interpolation floats (`ADSInterpSpeed=10`, `RecoilDecaySpeed=8`) — simpler, but a different feel. Not a gap, but relevant if animation "snappiness"/overshoot behavior is ever compared side-by-side.

- **Recoil escalation.** TFA ramps recoil magnitude the longer the trigger is held (`RecoilRampCount`, incremented in `AddRecoil`). No equivalent counter was found in `CombatComponent` — sustained-fire recoil growth may simply not exist yet natively. This is a gameplay-feel gap, not just an animation-bridge one.

- **Action-lock model: flat bool vs enum state machine.** TFA gates nearly every action behind one flat `bIsBusy` bool, set/cleared directly by whichever function is running. Native instead infers "busy" from `ECombatAction CurrentAction` via `SetCombatAction()`, with separate `bIsReloading`/`bIsInteracting`/`bInCombatState`/`bHighReady` flags layered on top. This is more granular and extensible, but it means a future `AN_UnlockActions` notify can't just "set bIsBusy=false" the way TFA's `StopWeaponAnimation` does — it would need to call back into `CombatComponent::SetCombatAction(None)` (or equivalent) to stay consistent with the state machine. Worth deciding deliberately when that notify class is eventually built.

- **Replication.** TFA has zero `ReplicatedUsing`/replication anywhere — it's a single-player demo. Every corresponding piece of native state is properly replicated (`OnRep_EquippedWeapon`, `OnRep_Aiming`, `OnRep_CombatAction`, `OnRep_HighReady`, `OnRep_Health`, `OnRep_IsProne`, `OnRep_DesiredYaw`, etc.), consistent with CLAUDE.md's server-authoritative pillar. This underlies *why* several native variables are more decomposed than TFA's (e.g. separate `ADSLocationTarget`/`ADSRotationTarget`/`ADSAlpha` instead of one local `FTransform` + spring struct) — they're shaped for replication and server-authority, not just local Blueprint convenience.

- **Native-only gameplay systems with no TFA counterpart at all** (not gaps, just scope differences worth remembering when reasoning about "what TFA would do here"): full prone stance (`bIsProne`, prone-specific arm length/capsule/walk-speed tuning), turn-in-place (`ETurningInPlace`), high-ready pose (`bHighReady`/`ToggleHighReady`), combat-state timeout (`bInCombatState`, `EnterCombatState`/`ExitCombatState`), health/damage (`Health`/`TakeDamage`), and the entire interaction-prompt system (`FindBestInteractableInView`, prompt widgets, etc.). TFA is a bare movement/animation showcase; the native project is a full replicated game, so a lot of "native-only" surface area is simply out of TFA's scope by design, not a sign anything is missing.

- **`EPlayerWeaponStance WeaponStance`** — a separate enum from both `CurrentStance` and `CurrentAction` was found on `UShooterAnimInstanceBase`, with no obvious TFA analog identified in this pass. Its exact purpose relative to `CurrentStance` wasn't pinned down — worth a closer look if stance-driven animation blending ever behaves unexpectedly.

- **Fire-mode storage location.** Native clearly *has* fire-mode cycling (`CycleFireModeButtonPressed` → `CombatComponent::ServerCycleFireMode`), but neither agent's pass pinned down exactly where the current fire-mode enum value is stored (likely on `UWeaponConfig`/`AWeapon` rather than the character/AnimInstance, unlike TFA's character-level `CurrentFireMode`). Not treated as a gap, just an unconfirmed detail.

---

## Open items for follow-up (not achievable read-only / out of this diff's scope)

1. **Why is `FPArms` hidden on `BeginPlay` with nothing found to unhide it?** Confirm whether a first-person camera mode is planned/wired elsewhere, or whether the FP rig is intentionally dormant scaffolding for later milestones.
2. **Does `ANS_BlockADS` actually cancel an in-progress aim** on the character/`CombatComponent` side, or only suppress the anim-side visual? (CLAUDE.md already flags this as unverified.)
3. `BP_TFA_BaseWeapon` vs `BP_Rifle` were not inspected — magazine/casing-eject notify comparison in §3 is based on absence-of-evidence on the character side only.
4. Exact native storage location for the current fire-mode enum, and the exact socket `BP_Rifle` attaches to on the character, were not confirmed in this pass.
