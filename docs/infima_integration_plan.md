# ShooterGame — Infima Pack Integration Plan (Execution-Ordered)

> Work through phases in order. Each phase is self-contained — do not start a phase until the prior phase's checkboxes are complete. Items marked **[CREATE]** don't exist yet and need to be built. **[FIX]** means broken and needs correction. **[VERIFY]** means confirm in-editor. **[TRANSFER]** means migrate from Infima plugin content into project folders.

---
## Related Comparison Reports

Full technical audits backing the fixes in this plan live in `docs/comparisons/`. Consult these before touching the referenced sections — they contain exact node names, branch-filter values, and wiring details not repeated in full here.

| Report | Covers | Verdict |
|---|---|---|
| `ABP_TFA_FP_BaseCharacter_vs_ABP_FP_Default.md` | Full AnimGraph/EventGraph diff of the FP AnimBP against Infima's reference | FP is largely complete — FABRIK, grip overlay, and camera blend are done; see Phase 4 for remaining verification/notify items |
| `ABP_TFA_TP_BaseCharacter_vs_ABP_TP_Default.md` | Full AnimGraph/EventGraph diff of the TP AnimBP against Infima's reference | TP FABRIK is confirmed broken (unwired effectors, wrong tip bone, dead selector node) — see Phase 1 for the exact fix steps |
| `BP_TFA_BaseCharacter_vs_BP_PlayerCharacter.md` | Character Blueprint diff (camera/perspective system, action gating, weapon ownership) | Referenced inside both AnimBP reports above as prior groundwork — confirms no perspective-switching system exists yet, which is why Phase 3 (camera/mesh switching) is required |

If a fix in this plan seems ambiguous or under-specified, check the matching report first before asking for clarification — the reasoning and exact bug detail is usually already documented there.
---

## Execution Roadmap

1. Phase 1 — TP FABRIK Fix (`ABP_TP_Default`)
2. Phase 2 — Weapon Mesh Socket Addition (`SOCKET_Grip_R`)
3. Phase 3 — FP/TP Camera and Mesh Switching System
4. Phase 4 — Remaining AnimBP Work (`ABP_FP_Default` / `ABP_TP_Default`)
5. Phase 5 — Notify and Notify-State Classes
6. Phase 6 — Supporting Actor Migration
7. Phase 7 — Weapon and Magazine AnimBPs
8. Phase 8 — Final Verification Sweep (Sections 1 and 6 reference items)

---

## Phase 1 — TP FABRIK Fix

**Target asset:** `ABP_TP_Default` AnimGraph.

**Confirmed bugs (comparison audit, 2026-07-04):**
1. Neither `Fabrik_0` (right arm) nor `Fabrik_1` (left arm) is wired to `RightHandTransform`/`LeftHandTransform` — both nodes currently use hardcoded identity targets with `Alpha` pinned to `1.0`.
2. `Fabrik_1`'s effector target bone is set to `hand_r` — this is wrong, it must be `ik_hand_l`.
3. `BlendListByBool_1` (the node meant to select between the left-arm and right-arm FABRIK outputs) has a hardcoded, unconnected `bActiveValue = False` — this permanently locks the output to a cached pose called `RightArmPinned`, meaning the left-arm branch can never be reached regardless of any other fix.

**Steps:**
1. Open `ABP_TP_Default` in the Animation Blueprint editor, navigate to the AnimGraph.
2. Delete `BlendListByBool_1` entirely — do not attempt to fix its bool input, it is structurally the wrong pattern here.
3. Rewire the graph so `Fabrik_1` (left arm) feeds directly into `Fabrik_0` (right arm) as a sequential chain — left arm IK resolves first, then right arm IK resolves on top of that result. This matches the working pattern already built in `ABP_FP_Default`.
4. On `Fabrik_1`: change the effector target bone from `hand_r` to `ik_hand_l`.
5. Wire `Fabrik_1`'s effector Target Transform input to a `Get LeftHandTransform` node (pulls from `UShooterAnimInstanceBase::LeftHandTransform`).
6. Wire `Fabrik_0`'s effector Target Transform input to a `Get RightHandTransform` node (pulls from `UShooterAnimInstanceBase::RightHandTransform`).
7. Confirm both FABRIK nodes have: Effector Transform Space = Bone Space, Effector Rotation Source = CopyFromTarget, Precision = 0.01, LODThreshold exposed.
8. Compile and save.
9. **[VERIFY]** — cannot be visually confirmed in PIE until Phase 2 (socket) and Phase 3 (TP mesh must be visible) are complete. Compile-clean is the only verification possible right now.

