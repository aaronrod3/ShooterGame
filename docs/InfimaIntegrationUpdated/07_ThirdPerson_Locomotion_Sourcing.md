# Phase 7 — Third-Person Locomotion Content Sourcing

**This is the single most important finding in this entire plan.** It reframes an existing, previously-unexplained gap rather than introducing new scope.

---

## The finding

Project memory and `infima_integration_plan.md` both record that `ABP_TP_Default → SM_TP_Locomotion` is incomplete — only 1 of 3 states exist (`Unarmed_IdleWalkRun`; `Armed_Walk` and `Sprint` are missing), framed as unfinished wiring work.

**It isn't unfinished wiring. There is no source content for it, by Infima's own deliberate choice.** Straight from the official FAQ:

> "there are no third-person movement (locomotion) animations included, those are only included for first-person."

And the author's own stated reasoning:

> "I am mainly focusing on high-quality upper-body movement animations because I don't feel confident in producing full-body movement animations. There are plenty of high-quality full-body movement packs available which can be integrated for the best results."

Infima's recommended workflow, stated explicitly: **"use the third-person upper-body animations in combination with mocap (or similar) third-person locomotion data on the lower body."** This is exactly the layered architecture ShooterGame's TP AnimBP already uses (`LayeredBoneBlend_1` fusing `SM_TP_Locomotion` with `SM_TP_UpperBody`, branch filter `spine_01`, `blendDepth: -1` — already confirmed correct via live MCP inspection, see `project_shootergame_animation_state` memory) — the architecture is right, it's just missing the lower-body input it was always designed to receive from somewhere else.

**No amount of `infima_integration_plan.md` Phase 4 effort fixes this.** Building the missing `Armed_Walk`/`Sprint` states requires real animation *content* (blend spaces with actual walk/run/sprint clips), which the Infima pack never shipped and was never going to ship.

---

## What this means for sequencing

Treat this as its own content-acquisition task, separate from (and blocking) the remaining `SM_TP_Locomotion` state-machine work in `infima_integration_plan.md` Phase 4. Don't mark that phase's TP locomotion sub-item "blocked/investigating" indefinitely — it's blocked on a concrete, actionable decision: **where does the movement content come from?**

## Sourcing options, roughly in order of effort

1. **UE5's own default Manny locomotion animations.** ShooterGame already uses `SK_Mannequin` — UE5 ships a full idle/walk/run blend space and matching animation set for exactly this skeleton, at zero additional cost and zero licensing consideration. This is very likely the fastest path to a working `Armed_Walk` state, even if the visual fidelity is a step down from a premium mocap pack. Worth prototyping first since it costs nothing to try.
2. **A marketplace/Fab mocap locomotion pack**, purchased and retargeted the same way any custom character content would be (see [10_Reference_Custom_Weapon_And_Character_Procedures.md](10_Reference_Custom_Weapon_And_Character_Procedures.md) for the general custom-character retargeting workflow, even though that guide is written for full character meshes rather than animation-only packs — the skeleton-assignment logic is the same idea).
3. **Custom mocap capture or hand-keyed authoring**, the highest-effort option, only worth it once the rest of the game's core loop is proven and locomotion fidelity becomes a differentiator worth the investment.

## Integration notes regardless of source

- Whatever locomotion content is sourced needs a **2D blend space** (`X`/`Y` inputs) matching the existing convention already used elsewhere in the project's AnimBPs (`Get Direction`/`Get Speed` for TP, matching the `X`/`Y` = forward/strafe convention Infima's own FP locomotion already uses) — don't introduce a different input convention just because the content came from a different source.
- The sourced content only needs to drive the **lower body**; the existing `LayeredBoneBlend_1` fusion with `SM_TP_UpperBody` (already correctly configured) will handle blending it with Infima's actual upper-body/aim content. Don't scope this as "replace the whole TP AnimBP" — it's specifically filling in the locomotion state machine's missing states.
- Root motion: confirm whether the sourced content uses root motion or is in-place. Infima's own content is entirely in-place (no root motion anywhere in the pack) — if new locomotion content uses root motion and the rest of the character relies on `CharacterMovementComponent`-driven movement (per `CLAUDE.md`, `BP_TFA_BaseCharacter_vs_BP_PlayerCharacter.md`'s finding that ShooterGame's native movement already replaces Infima's simulated-velocity approach), root motion will likely need to be disabled or extracted, matching the in-place convention already established.

---

## Checklist

- [ ] Decide the sourcing option (default Manny content is the recommended first attempt given it's already zero-cost and skeleton-compatible).
- [ ] Confirm the sourced content's input convention matches the existing `Get Direction`/`Get Speed` (or equivalent) pattern.
- [ ] Confirm root motion is disabled/extracted to match the project's in-place, `CharacterMovementComponent`-driven convention.
- [ ] Build `Armed_Walk` and `Sprint` states in `SM_TP_Locomotion` using the sourced content, following the same transition/blend conventions (`TLT_StandardBlend`, 0.2s) already specified for the existing state.
- [ ] Re-open `infima_integration_plan.md` Phase 4's TP locomotion sub-item once content is sourced, and complete it there — don't track state-machine completion status in two places.

## Exit criteria

- `SM_TP_Locomotion` has all 3 states with real, playable content and correct transitions.
- A player character viewed in third person visibly walks/runs/sprints with matching lower-body motion while the existing upper-body/aim layer continues to blend correctly on top, unchanged.
