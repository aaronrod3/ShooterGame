# Phase 6 — AnimBP Core Systems (Cross-Reference)

**This phase is deliberately thin.** `docs/infima_integration_plan.md` Phases 1–4 are the actively-maintained, session-by-session tracker for `ABP_FP_Default`/`ABP_TP_Default` AnimGraph work (FABRIK rewiring, state machine construction, grip cascades, animation content assignment). That document should stay authoritative for status on all of that — this file only records new corroborating detail surfaced by the Guides that's worth folding into future work on those graphs, and one dependency note for Phase 7.

---

## New corroborating detail from the Guides

### `ik_hand_l`/`ik_hand_r` editing convention — reconfirmed independently, twice
Two separate agent passes over the Guides' "How to Fix Left-Hand Clipping" tutorial both independently surfaced the same core rule, stated explicitly in Infima's own docs:

> "You should adjust the IK bone, not the hand bone directly... for left-hand placement, edit `ik_hand_l` and not `hand_l`."

This directly corroborates the root-cause reasoning already recorded for **Bug A** (IK bone-space mismatch / TP arm flailing) in the project's own investigation notes — the official docs independently confirm `ik_hand_l`/`ik_hand_r` are the correct manipulation points, and that conflating them with the live `hand_l`/`hand_r` bones is a known category of mistake in this pack's design, not something unique to this port. **Adopt as a standing authoring rule**: any future custom pose work (idle poses, grip poses, anything hand-placement-related) edits the `ik_hand_*` bones, never `hand_*` directly. Worth a one-line comment in `ShooterAnimInstanceBase.cpp` near `UpdateIKData()` if not already present.

One practical authoring caveat also confirmed: the pose preview does **not** update live while nudging an IK bone in the Skeletal Mesh Editor / pose-creation workflow — expect to iterate blind and re-check, not tune interactively.

### Mesh-space additive authoring checklist
Confirmed from the Animation reference (already known) and reinforced by the Guides' left-hand-grip mechanism explainer. For any new additive animation content added to either AnimBP going forward:
- Additive Type: **Mesh Space** (not Local Space) — local-space additives on this rig produce arm twisting/double-transforms
- Base Pose: Reference Pose, or the same base pose the rest of the pack already uses
- Root Motion: disabled

If a new additive breaks the pose stack, check mesh-space vs. local-space first — Infima's own docs call this out as the most common cause.

### Camera-shake dampening — confirmed as an intentional, documented, tunable feature
Infima's FAQ describes the exact mechanism already present in `ABP_FP_Default`'s AnimGraph (the head/camera `LayeredBoneBlend`, branch filter on `head`, controlled by `bAnimateCamera`):

> "The first-person camera shake animations are baked directly onto the head bone of the character. You can use a Layered Blend Per Bone node to limit how much the head bone moves, use the blend weights to adjust the intensity."

This confirms the existing node's *purpose* — it's not just a blunt on/off toggle for head motion, it's meant to be a tunable intensity dial. Worth revisiting once animation content is fully assigned (post `infima_integration_plan.md` Phase 4) to confirm the blend weight is actually being used for intentional feel-tuning rather than left at a default 0/1 value.

### Bodycam perspective — deprioritize
Infima's own FAQ: *"the third-person animations were not created with a true FPS setup in mind, so the head movements can be pretty motion-sickness-inducing by default... it would require some manual work to get it feeling right."* This is Infima's own admission that the bodycam camera mode (`SocketHelmetCamera`/`SocketChestCamera`) is unpolished. Treat any bodycam-perspective work as a stretch goal, not a system worth investing verification time in until FP/TP/gun-cam are solid.

---

## Dependency note for Phase 7

`SM_TP_Locomotion`'s incomplete state count is **not** an AnimBP wiring bug — see [07_ThirdPerson_Locomotion_Sourcing.md](07_ThirdPerson_Locomotion_Sourcing.md). Don't spend further `infima_integration_plan.md` Phase 4 effort trying to "finish" that state machine by wiring existing content; there is no existing third-person locomotion content in the pack to wire. Source it first (Phase 7), then return to state-machine construction.

---

## Checklist

- [ ] Add the `ik_hand_*`-not-`hand_*` rule as a durable comment/convention note near `UShooterAnimInstanceBase::UpdateIKData()` if not already documented there.
- [ ] When authoring any new additive animation content, apply the mesh-space checklist above before importing.
- [ ] Confirm the head-bone `LayeredBoneBlend`'s blend weight is being used as an intentional camera-shake dial, not left at a default value, once real animation content is assigned.
- [ ] Continue all granular AnimGraph status tracking in `infima_integration_plan.md` — do not duplicate it here.

## Exit criteria

N/A as a standalone phase — this file's content should be folded into authoring practice as `infima_integration_plan.md`'s own phases complete, and revisited only if new Guides-derived detail surfaces later.
