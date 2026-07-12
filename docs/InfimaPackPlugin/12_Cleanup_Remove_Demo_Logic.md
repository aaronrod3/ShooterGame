# Step 12 — Official Guide: Safely Remove the Demo Code Logic

Once you've confirmed the demo setup works (Steps 1–9) and finished any custom character/weapon import (Steps 10–11), this is the pack's own recommended path to strip out demo-only Blueprint logic and keep just the art content — animations, meshes, textures — as a clean base for your own gameplay systems.

**Stated purpose, directly from the docs:** *"...ideal if you want to use the included Tactical FPS assets in your own Unreal Engine 5 setup without keeping the showcase Blueprints and demo-specific logic."*

## The one rule that matters most

> **`Common/Characters` contains the mannequin skeleton every included animation is assigned to. Deleting this folder WILL break all of the included animations.**

Do not delete or restructure anything under `Common/` until the folders below have been moved out first. This is an ordering requirement, not a suggestion — do the steps in the order given.

## Steps, in order

1. **Delete the per-weapon Demo folder:** `InfimaGames/TacticalFPSAnimations/Weapons/WeaponName/Demo`.
2. **Delete demo animation montages** — filter the Content Browser by Animation Montage type, scope to the weapon folder, select all, delete. These are heavily used by the demo showcase and depend on the demo notify/notify-state logic.
3. **Delete demo blend spaces** — filter by Blend Space and Blend Space 1D, select, delete (also referenced by demo logic).
4. **Delete the weapon Animation Blueprint** — still references demo logic even after the montages are gone.
5. **Move these four `Common/` folders OUT to your own project structure before deleting anything else in `Common/`:**

   | Folder | Why it must be preserved |
   |---|---|
   | **`Audio`** | Footstep/cloth foley sounds used directly by locomotion animation sequences |
   | **`AudioClasses`** | Master sound class referenced by the included audio cues — keep only if keeping the included SFX |
   | **`Characters`** | **Most important** — contains the shared mannequin skeleton every animation is assigned to. Deleting it breaks everything |
   | **`Materials`** | Master material used by weapon material instances |

6. **After the four folders above are safely relocated, the remaining `Common/` folders are safe to delete:** `Core`, `Environment`, `Props`, `VFX`.
   - **Update redirectors first**, before deleting, to help avoid cleanup issues.
   - Unreal may still show a warning that some files reference the deleted folder afterward — **this is expected and safe to ignore** in this specific cleanup scenario (assuming the prior steps were done correctly).

## End state

A clean, art-only content setup: essential art content (animations, meshes, textures, the shared skeleton, the master material) retained, demo showcase Blueprint logic removed — providing, in the pack's own words, *"a much cleaner base for building your own weapon systems, gameplay logic, and project-specific folder setup."*

## Checklist

- [ ] Confirm the demo setup and any custom import work are both verified working before starting cleanup.
- [ ] Delete the per-weapon `Demo/` folder.
- [ ] Delete demo montages and blend spaces.
- [ ] Delete the demo weapon Animation Blueprint.
- [ ] Move `Common/Audio`, `Common/AudioClasses`, `Common/Characters`, `Common/Materials` to your own project structure — **before** touching the rest of `Common/`.
- [ ] Update redirectors, then delete `Common/Core`, `Common/Environment`, `Common/Props`, `Common/VFX`.
- [ ] Confirm no animation broke as a result — if `Common/Characters` was moved (not deleted) and references updated correctly, nothing should have broken.
- [ ] Ignore (don't chase down) any stale "still referenced" warnings that appear immediately after this cleanup, per the pack's own guidance.
