# Phase 6 — AnimBP Core Systems (Cross-Reference)

> **Updated 2026-07-12** with live MCP-verified findings (superseding the original "defer entirely" framing — this pass has harder, direct data that `infima_integration_plan.md` didn't have) and corrected reasoning on FABRIK/Control Rig. See [11_Refactoring_Recommendations.md](11_Refactoring_Recommendations.md) for the full detail behind everything here.

`docs/infima_integration_plan.md` Phases 1–4 remain the actively-maintained tracker for granular AnimGraph bug-fixing (FABRIK rewiring, animation content assignment, individual node-level status). This file adds corrected architectural context on top of that, not a competing checklist.

---

## Live-verified state machine completeness (2026-07-11/12, via `list_graphs`/`find_nodes`)

| State machine | States | Status | Notes |
|---|---|---|---|
| `ABP_FP_Default → SM_FP_Locomotion` | Standing, Crouching, Aiming (3/3), 6 transitions | Complete | No action needed |
| `ABP_TP_Default → SM_TP_Locomotion` | `Unarmed_IdleWalkRun` (1 state, by design) | **Intentional single-state design, content-complete** — see [07](07_ThirdPerson_Locomotion_Sourcing.md) | `BS_UnequippedIdleWalkRun` already has 18 real samples, correctly wired to `GetDirection()`/`GetSpeed()`. No additional states, no additional content needed — only a cosmetic rename is outstanding |
| `ABP_TP_Default → SM_TP_UpperBody` | `CombatReady`, `ADS` (2 states, by design) | **Very likely already complete for current scope** | Confirmed `AnimGraphNode_Slot_0`/`Slot_3` (`slotName` = `DefaultSlot`/`Aiming`) carry all montage-driven actions (reload/inspect/melee/heal/grenade/etc.) on top of whichever of these two states is active — action variety doesn't need its own states |
| `ABP_TP_Default → SM_AimingTranstitions` | Default, AimStart, AimEnd (3/3) | Complete | (Typo in the live asset name — `Transtitions` not `Transitions` — is baked in, not worth renaming on its own) |

### Infima's actual reference architecture (for comparison only — not a target to replicate)
- `ABP_TFA_FP_BaseCharacter`: **six** cascaded state machines (`SM_LocomotionStanding`, `SM_LocomotionAimed`, `SM_LocomotionCrouched`, plus dedicated transition-only machines `SM_RunningTransitions`/`SM_WalkingTransitions`/`SM_CrouchingTransitions`), plus `TacSprintStateEntry`/`TacSprintStateExit`. Far more complex than its own documentation describes, and far more complex than ShooterGame's single-state-machine FP design — the simplification here is sound, not a shortfall.
- `ABP_TFA_TP_BaseCharacter`: exactly one state machine, `SM_AimingTransitions`. No TP locomotion architecture exists in Infima's own reference at all.

## Content assignment is further along than any prior doc assumed (checked 2026-07-12)

Spot-checked actual pose-player content on both AnimBPs — this was assumed to be a significant remaining gap; it mostly isn't:

- `ABP_TP_Default → SM_TP_Locomotion → Unarmed_IdleWalkRun`'s `BlendSpacePlayer` → `BS_UnequippedIdleWalkRun`, which already has **18 real samples** (idle + walk + run across 8 directions), inputs wired to `GetDirection()`/`GetSpeed()`. See [07](07_ThirdPerson_Locomotion_Sourcing.md).
- `ABP_TP_Default → SM_TP_UpperBody → CombatReady`'s `SequencePlayer` → `AS_TP_AR_Idle_Pose` (real, project-owned asset).
- `ABP_TP_Default → SM_TP_UpperBody → ADS`'s `RotationOffsetBlendSpace` (aim offset) → `AO_ADS`, `alpha = 1`, fed by `Get AimOffset_Yaw`/`Get AimOffset_Pitch`.
- `ABP_FP_Default → SM_FP_Locomotion → Standing`'s `BlendSpacePlayer` → `BS_FP_AR_Locomotion_Standing` (spot-checked as a sanity comparison — FP is at least as far along).

**Lesson for future sessions: verify content-assignment status live before assuming it's a gap.** Both this project's own `infima_integration_plan.md` and the first draft of this Infima adoption plan assumed significant unassigned content remained; direct inspection shows the opposite for everything checked so far.

## Grip cascade — needs a fresh live-editor visual check, don't trust either old doc or this reflection alone

`infima_integration_plan.md` Phase 4 says only the `None` branch is wired on the grip `BlendListByEnum` nodes, framing `GripVertical`/`GripAngled` as needing new pins added. Live property inspection this session shows something in between: the node's `blendPose` array **already has 3 slots** (matching `EWeaponGrip`'s None/GripVertical/GripAngled), but `enumToPoseIndex` is empty and all 3 slots report `linkId: -1` (unlinked) in this reflection-based read. This doesn't cleanly confirm either the old doc's framing or a fully-wired state — **check this by eye in the AnimGraph editor before doing any more work on it**, since a generic property reflection may not accurately represent pin connectivity for this node type (the same caveat that applies to `AnimStateNode` inspection may partly apply here too).