---

## Phase 2 — Weapon Mesh Socket Addition

**Target asset:** every weapon skeletal mesh currently in use (start with the rifle receiver mesh used by `BP_Rifle`).

**Context:** `SOCKET_Grip` (left-hand target) already exists and works. `SOCKET_Grip_R` (right-hand target) was added on the C++ side on 2026-06-30 but has never been added to any weapon mesh — until this phase, `RightHandTransform` silently stays at identity and the right-arm FABRIK target never moves, in both FP and TP.

**Steps:**
1. Open the rifle's receiver skeletal mesh asset in the Skeletal Mesh Editor.
2. Go to the Skeleton tab, right-click the appropriate bone (the same bone `SOCKET_Grip` is parented to, or the primary-grip bone — confirm bone name before adding), and add a new socket named exactly `SOCKET_Grip_R`.
3. Position and rotate the socket to sit at the primary (right-hand) grip point on the mesh — use `SOCKET_Grip`'s placement as a visual reference for scale/offset conventions.
4. Save the mesh.
5. Repeat steps 1–4 for every additional weapon mesh in the project that will use hand IK.
6. **[VERIFY]** — PIE test is still blocked until Phase 3, but confirm the socket appears correctly positioned in the Skeletal Mesh Editor preview.

---

## Phase 3 — FP/TP Camera and Mesh Switching System

**Why this phase is next:** `FPArms` mesh is currently hidden via `SetVisibility(false, true)` in `BP_PlayerCharacter`'s `BeginPlay`, with no code path anywhere that unhides it. This blocks any real PIE verification of Phase 1, Phase 2, and all prior FP/TP AnimBP work. This must be built before further animation polish is worth doing.

**Steps:**
1. In `BP_PlayerCharacter`, confirm current state: locate the `SpringArm`/`Camera` component pair, confirm whether `FPArms` and `TPMesh` are both attached simultaneously (expected) or spawned/possessed separately.
2. **[CREATE]** Add a new Input Action `IA_ToggleView` in the project's Input folder.
3. Bind `IA_ToggleView` inside `IMCGameplay` (the gameplay input mapping context) to a placeholder key (e.g. `V`) — user can rebind later.
4. **[CREATE]** Add a boolean variable `bIsFirstPerson` to `BP_PlayerCharacter`. Local-only (not replicated) — this is a client-side camera preference, not gameplay-authoritative state.
5. **[CREATE]** Add a boolean variable `bViewSwitchingAllowed` (default `true`) to `BP_PlayerCharacter` (or a settings data asset if the project already has one) — this is the future on/off hook for locking the game to FP-only or TP-only.
6. Implement the toggle input handler:
   - On `IA_ToggleView` triggered: if `bViewSwitchingAllowed == false`, do nothing (Return).
   - Otherwise, flip `bIsFirstPerson`.
   - Switch the active camera view target: FP camera socket if `bIsFirstPerson == true`, TP boom camera if `false`.
   - Toggle `FPArms` visibility off when TP is active, and vice versa for `TPMesh`. Use `SetVisibility` only — do **not** stop ticking/simulating either mesh, so `ABP_FP_Default` and `ABP_TP_Default` don't desync from `CharacterBP` state while hidden.
   - Do **not** swap Anim Class at runtime. `ABP_FP_Default` and `ABP_TP_Default` stay permanently assigned to their respective meshes — only visibility and camera view target change.
7. **[VERIFY]** Multiplayer correctness: confirm this toggle only affects the local (owning) player's own camera/mesh visibility. Check `FPArms` has `SetOwnerNoSee` enabled so remote clients never see it. Check `TPMesh` does **not** have `SetOnlyOwnerSee` enabled — remote clients must always see this player's `TPMesh` regardless of the owner's local toggle state.
8. **[VERIFY]** Test in PIE with two clients: local player toggles view repeatedly, confirm camera and mesh visibility change correctly on their own screen only; confirm the second client sees this player in TP the entire time regardless of the first player's toggle state.
9. Note for later, no action now: if design later locks the game to FP-only or TP-only, set `bViewSwitchingAllowed = false` and remove the toggle keybind from the input settings menu — no other code changes needed.

---

