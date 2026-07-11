# Phase 9 — Demo Content Migration & Cleanup

**Relationship to existing plan:** this is `docs/infima_integration_plan.md` Phase 6 ("Supporting Actor Migration"), confirmed **NOT STARTED** as of 2026-07-11 (all 4 listed Blueprints still only exist under the Infima plugin/demo folder — no project-owned copies anywhere in `Content/`). This phase distills Infima's own official "How to Safely Remove the Demo Code Logic" guide into the exact steps and ordering that guide recommends, since it turns out to line up closely with what `infima_integration_plan.md` Phase 6 already needs to do.

`CLAUDE.md`'s Off-Limits section already states the destination intent: *"Infima demo/common/core logic — being replaced, not extended."* This phase is how that gets executed safely, without breaking the animations that are staying.

---

## The one warning that matters most: skeleton first

Infima's own guide is emphatic about ordering, and gets this exactly right — read it before touching anything:

> `Common/Characters` "contains the mannequin skeleton that all of the animations are assigned to. **Deleting this folder WILL break all of the included animations.**"

**Do not delete or restructure anything under the Infima `Common/` folder until the skeleton (and the other folders below) have been migrated out to a project-owned location first.** This is the correct order specifically because later steps in this same guide tell you to delete the rest of `Common/` — if the skeleton migration is skipped or done out of order, that later deletion step silently destroys every animation asset already in use.

---

## Step order (per Infima's own guide, adapted to this project's existing Phase 6 asset list)

1. **Move these four `Common/` folders out to project-owned locations FIRST**, before deleting anything:
   - `Common/Characters` — the shared mannequin skeleton (`SKEL_TFA_Mannequin`-equivalent) that every animation in the project is assigned to. **Highest priority, do this first.**
   - `Common/Materials` — the master material used by weapon material instances (`M_TFA_Weapon_Master` in Infima's own convention — check what this project's `MI_*` weapon material instances actually parent to before assuming the name matches).
   - `Common/Audio` — footstep/cloth foley sounds used directly by locomotion animation sequences. Keep if any locomotion content (including whatever gets sourced in Phase 7) references these cues.
   - `Common/AudioClasses` — the master sound class referenced by the included audio cues. Keep only if the included SFX are being kept.
2. **Migrate `infima_integration_plan.md` Phase 6's 4 named Blueprints** using Unreal's Migrate tool (right-click → Asset Actions → Migrate), same as that phase already specifies:
   - `BP_TFA_BaseMagazine` → project-owned copy (e.g. `BP_MagazineCosmetic`) — likely superseded by whatever cosmetic magazine actor Phase 3/4 of *this* plan builds; confirm before migrating whether it's still needed at all, or whether the native replacement makes this migration moot.
   - `BP_TFA_BasePhysicsObject` → only relevant as a *reference* at this point; `ACaseEject` already exceeds it (see [04_Physics_Props_And_Attachments.md](04_Physics_Props_And_Attachments.md)) — likely not worth migrating verbatim, review whether it's needed before doing the migration work.
   - `BP_TFA_PhysicsMagazine` → same caveat as above, likely superseded by the native `ADroppedMagazine`-equivalent from Phase 4.
   - `BP_TFA_PhysicsCasing` → same caveat, already superseded by `ACaseEject`.
   - `BP_TFA_AttachmentLaser` (or whatever the correct current asset name is — `infima_integration_plan.md` notes the name should be confirmed before migrating) → likely not worth migrating if Phase 4's native laser actor is built instead of ported.

   **Given Phases 3–4 of this plan build native replacements for 4 of these 5 assets, re-evaluate whether "migrate the Blueprint" is still the right move versus "build the native version and never migrate the original at all."** Migrating Infima's Blueprint versions only makes sense for content you intend to keep using as-is; where a native rebuild is already planned, migrating the original is wasted effort.

3. **Remove per-weapon demo content** (only after step 1's migration is confirmed safe):
   - Delete each weapon's `Demo/` folder entirely (`Weapons/<WeaponName>/Demo`).
   - Delete demo animation montages (filter Content Browser by Animation Montage type, scoped to the weapon folder).
   - Delete demo blend spaces (filter by Blend Space / Blend Space 1D).
   - Delete the weapon's demo Animation Blueprint — still references demo notify/notify-state logic even after the montages are gone.
4. **Remove the remaining `Common/` demo folders** — safe only after step 1's four folders have been moved out: `Common/Core`, `Common/Environment`, `Common/Props`, `Common/VFX`.
   - **Update redirectors first**, before deleting, per Infima's own recommendation.
   - Unreal may still warn that some files reference the deleted folder after this — Infima's guide calls this **expected and safe to ignore** in this specific cleanup scenario, since the warning is a stale-reference notice, not an active breakage (assuming steps 1–3 were done correctly first).

---

## Checklist

- [ ] Migrate `Common/Characters` (skeleton) to a project-owned folder. **Before this project's own animation content depends on anything else being deleted, confirm this step is complete.**
- [ ] Migrate `Common/Materials`, `Common/Audio`, `Common/AudioClasses` as needed (confirm which are actually still referenced by kept content before assuming all three are needed).
- [ ] Re-evaluate `infima_integration_plan.md` Phase 6's migration list against Phases 3–4 of this plan — skip migrating any Blueprint that's being replaced by a native rebuild instead.
- [ ] Delete per-weapon `Demo/` folders, demo montages, demo blend spaces, demo weapon AnimBP.
- [ ] Update redirectors, then delete remaining `Common/Core`, `Common/Environment`, `Common/Props`, `Common/VFX`.
- [ ] Cross-check against `infima_integration_plan.md` Phase 8 (Final Verification Sweep) once this phase completes — several of its still-open socket/skeleton verification items become easier to confirm once the migrated, project-owned assets are the only copies left.

## Exit criteria

- No project-owned animation, weapon, or character asset references anything under the original Infima plugin/demo folder path.
- `Content/Animation/InfimaAnimation/InfimaGames/...` (or wherever the original import landed) contains only reference material genuinely still needed for comparison, or is removed entirely if nothing remains that depends on it.
- A full project rebuild + PIE session shows no broken references, no missing skeleton errors, and no console warnings tracing back to a deleted Infima folder.
