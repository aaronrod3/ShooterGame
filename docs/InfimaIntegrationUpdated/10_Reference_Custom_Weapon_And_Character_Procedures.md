# Reference — Custom Weapon & Character Import Procedures

**Not a phase — a repeatable procedure.** Consult this whenever a new weapon type is added (remember: Infima sells weapon packs separately, so a second weapon type means installing a second product with its own `Weapons/<WeaponName>/` folder) or a new character model needs to go in. Distilled from Infima's own Guides 1–3, which were read in full (twice, independently, with consistent results) — condensed here to the exact click-paths and gotchas, with marketing/framing text stripped out.

---

## Procedure A — Swap in a custom character model

Source: Infima's "How to Assign a Custom Character Model in Unreal Engine 5" guide.

**Prerequisite:** the custom mesh must be rigged to UE5 Manny with a matching bone hierarchy — this is a hard requirement, not a suggestion. ShooterGame already uses `SK_Mannequin`, so this is the well-supported path (Infima's FAQ explicitly recommends staying on Manny-compatible meshes; a fully custom, non-Manny skeleton is "technically possible" but explicitly warned against — *"the finger/hand placement may not be perfect"*).

1. **Duplicate the target skeletal mesh first** — never edit the original in place, for rollback safety.
2. Right-click the duplicated mesh → **Skeleton → Assign Skeleton** → search for and select **`SKEL_TFA_Mannequin`** (or this project's equivalent skeleton asset — confirm the exact name in `Content/`).
3. Verify all bones match in the confirmation dialog before accepting — a mismatch means the mesh isn't actually Manny-compatible; fix the rig/hierarchy and retry rather than forcing the assignment.
4. Test by assigning the new mesh to both the FP and TP mesh fields on the relevant weapon/character config asset, then Play in a test level.

---

## Procedure B — Import a custom weapon model (Blender → UE5)

Source: Infima's "How to Replace the Weapon With a Custom Model" guide — by far the longest guide in the pack's docs, condensed here to a checklist. Requires Blender 4.5 LTS or later.

### In Blender
1. Open the weapon-specific Blender rig from the pack's `Rigs/` folder (from the purchased product's Fab "Additional files" — duplicate before editing).
2. Import the custom weapon FBX into a new collection (rename via **F2**, e.g. `RIG_CustomWeaponName`).
3. Clean up: hide the original weapon collection, select-and-delete anything not part of the new model (`A` select all, `Shift+Z` wireframe, `B` box-select with `Shift` held to deselect keepers, `X → Delete`). Join multi-part imports with `Ctrl+J`. Reset transforms (`Alt+R`, `Alt+G`).
4. **Scale/align priority order, exactly as Infima specifies — do this in order, not simultaneously:** Grip (right hand) → Trigger/trigger area → Magazine (left hand) → Handguard/barrel (left-hand idle placement). Apply final transforms with `Ctrl+A → All Transforms`. Name the mesh `SK_<WeaponName>`.
5. **Separate the magazine into its own mesh** (recommended — "the included demo logic expects the magazine to be its own mesh"): select linked geometry (`L`) including internal parts (bullets/spring), `P → Selection` to split, name it `SM_<WeaponName>_Magazine`.
6. Clean unused material slots (Materials tab → **Remove Unused Slots**).
7. **Skin to the existing armature:** enable the hidden **Deform Bones** collection (controller bones exist only to make Blender animation easier — they are *not* what gets exported/used for deformation). Shift-select mesh, then shift-select armature (order matters — armature must be selected second), `Ctrl+P → With Empty Groups`.
8. **Assign vertex groups** for moving parts — Infima's own named convention: `Grip`, `Trigger`, `Bolt`, `Dust_Cover`, `Charging_Handle`, `Magazine_Release`, `Fire_Selector`. Workflow: assign the whole mesh to `Grip` as a baseline, then per moving part select its geometry, remove it from `Grip`, assign it to its real group.
9. **Test weights** in Pose Mode — move/rotate bones, watch for stretching, popping, or a part following the wrong bone.
10. **Fix pivot issues on moving parts:** select both the deform bone and its paired controller bone (e.g. `Fire_Selector` + `CB_Fire_Selector`), move both so the bone pivot sits at the mesh's actual hinge point.
11. **Fix the magazine pivot specifically:** Pose Mode → select the magazine bone → `Shift+S → Cursor to Selected` → Object Mode → select magazine mesh → **Object → Set Origin → Origin to 3D Cursor**. Sanity-check with a temporary **Copy Transforms** constraint targeting the magazine bone — if it lines up, correct. **Remove/disable the constraint and re-apply transforms before export.**
12. **Export two separate FBX files:** the receiver as a skeletal mesh (select mesh + armature together), the magazine as a static mesh (select magazine only, constraints disabled first).

### In Unreal Engine
1. Create a per-weapon folder structure: `<WeaponName>/{Materials, Meshes, Textures}`.
2. Import the magazine (static mesh) first — default settings, skip auto-texture/material import if unwanted. Invisible faces after import = flipped normals; fix in Blender and re-export.
3. Import the receiver (skeletal mesh) — **critical: select the correct existing weapon skeleton on import**, or animations won't play without a full retarget. Naming convention: `SKEL_TFA_<WeaponName>` (e.g. `SKEL_TFA_AR`, `SKEL_TFA_Pistol`). Verify moving parts (trigger, bolt) drive correctly in the Skeletal Mesh Editor afterward.
4. **Normal maps:** if shading looks inverted, the source used OpenGL-format normals but UE expects DirectX — open the texture, enable **Flip Green Channel**.
5. **Data textures** (Roughness/Metallic/AO): must have **sRGB disabled** — uncheck it per texture.
6. **Material Instance:** right-click Materials folder → **Material → Material Instance**, name `MI_<WeaponName>`, set **Parent = M_TFA_Weapon_Master** (confirm this project's actual master material name matches, post-migration in [09_Migration_And_Cleanup.md](09_Migration_And_Cleanup.md)). If textures aren't packed ORM, uncheck **Use Packed ORM Texture** and assign individually. For bullet-specific textures, duplicate the instance with a `_Bullet` suffix.
7. **Sockets to create on the receiver skeleton** (Infima's guide lists three as the demo-required minimum; a fourth is needed if using the eject-casing notify at all — see the naming note below):
   - `SOCKET_Muzzle` — muzzle flash / VFX spawn
   - `SOCKET_Laser` — laser attachment spawn
   - `SOCKET_Grip` — grip attachment spawn
   - `SOCKET_Eject` — casing ejection spawn point (confirmed from the notify docs, not listed in the main socket-setup step — easy to miss)
8. Populate the weapon's config data asset (`UWeaponConfig` instance) with the new receiver/magazine meshes, then test in a PIE session.

### Common failure causes (Infima's own troubleshooting list)
- Forgetting to apply transforms in Blender before export.
- Missing sockets (symptom: attachments/effects appear to spawn at the weapon's origin instead of the correct location).
- Incorrect weight painting — multiple bones influencing the same part.
- Flipped normals — invisible faces in-engine.
- No bullets appearing in the magazine during mag-check/reload — the magazine mesh needs sockets matching `PrefixBulletSocket` (confirm the actual value post-[Phase 1](01_Config_And_Montage_Ownership_Cleanup.md) cleanup rather than assuming `"Bullet_"`), named e.g. `Bullet_001`, `Bullet_002`. Quick alternative: duplicate the old magazine mesh, right-click → **Reimport with New File** to swap content while keeping the same asset reference.

---

## Procedure C — Fix left-hand clipping (after any weapon swap)

Source: Infima's dedicated guide, directly cross-referenced from Procedure B's troubleshooting section as "a super common issue when replacing existing weapon models."

1. Locate the existing FP idle pose to use as an edit base.
2. **Edit `ik_hand_l`, never `hand_l` directly** — the demo (and ShooterGame's native equivalent) auto-attaches hands to the weapon via FABRIK; adjusting the raw hand bone fights against the IK solve instead of cooperating with it. Nudge `ik_hand_l` on the relevant axis to clear the clipping mesh.
3. **The pose preview does not update live** while adjusting the IK bone — expect to iterate blind and re-check rather than tuning interactively.
4. Save as a **new** pose asset (don't overwrite the original) via **Create Asset → Create Animation → Current Pose**.
5. Test by swapping the new pose into the weapon config's FP idle pose field and playing.
6. **Scope limit, stated explicitly by Infima's own guide:** this is a quick fix for minor clipping only. A substantially different custom grip needs either a full new pose set authored from scratch, or a more advanced dynamic IK setup — this procedure doesn't cover either.

---

## When to use this reference

- Adding a second (or third) weapon type to the project.
- Any time left-hand clipping appears after a weapon mesh change — check Procedure C before assuming it's an IK bug in the shared AnimBP.
- Swapping in a new player character mesh, cosmetic-only or otherwise.