## Phase 4 — Remaining AnimBP Work

Now that FP/TP visibility is real, verify and complete the following.

### ABP_FP_Default
1. **[VERIFY]** Now that `FPArms` is actually visible in PIE, confirm the FABRIK hand IK, grip-alpha overlay (`BlendListByBool` cascade + `LayeredBoneBlend`), and camera/head `LayeredBoneBlend` gated by `bAnimateCamera` all behave correctly in a live session — this was previously only compile-verified.
2. **[CREATE]** Implement the Unlock Actions notify hook: on notify fire, set `CharacterBP.bIsBusy = false`. Either port the logic pattern from Infima's `AN_TFA_UnlockActions`, or build fresh as your own notify class (see Phase 5).
3. Integration checklist — confirm each of the following is true:
   - Mesh's Anim Class = `ABP_FP_Default`
   - AnimBP implements `IShooterAnimStateInterface`
   - Montages include Left Hand Grip (`ANS_LeftHandGrip`) notify-state windows where needed
   - `BP_PlayerCharacter` is correctly updating `SimulatedVelocity`, ADS/recoil/crouch transforms, and `CurrentGrip`

### ABP_TP_Default
1. **[VERIFY]** Confirm the Phase 1 FABRIK fix behaves correctly now that `TPMesh` visibility is real and controllable.
2. **[CREATE]** Build the Aiming Transitions mesh-space additive state machine: states Default / Aim Start / Aim End, layered on top of the stance pose, bound to your rifle's aim-transition animation assets. Note: Infima's own reference implementation of this (`SM_AimingTransitions`) is an empty, unconfigured template — there is no usable animation content to port, only the structural pattern. Animation content must be authored or sourced separately.
3. **[CREATE]** Build the Breathing/Idle Additive: a looping sequence applied additively (local-space, always-on), bound to your rifle's idle loop animation. Same situation as above — Infima's `ApplyAdditive_1` slot is structurally present but empty; only the wiring pattern is reusable.
4. **[CREATE]** Implement the Unlock Actions notify hook, same pattern as FP side.
5. Integration checklist — confirm:
   - Mesh's Anim Class = `ABP_TP_Default`
   - Owning pawn is `BP_PlayerCharacter`
   - AnimBP implements `IShooterAnimStateInterface`
   - Montages include Left Hand Grip notify-state windows where expected
   - `DA_RifleBaseConfig` provides TP idle pose, TP idle loop, TP aim pose, TP aim-start/aim-end transitions

---

## Phase 5 — Notify and Notify-State Classes

Recreate as your own classes (not direct Infima reuse) using the same logic pattern. Prefix convention: `AN_` for one-shot, `ANS_` for notify-state, no `TFA` tag.

### One-Shot Notifies — [CREATE]
| Class Name | Logic | Calls Into |
|---|---|---|
| `AN_DropMagazine` | Cast owner to weapon, call `Weapon.SpawnDroppedMagazine(ImpulseForce, RotationForce)` at release frame | `BP_Rifle` |
| `AN_EjectCasing` | Cast owner to weapon, build randomized `RotationOffset`, call `Weapon.EjectCasing(RotationOffset, MinEjectForce, MaxEjectForce, RotationSpeed)` | `BP_Rifle` |
| `AN_SpawnObjectAttached` | Cast owner to character, spawn+attach prop tagged `DisposableItem` at socket (default `ik_hand_l`) | `BP_PlayerCharacter` |
| `AN_ThrowPhysicsObject` | Cast owner to character, apply linear/angular impulse, optionally clear `DisposableItem`-tagged actors | `BP_PlayerCharacter` |
| `AN_UnlockActions` | Cast owner to character, set `Character.bIsBusy = false` | `BP_PlayerCharacter` |

Default parameter values (starting reference, tune per weapon feel):
- `AN_DropMagazine`: ImpulseForce = -50.0, RotationForce = 150.0
- `AN_EjectCasing`: MinEjectForce = 50.0, MaxEjectForce = 65.0, RotationSpeed = 0.0
- `AN_SpawnObjectAttached`: SpawnSocketName = ik_hand_l, LocationOffset = (0,0,0), RotationOffset = (0,0,0), VisibilityDelay = 0.0
- `AN_ThrowPhysicsObject`: SpawnSocketName = ik_hand_l, ThrowForce = 80.0, ThrowRotationForce = 80.0, DestroySocketItem = true

