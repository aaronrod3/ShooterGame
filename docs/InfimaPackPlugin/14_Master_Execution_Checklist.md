# Step 14 — Master Execution Checklist

The condensed, ordered version of Steps 1–13 — everything needed to go from an empty UE5 project to a fully working implementation of the Tactical FPS Animations pack, exactly as Infima's own documentation lays it out. Use the numbered files for full detail on any given step; use this file to track overall progress.

---

## Phase A — Get the pack in and understand it (Steps 1–2)

- [ ] Confirm Unreal Engine 5.4+.
- [ ] Install the weapon product via Fab.
- [ ] Locate `InfimaGames/TacticalFPSAnimations`, understand the `Common/` vs. `Weapons/<WeaponName>/` split.
- [ ] Play the stock demo map to confirm the install works before changing anything.
- [ ] Download the weapon's Blender source ZIP now if custom weapon work is planned later.
- [ ] Create/populate a `BP_TFA_BaseConfig` instance for your weapon — every field group from Step 2's checklist.

## Phase B — Core actors (Steps 3–5)

- [ ] Set up `BP_TFA_BaseCharacter`: confirm `WeaponConfig`/`CurrentWeaponActor` wiring, `InitialSetup` succeeds, all four camera perspectives work, recoil ramps and decays correctly, `bIsBusy`/`bIsAimingBlocked` gate actions correctly.
- [ ] Set up `BP_TFA_BaseWeapon`: confirm construction-script mesh assembly, both magazine actors spawn correctly (reserve hidden), grip/laser only spawn when their sockets exist.
- [ ] Set up the supporting actors: `BP_TFA_BaseMagazine` (bullet-socket scan), `BP_TFA_BasePhysicsObject`-derived `PhysicsMagazine`/`PhysicsCasing`, `BP_TFA_Attachment_Laser`.

## Phase C — Animation Blueprints (Steps 6–7)

- [ ] Build/verify `ABP_TFA_FP_BaseCharacter`: dependencies satisfied (owner class, `BPI_TFA_AnimationState`, IK bones present), locomotion base pose, mesh-space additive stack, FABRIK hand IK, camera/head toggle, grip overlays — in that evaluation order.
- [ ] Build/verify `ABP_TFA_TP_BaseCharacter`: same dependency checks, stance blend, `SM_AimingTransitions` state machine, breathing additive, montage slots, sequential FABRIK.
- [ ] **Decide your third-person locomotion strategy now** — this pack provides none; see Phase F below.

## Phase D — Notify glue (Step 8)

- [ ] Implement all 5 one-shot notifies and all 4 notify states.
- [ ] Confirm both AnimBPs implement `BPI_TFA_AnimationState`.
- [ ] Place weapon-owned notifies (`AN_TFA_DropMagazine`, `AN_TFA_EjectCasing`, `ANS_TFA_HideMainMag`, `ANS_TFA_ShowReserveMag`) only on weapon-mesh montages; place character-owned notifies (`AN_TFA_SpawnObjectAttached`, `AN_TFA_ThrowPhysicsObject`, `AN_TFA_UnlockActions`, `ANS_TFA_BlockADS`, `ANS_TFA_LeftHandGrip`) only on character-mesh montages.
- [ ] PIE-test a complete reload cycle end to end.

## Phase E — Content population (Step 9)

- [ ] Populate every relevant field on `BP_TFA_BaseConfig` against the full 123-asset catalog.
- [ ] Confirm FP animations (locomotion, combat, poses/transitions) are fully assigned — this is the most content-dense category and the one most setups lean on hardest.

## Phase F — Fill the gaps the pack deliberately leaves open

These are not optional oversights to report as bugs — they're documented, intentional scope boundaries (see [13_FAQ_Scope_And_Constraints.md](13_FAQ_Scope_And_Constraints.md)):

- [ ] **Third-person locomotion:** source real movement animations from outside this pack (a separate mocap/locomotion pack, or hand-authored content) and build your own locomotion state machine for the TP AnimBP's lower body, fused with this pack's upper-body content via the same kind of Layered Blend Per Bone approach already used elsewhere in the pack.
- [ ] **Multiplayer/replication:** design your own replication strategy for every piece of state this guide covers — none of it is replicated in the demo, and the docs offer no guidance on how to add it.
- [ ] **Reload gameplay logic:** build your own ammo/magazine gameplay system; the demo's `AmmoCount` is cosmetic only.
- [ ] **Root motion:** if your movement system expects it, note that none of this pack's content provides it — plan around in-place animation plus your own `CharacterMovementComponent`-driven movement.

## Phase G — Customize (Steps 10–12, as needed)

- [ ] If bringing in a custom character model: follow Guide A in [10_Custom_Character_And_Weapon_Import.md](10_Custom_Character_And_Weapon_Import.md).
- [ ] If bringing in a custom weapon model: follow Guide B in the same file, end to end, including all four minimum sockets.
- [ ] If left-hand clipping appears after a weapon swap: follow [11_LeftHand_Clipping_Fix.md](11_LeftHand_Clipping_Fix.md) — edit `ik_hand_l`, never `hand_l`.
- [ ] Once ready to build real gameplay systems on top: follow [12_Cleanup_Remove_Demo_Logic.md](12_Cleanup_Remove_Demo_Logic.md), in its exact order (preserve `Common/Characters`/`Materials`/`Audio`/`AudioClasses` **before** deleting anything else in `Common/`).

## Phase H — Final verification

- [ ] Every camera perspective (FP/TP/gun-cam/bodycam) works.
- [ ] Fire/reload/inspect/mag-check/fire-mode-switch all play correctly in both FP and TP.
- [ ] Grip variants (none/vertical/angled) correctly swap both mesh and pose.
- [ ] Left-hand grip detaches/reattaches cleanly during reloads with no visible pop.
- [ ] Magazine drop, casing eject, and laser attachment (if applicable) all function.
- [ ] `bIsBusy`/`bIsAimingBlocked` correctly gate and release on every montage path, including interrupted ones.
- [ ] A documented decision exists for each Phase F item — even "not doing this yet" is a valid decision, as long as it's a deliberate one and not an oversight.
