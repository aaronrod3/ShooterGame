# Phase 7 — Third-Person Locomotion Content Sourcing

> **Revised 2026-07-12.** The first version of this file treated `SM_TP_Locomotion`'s reduced state count as an unfinished-wiring gap. It wasn't — the extra states (`Armed_Walk`, `Sprint`) were deliberately removed because they weren't needed, confirmed directly by the user. This revision keeps the one finding that's still true and useful (Infima ships no TP locomotion content at all) and drops everything that assumed the old 3-state layout needed restoring. See [11_Refactoring_Recommendations.md](11_Refactoring_Recommendations.md) for the full corrected reasoning.

---

## What's still true: Infima has no reference architecture for this, at all

Confirmed twice now — once from the pack's own FAQ ("there are no third-person movement (locomotion) animations included, those are only included for first-person... I don't feel confident in producing full-body movement animations"), and again this session by live-inspecting Infima's actual `ABP_TFA_TP_BaseCharacter`, which has exactly **one** state machine (`SM_AimingTransitions`) and no locomotion state machine of any kind. This isn't just missing content — Infima's own reference implementation never attempted the architecture either. If this system ever needs to grow, there's still no Infima pattern to copy; the design has to come from elsewhere (this file recommends Lyra's pattern below, already partially in place).

## What's corrected: the target design is simpler than previously assumed, not the same design with content filled in

Verified directly against source this session (`CombatComponent.cpp`, `ShooterGameCharacter.cpp`):

- **No armed/unarmed split belongs in the locomotion layer.** Weapon-holding is an upper-body concern, handled entirely by `SM_TP_UpperBody` and the branch-filtered `LayeredBoneBlend_1` (`spine_01`, depth `-1`) that fuses it with locomotion. A person's gait doesn't fundamentally change from carrying a rifle at low-ready. The old `Armed_Walk` state was very likely solving a problem that belongs one layer up.
- **No discrete Sprint state.** Confirmed from `AShooterGameCharacter::StartSprinting()`/`StopSprinting()`: sprint is `GetCharacterMovement()->MaxWalkSpeed` toggling between `WalkSpeed` and `RunSpeed` — there's no separate sprint speed tier (a comment in the movement-speed-selection code explicitly notes a third tier is declared but unused, "kept declared in case a future third tier is wanted"). Sprint is gated by a real stamina system and is blocked entirely while aiming. This is a continuous speed change within a single blend space, not a distinct animation state.
- **Target design:** `SM_TP_Locomotion` = one state, a single Idle/Walk/Run blend space driven by continuous speed. Rename `Unarmed_IdleWalkRun` once the armed/unarmed split is confirmed unnecessary — the name describes a distinction the state no longer needs to make.

## Correction #2 (2026-07-12, same session): the content-sourcing task is already done

Live-inspected the actual `Unarmed_IdleWalkRun` state's `BlendSpacePlayer` node: it already plays `/Game/Animation/BlendSpace/BS_UnequippedIdleWalkRun`, which already has **18 real animation samples** (`St_Idle_00`, plus `St_Walk_*`/`St_Run_*` across 8 directions each, sourced from `/Game/Animation/Folder2/Unequipped/` — not Infima content, so this was sourced from elsewhere exactly as this file originally recommended). Its two inputs are wired to `GetDirection()`/`GetSpeed()` — reading directly off `UShooterAnimInstanceBase::Direction`/`Speed`, the same native-C++-drives-the-graph pattern used everywhere else. The blend space's Speed axis (`0–350`) comfortably covers `RunSpeed` (150) with headroom for the declared-but-currently-unused `SprintSpeed` (250) tier.

**This means Phase 7's original task — source real TP locomotion content — is already complete.** Someone (this project, prior to this documentation existing) already did exactly what this file recommended: found real directional locomotion animations from outside the Infima pack and wired them in correctly. The earlier framing in this file (both the original "gap to fill" version and even this revision's first pass, which still assumed content-sourcing was outstanding) was wrong on this point specifically — the state-count reduction was the only thing that needed correcting last time; this time, the content-completeness assumption needed correcting too.

## What's actually left (much smaller than either previous version of this file assumed)