### Notify-States — [CREATE except where noted DONE]
| Class Name | Logic | Calls Into | Status |
|---|---|---|---|
| `ANS_BlockADS` | Begin: `bIsAimingBlocked = true` + `ForceStopAiming()`. End: `bIsAimingBlocked = false` | `BP_PlayerCharacter` | **[DONE]** — implemented, `ForceStopAiming()` hookup still needs a live [VERIFY] pass |
| `ANS_HideMainMag` | Begin: hide main magazine mesh. End: show it again | `BP_Rifle` | [CREATE] |
| `ANS_ShowReserveMag` | Begin: show reserve magazine mesh. End: hide it | `BP_Rifle` | [CREATE] |
| `ANS_LeftHandGrip` | Begin: `IsLeftHandOnWeapon = false` via interface. End: `= true` | `IShooterAnimStateInterface` | **[DONE]** — implemented |

Default values: `ANS_LeftHandGrip`: BlendOutSpeed = 15.0, BlendReturnSpeed = 15.0

### Owner Requirements
- `AN_DropMagazine`, `AN_EjectCasing`, `ANS_HideMainMag`, `ANS_ShowReserveMag` — only function on montages playing on a mesh owned by `BP_Rifle`.
- `AN_SpawnObjectAttached`, `AN_ThrowPhysicsObject`, `AN_UnlockActions`, `ANS_BlockADS` — only function on montages playing on a mesh owned by `BP_PlayerCharacter`.
- `ANS_LeftHandGrip` — only functions if the active anim instance implements `IShooterAnimStateInterface`.

---

## Phase 6 — Supporting Actor Migration

Migrate these from the Infima plugin content into your own project folders (e.g. `Content/ShooterGame/Weapons/Physics/`) so they survive a future demo-content cleanup pass. Use Unreal's **Migrate** tool (right-click asset → Asset Actions → Migrate).

1. **[TRANSFER]** `BP_TFA_BaseMagazine` → migrate, rename if desired (e.g. `BP_MagazineCosmetic`)
2. **[TRANSFER]** `BP_TFA_BasePhysicsObject` → migrate, shared base class for dropped physics props
3. **[TRANSFER]** `BP_TFA_PhysicsMagazine` → migrate, used by `AN_DropMagazine`
4. **[TRANSFER]** `BP_TFA_PhysicsCasing` → migrate, used by `AN_EjectCasing`
5. **[TRANSFER]** `BP_TFA_AttachmentLaser` → migrate only if laser attachments are planned for `BP_Rifle`
6. After migrating each asset, fix up any broken references in `BP_Rifle` and `DA_RifleBaseConfig`.

---

## Phase 7 — Weapon and Magazine AnimBPs

1. **[CREATE]** Weapon AnimBP for the `ABPWeapon` field on `DA_RifleBaseConfig` — none exists yet. Reuse an Infima weapon AnimBP as a starting template, or build fresh, then assign it.
2. **[CREATE]** Magazine AnimBP for the `ABPMagazine` field — same situation, build or reuse, then assign.

---

## Phase 8 — Final Verification Sweep

Quick confirmation-only pass, no new building expected.

### Skeleton and Retarget
1. **[VERIFY]** Player character mesh is assigned to `SK_Mannequin`, and `BP_PlayerCharacter` references that mesh.
2. **[VERIFY]** FABRIK IK bone chains present: `clavicle_r`→`hand_r`→`ik_hand_r`, `clavicle_l`→`hand_l`→`ik_hand_l`.
3. **[VERIFY]** `DA_RifleBaseConfig` FP/TP mesh fields point to actual character meshes, not Infima demo meshes.
4. **[VERIFY]** Custom weapon receiver meshes were imported with the correct weapon skeleton on import.

### IK Retargeter (reference only)
5. Confirmed not needed — character uses standard `SK_Mannequin`, same as Infima's target skeleton. Keep as fallback reference only if a divergent custom skeleton is introduced later.

### Sockets (non-IK)
6. **[VERIFY or CREATE per-socket]** Confirm these exist on the rifle/character mesh: `SocketMagazineAttachment`, `SocketMagazineReserveAttachment`, `SocketCasingEject`, `SocketGripAttachment`, `SocketLaserAttachment`, `SocketGunAttachment`, `SocketGunCamera`, `SocketHelmetCamera`, `SocketChestCamera`, `SocketMuzzle`.
