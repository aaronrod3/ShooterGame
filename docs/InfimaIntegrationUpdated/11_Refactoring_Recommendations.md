# Refactoring Recommendations — Industry Standards, Lyra, and Corrected AnimBP Scope

> Compiled 2026-07-12 from a live MCP comparison of ShooterGame's `ABP_FP_Default`/`ABP_TP_Default` against Infima's actual reference Blueprints (not just their prose docs), against Epic's Lyra sample project's animation architecture, and against direct source verification of what movement/combat systems actually exist today. Several findings here **correct assumptions made in files 00, 06, and 07 of this same folder** — this document is the record of what changed and why; the corrected files point back here.

---

## The core lesson this pass taught: fewer states isn't automatically a gap

The prior pass in this folder treated `ABP_TP_Default`'s reduced state count (`SM_TP_Locomotion` at 1 of a previously-existing 3 states, `SM_TP_UpperBody` at 2 of a previously-existing 3) as unfinished work, based on stale framing carried over from `infima_integration_plan.md` and project memory. **It wasn't a gap — the extra states were deliberately removed because they weren't needed.** This matters beyond just this one correction: any future comparison against an old snapshot (memory, prior docs, a backup asset) needs to check *why* something looks smaller before assuming it needs to grow back. The rest of this document reflects the corrected model.

---

## Live comparison findings (MCP-verified, 2026-07-11/12)

| Asset | Structure found | Implication |
|---|---|---|
| `ABP_FP_Default` → `SM_FP_Locomotion` | 3 states (Standing/Crouching/Aiming), 6 transitions — complete | No action needed |
| Infima's real `ABP_TFA_FP_BaseCharacter` | **Six** cascaded state machines: `SM_LocomotionStanding` (Stand/Running/Sprinting), `SM_LocomotionAimed`, `SM_LocomotionCrouched`, plus three *dedicated transition-only* machines (`SM_RunningTransitions`, `SM_WalkingTransitions`, `SM_CrouchingTransitions`), plus top-level `TacSprintStateEntry`/`TacSprintStateExit` | ShooterGame's single-state-machine FP is a legitimate simplification of a far more baroque reference, not something lagging behind it. Do not replicate Infima's complexity for its own sake |
| Infima's real `ABP_TFA_TP_BaseCharacter` | Exactly **one** state machine, `SM_AimingTransitions` (Default/Aim Start/Aim End, using real `AnimationTransitionGraph` rule graphs). No TP locomotion state machine exists in Infima's own reference at all | Confirms architecturally, not just from FAQ prose, that there is no Infima reference to copy for TP locomotion |
| `ABP_TP_Default` outer AnimGraph | Grip `BlendListByEnum` ×2, `LayeredBoneBlend` ×2, sequential FABRIK ×2, breathing `ApplyAdditive`, recoil `ModifyBone`, mesh-space `SM_AimingTranstitions` additive — all present, confirmed via `find_nodes` | Matches what the prior plan assumed; no surprises here |
| `AnimGraphNode_Slot_0`/`Slot_3` | `slotName` = `"DefaultSlot"` and `"Aiming"` respectively | Confirms montages (reload/inspect/melee/heal/etc.) play through these two generic slots regardless of which `SM_TP_UpperBody` state is active — action variety is a montage concern, not a state-machine concern |
| `ABP_TP_Default_Old` | Was a legacy reference copy the user kept while making fixes (confirmed by the user directly, not inferred). Deleted 2026-07-12 after confirming `ABP_TP_Default` compiles clean without it (`compile_blueprint` produced zero warnings/errors) | No longer exists; do not reference it in any future planning |

### Hard MCP limitations — re-tested empirically this session, both still confirmed
- `find_node_types` against a state's inner graph (`.../SM_TP_UpperBody.AnimStateNode_1.CombatReady`) fails with `Cannot cast type 'AnimStateNode' to 'Blueprint'` — state machine internals (new states, new transitions) still cannot be created via MCP. Same failure mode as this project's prior documented history; nothing has changed.
- `add_node_pin` against the existing grip `BlendListByEnum` node fails with `Node "Blend Poses (EWeaponGrip)" does not support adding pins` — adding the `GripVertical`/`GripAngled` branches beyond `None` still requires manual in-editor work.
- **Practical consequence:** any new state machine states/transitions, and the remaining grip cascade branches, must be added by hand in the editor. MCP can verify, wire the *outer* graph around them, assign content to *existing* pose-player nodes, and compile/save — but cannot create the internals themselves.

---

## Corrected: FABRIK vs. Control Rig

My earlier framing ("Control Rig is the modern replacement for FABRIK") was wrong on the merits, not just imprecise — these solve different problems:

- **FABRIK is an IK *solver algorithm*** — a specific AnimGraph node that iteratively bends a bone chain (e.g., clavicle→upper arm→forearm→hand) to reach a target transform. That's the entire scope of the problem it solves.
- **Control Rig is a rigging and procedural-animation *framework*** — its own asset type with its own graph editor, used for building custom control hierarchies, space-switching, secondary/corrective motion, retargeting, and cinematic rig authoring. It happens to *also* ship its own IK nodes (which could solve the same point-to-target problem FABRIK does), but that's a small slice of what it exists for — the bulk of its value is in *overall bone placement and procedural rig behavior* for animation authoring, as opposed to solving one runtime reach-target problem.
- **Corrected recommendation:** keep FABRIK. It is correctly scoped for "pin two hands to two weapon-grip sockets" — that is precisely the class of problem it's built for, and swapping it for Control Rig would add a new asset type, a new editor workflow, and compile/evaluation overhead for no behavioral gain. Control Rig becomes worth adopting only if the project later wants something FABRIK categorically can't do — e.g., secondary physics-like weapon-sway motion, multi-constraint procedural rigging beyond a simple 2-bone chain, or a desire to visually tune IK against a live rig in the viewport. None of that is in scope today. Treat this as **not applicable right now**, not as a deferred "future" item — it isn't the natural next step, it's a different tool for a different job that hasn't come up yet.

---

## Lyra-derived patterns, deepened

Two distinct Lyra patterns are relevant here, addressing two different problems.

### 1. Lower-body-movement-only / upper-body-everything-else (addresses the TP locomotion design question directly)

Lyra's full-body pose is built from a base locomotion layer (speed/direction-driven idle/walk/run, frequently combined with **Orientation Warping** — a real, shipped UE5 AnimGraph node since 5.1, not a Lyra-only custom thing — so the legs can move in the character's actual movement direction while the torso/aim tracks a different direction) fused with an upper-body/aim layer via Layered Blend Per Bone.

**This is structurally the same thing ShooterGame's `LayeredBoneBlend_1` (branch filter `spine_01`, depth `-1`) already does.** The architecture is already Lyra-aligned at the top level — the correction needed isn't to the fusion mechanism, it's to what the locomotion layer's *states* are allowed to encode:

- **Confirmed via source (`CombatComponent.cpp`/`ShooterGameCharacter.cpp`), not assumed:** sprint has no distinct animation or speed tier of its own. `StartSprinting()`/`StopSprinting()` just set `GetCharacterMovement()->MaxWalkSpeed` between `WalkSpeed` and `RunSpeed` directly — there's an explicit comment confirming a third tier (`SprintSpeed`) is declared but deliberately unused: *"kept declared in case a future third tier is wanted."* Sprinting is gated by a real stamina system (`CurrentStamina`/`SprintDrainRate`/`SprintRecoveryRate`/`bCanSprint`/`SprintMinStaminaToStart`) that auto-stops sprint on depletion, and sprint is blocked entirely while aiming (`if (Combat && Combat->bAiming) return;` in `StartSprinting()`).
  - **This confirms, from source rather than just the user's description, that `SM_TP_Locomotion` needs no discrete Sprint state** — it's a continuous speed increase within a single Idle/Walk/Run blend space, exactly as already designed.
- **Two pieces of behavior the user described are not yet implemented anywhere in `Source/`** (confirmed by grep — zero matches for "Sprint" in `CombatComponent.cpp`, zero matches for "Sway" anywhere in `Source/`):
  1. **Firing should cancel sprint back to walk speed.** `HandleFire()` currently has no interaction with `bIsSprinting`/`StopSprinting()` at all. This is real, small, well-scoped gameplay work — not an animation concern — likely a one-line addition near the top of `HandleFire()` or `FireButtonPressed()`.
  2. **Weapon sway should increase with speed/while sprinting.** No sway system of any kind exists yet. The natural, in-pattern way to add this: a `SprintSwayTransform` (or similarly named) procedural `FTransform`, computed and spring-interpolated exactly like the existing `CrouchTransform`/`AimDownSightsTransform`/`RecoilTransform` triplet in `UShooterAnimInstanceBase`, scaled by current speed or `bIsSprinting`. This reuses an established pattern rather than inventing a new one.
  - **Neither of these blocks TP locomotion animation work** — they're independent, additive gameplay-layer tasks. Flagging them here since they surfaced directly from verifying the user's design description against source, and they're cheap, well-defined, and worth doing (or filing as issues) regardless of AnimBP sequencing.