## Hard MCP limitations — re-tested empirically 2026-07-12, both still confirmed exactly as documented

- `find_node_types` against a state's inner graph fails: `Cannot cast type 'AnimStateNode' to 'Blueprint'`. State machine internals (new states/transitions) cannot be created via MCP.
- `add_node_pin` against the grip `BlendListByEnum` fails: `Node "Blend Poses (EWeaponGrip)" does not support adding pins.` — consistent with the node already having 3 slots (nothing to add), though this doesn't confirm they're correctly connected; see above.
- Both limitations are unchanged from this project's prior documented history — nothing has been fixed upstream. Plan around them, don't re-test them again without a specific reason to think something changed.

---

## Corrected detail from the Guides (retained from the prior pass, still valid)

### `ik_hand_l`/`ik_hand_r` editing convention — reconfirmed independently, twice
Two separate research passes over the Guides' "How to Fix Left-Hand Clipping" tutorial both surfaced the same rule, stated explicitly in Infima's own docs: *"you should adjust the IK bone, not the hand bone directly... for left-hand placement, edit `ik_hand_l` and not `hand_l`."* This corroborates the root-cause reasoning already recorded for **Bug A** (IK bone-space mismatch / TP arm flailing). **Adopt as a standing authoring rule** for any future custom pose work. One practical caveat: the pose preview does not update live while nudging an IK bone — expect to iterate blind and re-check.

### Mesh-space additive authoring checklist
For any new additive animation content: Additive Type = **Mesh Space** (not Local Space — local-space additives on this rig produce arm twisting/double-transforms), Base Pose = Reference Pose, Root Motion = disabled.

### Camera-shake dampening — confirmed intentional and tunable
Infima's FAQ describes the exact mechanism already present in `ABP_FP_Default`'s AnimGraph (head/camera `LayeredBoneBlend`, branch filter on `head`, controlled by `bAnimateCamera`): *"You can use a Layered Blend Per Bone node to limit how much the head bone moves, use the blend weights to adjust the intensity."* Confirms the node's *purpose* is a tunable intensity dial, not a blunt on/off toggle — worth revisiting once animation content is fully assigned to confirm the blend weight is being used intentionally.

### Bodycam perspective — deprioritize
Infima's own FAQ admits this mode is unpolished (*"motion-sickness-inducing by default... would require some manual work to get it feeling right"*). Treat as a stretch goal.

---

## Corrected: FABRIK vs. Control Rig

The original framing here ("Control Rig is a modern replacement for FABRIK") was wrong on the merits. They solve different problems:

- **FABRIK** is an IK *solver algorithm* — an AnimGraph node that iteratively bends a bone chain to reach a target. That's the whole scope of what it does, and it's the right tool for "pin a hand to a weapon grip socket."
- **Control Rig** is a rigging/procedural-animation *framework* (its own asset type, its own graph editor) for building custom control hierarchies, space-switching, secondary motion, retargeting, cinematic rigging. It has its own IK nodes and could solve the same point-to-target problem, but that's a small slice of its actual purpose — the bulk of its value is in overall bone placement and procedural rig authoring, not runtime reach-target solving.
- **Keep FABRIK.** It's correctly scoped, not a shortcoming. Control Rig is worth adopting only if the project wants something FABRIK genuinely can't do (secondary weapon-sway physics, complex multi-constraint procedural motion, visual IK tuning against a live rig) — none of which is in scope today. This isn't a deferred future item so much as a different tool for a problem that hasn't come up yet.

## Lyra alignment note

Lyra fuses a base locomotion layer with an upper-body/aim layer via Layered Blend Per Bone — structurally the same thing `LayeredBoneBlend_1` (`spine_01`, depth `-1`) already does here. The architecture is already aligned with the industry-standard reference; see [07](07_ThirdPerson_Locomotion_Sourcing.md) and [11](11_Refactoring_Recommendations.md) for what that implies about which layer should own which states, and for **Orientation Warping** as a concrete follow-up technique once locomotion content exists.

---

## Checklist

- [ ] Continue all granular AnimGraph status tracking in `infima_integration_plan.md` — do not duplicate it here.
- [ ] Add the `ik_hand_*`-not-`hand_*` rule as a durable comment/convention note near `UShooterAnimInstanceBase::UpdateIKData()` if not already documented there.
- [ ] Apply the mesh-space additive checklist to any new animation content.
- [ ] Do not attempt to re-solve the grip `BlendListByEnum` branch or state-machine-internals limitations via MCP without a specific reason to believe the tooling has changed — both were just re-confirmed.

## Exit criteria

N/A as a standalone phase — fold into authoring practice as `infima_integration_plan.md`'s own phases complete.