1. **Cosmetic rename, low priority.** `Unarmed_IdleWalkRun` (the state) and `BS_UnequippedIdleWalkRun` (the blend space asset) both carry "unarmed"/"unequipped" naming from when an armed/unarmed split existed at this layer. Since this content now correctly serves all movement regardless of weapon state, the names are misleading but not functionally wrong — rename when convenient, not urgent.
2. **The two independent gameplay tasks below** (fire-cancels-sprint, sway-scales-with-speed) — still genuinely unbuilt, confirmed by grep.
3. **Nothing else identified.** The upper-body fusion (`LayeredBoneBlend_1`) was already confirmed correct in a prior session; this session confirmed the locomotion content and wiring underneath it are also correct. Don't assume there's a bigger content-sourcing task here than there is — verify against live state again before adding scope.

## Two small, independent gameplay tasks this investigation surfaced (not blocking, not animation work)

Confirmed absent from `Source/` entirely (zero grep matches):

1. **Firing should cancel sprint back to walk speed** — described as intended design, not yet wired. `HandleFire()` currently has no interaction with `bIsSprinting`. Likely a small addition (call the equivalent of `StopSprinting()` when a shot fires).
2. **Weapon sway should increase with speed/while sprinting** — no sway system exists at all yet. Recommended approach: a new procedural `FTransform` (e.g. `SprintSwayTransform`) computed and spring-interpolated exactly like the existing `CrouchTransform`/`AimDownSightsTransform`/`RecoilTransform` triplet in `UShooterAnimInstanceBase`, scaled by speed/`bIsSprinting` — reusing an established pattern rather than inventing a new mechanism.

Neither blocks the locomotion content-sourcing work above; both are worth their own tracking (GitHub issue per `CLAUDE.md`'s workflow) since they're gameplay-layer, not animation-layer.

## Integration notes (confirmed already satisfied, listed for future reference)

- Input convention already matches the rest of the project (`Direction`/`Speed` read straight off `UShooterAnimInstanceBase`, same native-drives-the-graph pattern used everywhere else) — confirmed, no action needed.
- Confirmed layered correctly under `SM_TP_UpperBody` via the existing `LayeredBoneBlend_1` fusion — unaffected by any of this, already correct per prior session's verification.
- Root motion: not independently re-verified this pass, but no indication of a problem; spot-check if locomotion ever looks like it's fighting `CharacterMovementComponent`.
- **Orientation Warping** (a real UE5 5.1+ AnimGraph node) remains worth evaluating once PIE testing shows whether strafing/off-axis aim while moving needs it — a polish item, not a blocker, since base content already exists and works.

---

## Checklist

- [x] Confirm the target design (single-state Idle/Walk/Run/Sprint-as-speed, no armed split) — confirmed by the user directly.
- [x] Confirm real blend-space content exists and is correctly wired — confirmed via live MCP inspection 2026-07-12 (`BS_UnequippedIdleWalkRun`, 18 samples, `GetDirection()`/`GetSpeed()` wired correctly).
- [ ] Rename `Unarmed_IdleWalkRun`/`BS_UnequippedIdleWalkRun` once convenient — cosmetic, not urgent.
- [ ] Build fire-cancels-sprint-speed in `CombatComponent`/`ShooterGameCharacter` (confirmed not yet implemented).
- [ ] Build a `SprintSwayTransform`-style procedural offset scaled by speed/`bIsSprinting`, following the existing `CrouchTransform`/`AimDownSightsTransform`/`RecoilTransform` pattern (confirmed no sway system exists yet).
- [ ] File the above two as separate, small tracked issues per `CLAUDE.md`'s workflow — they're real but independent of animation work.
- [ ] PIE-test locomotion directly (visual judgment call only the user/a live session can make) — everything checked so far is structural/data correctness, not "does it look right."
- [ ] Evaluate Orientation Warping after PIE testing, only if strafe/off-axis aim while moving needs it.

## Exit criteria

- ~~`SM_TP_Locomotion`'s single state has real, playable blend-space content spanning idle through sprint speed continuously.~~ **Already true, confirmed 2026-07-12.**
- A player character viewed in third person visibly walks/runs/sprints with correct lower-body motion while the existing upper-body/aim layer continues to blend correctly on top — needs a PIE pass to confirm it *looks* right, since this file can only confirm the data/wiring is correct, not the visual result.
- Fire-cancels-sprint and sway-scale-with-speed implemented and verified in PIE.