- **Recommendation for `SM_TP_Locomotion`:** a single state, Idle/Walk/Run continuous blend space (rename `Unarmed_IdleWalkRun` once the armed/unarmed split is confirmed dropped from this layer — the name is now misleading, not the structure). No armed/unarmed split, no discrete Sprint state. All weapon-holding/combat visual difference belongs entirely to `SM_TP_UpperBody` + the branch-filtered layer.
- **Recommendation for `SM_TP_UpperBody`:** the existing 2 states (`CombatReady`, `ADS`) are very likely already complete for current scope, not missing a third. Confirmed via the `Slot_0`/`Slot_3` check above — reload, inspect, melee, heal, grenade-throw, and every other action already play as montages through the `DefaultSlot`/`Aiming` slots on top of whichever of these two base states is active, so they don't need their own state-machine states. `Relaxed` (a fully-lowered, non-combat-ready idle) would only be needed if the game ever shows a moment where the character is armed but explicitly not combat-ready — for a wave-shooter where the player is effectively always combat-ready once equipped, this is very likely correctly excluded from current scope, matching the same reasoning already applied to locomotion.
- **Orientation Warping** is worth evaluating directly if/when the character needs to strafe or aim off-axis from its facing while moving — the concrete mechanism that makes "legs go one way, torso/aim goes another" look right without a huge locomotion state matrix. Relevant to the existing `AimOffset_Yaw`/`AimOffset_Pitch` + `TurningInPlace` system already in `UShooterAnimInstanceBase`. Not urgent, but the natural next locomotion-quality improvement once base content exists.

### 2. Linked Anim Layers / Anim Layer Interface (addresses future multi-weapon scaling — not urgent now)

Lyra's per-weapon animation behavior is driven by an **Anim Layer Interface**: the base character ABP declares named layer entry points as Blueprint interface functions, and each equippable item provides its own small Animation Blueprint implementing that interface. Item data (Lyra's Equipment Fragments) carries a reference to which Anim Layer class to link in; the base ABP calls `Link Anim Class Layers` at equip-time. This is a strict superset of ShooterGame's current `BlendListByEnum`-based grip switching and monolithic FP/TP graphs.

- **Direct mapping onto ShooterGame:** `UWeaponConfig` (already a `UPrimaryDataAsset`, already the "one data asset per weapon, no C++ changes" mechanism per `CLAUDE.md`) could carry a `TSubclassOf<UAnimInstance> AnimLayerClass` field. A new weapon = a new small linked-layer Blueprint plus a `WeaponConfig` entry, instead of new branches inside the monolithic FP/TP graphs.
- **Recommendation, unchanged from the first pass:** document as the next major refactor once the current single-weapon FP/TP graphs are content-complete and stable. Migrating to Linked Anim Layers before the base graphs even work correctly solves a scaling problem (multiple weapons) before the correctness problem (one weapon working right) is done — sequencing matters here.

---

## Addendum (same session, later): content assignment is further along than assumed above

The table above and the rest of this document were written assuming TP locomotion and upper-body states still needed real animation content assigned. Direct inspection immediately afterward showed otherwise: `Unarmed_IdleWalkRun`'s blend space (`BS_UnequippedIdleWalkRun`) already has 18 real directional samples correctly wired to `GetDirection()`/`GetSpeed()`; `CombatReady` and `ADS` both already have real pose/aim-offset content assigned too. **The architectural recommendations above (state design, Lyra patterns, FABRIK reasoning) are unaffected and still stand — only the "what's left to build" framing changes.** See the corrected [07_ThirdPerson_Locomotion_Sourcing.md](07_ThirdPerson_Locomotion_Sourcing.md) and [06_AnimBP_Core_Systems_CrossReference.md](06_AnimBP_Core_Systems_CrossReference.md) for the accurate current state. The one area that still needs a real look is the grip `BlendListByEnum` cascade, where a live-editor visual check is needed since reflection-based inspection gave an ambiguous read.

## What this means for the rest of this folder

- `00_Overview.md`'s systems table entry for TP locomotion content is corrected to reflect intentional scope, not a content gap (still true: Infima has zero TP locomotion reference architecture if/when this ever needs to grow).
- `07_ThirdPerson_Locomotion_Sourcing.md` is substantially revised — see that file directly; the short version is "source real blend-space content for the one state that exists, don't add states."
- `06_AnimBP_Core_Systems_CrossReference.md` is updated with the live-verified completeness table and the corrected FABRIK reasoning.
- The fire-cancels-sprint and sway-scales-with-speed findings are new, small, independent gameplay tasks — worth their own tracking (GitHub issue per `CLAUDE.md`'s workflow) rather than folding silently into animation work.
