# Step 10 — Official Guide: Custom Character Model and Custom Weapon Import

These are Infima's own two guides for bringing custom content onto the pack's rig/skeleton. Do this only after the demo setup (Steps 1–9) is working with the pack's own stock content — swapping content in before you have a working baseline makes it much harder to tell whether a problem is your custom asset or your setup.

---

## Guide A — How to Assign a Custom Character Model in Unreal Engine 5

**Prerequisite:** the custom character mesh must be rigged to **UE5 Manny** — bone hierarchy must match exactly.

1. **(Optional) Get a character asset from Fab:** Edit → Plugins → Fab, enable it, find a character (official example: "Modular Soldier Pack Vol. 1"), click **Add to Project**.
2. **Verify the import** — for the example pack, the UE5-compatible mesh lands at `<PackFolder> > Modules > ModularSoldierVol_1 > Meshes_UE5`.
3. **Duplicate the skeletal mesh first** (recommended) — keeps the original clean for rollback.
4. **Assign the skeleton:** right-click the duplicated mesh → **Skeleton → Assign Skeleton** → search for and select **`SKEL_TFA_Mannequin`** (the pack's own skeleton asset name).
5. **Verify bone match** — double-check all bones line up before clicking **Accept**. If bones are missing/mismatched, the mesh isn't actually Manny-compatible; fix the rig/hierarchy and retry rather than forcing the assignment.
6. **Test it:** open the weapon's config data asset (`Content/InfimaGames/TacticalFPSAnimations/Weapons/<WeaponName>/Demo/Data`), expand **Meshes → Character**, replace both the **TP mesh** and **FP mesh** fields with your custom mesh.
7. **Play a demo map** to confirm (example map name pattern: `L_TFA_AR_Showcase`).

No IK Retargeter step is needed here — this workflow assumes direct skeleton-sharing compatibility (custom mesh assigned onto the exact same `SKEL_TFA_Mannequin` skeleton), not cross-skeleton retargeting.

---

## Guide B — How to Replace the Weapon With a Custom Model (Blender to UE5)

**Requirements:** Blender 4.5 LTS or later (earlier untested); a custom weapon model (FBX recommended); this pack installed; the weapon-specific **Blender rig/source files** from the product's Fab page (see [01_Installation_And_Project_Structure.md](01_Installation_And_Project_Structure.md)).

**Selecting a model:** match the proportions of the original weapon as closely as possible — this reduces rework and improves hand/finger alignment with the existing animations.

### Part 1 — Blender

1. Open the weapon-specific Blender rig from the `Rigs/` folder (**duplicate before editing**).
2. **File → Import → FBX** to bring in your custom weapon model.
3. **Create a Collection** for it in the Outliner (right-click → New Collection), rename via **F2** (e.g. `RIG_CustomRifle`).
4. **Clean up extra meshes/transforms** (common issue with marketplace-sourced exports, which often include extra weapons/bullets/display props, or odd rotation):
   - Hide the original weapon's collection (uncheck its visibility box).
   - Select all (**A**), switch to wireframe (**Shift+Z**).
   - Box-select (**B**) while holding **Left Shift** to deselect the parts you want to keep, then delete the rest (**X → Delete**).
   - If the model is multiple separate objects, join with **Ctrl+J** for easier scaling/positioning.
   - Reset transforms: **Alt+R** (rotation), **Alt+G** (location).
5. **Fit scale and placement to the reference weapon:**
   - Re-enable the original weapon's collection as a placement reference.
   - Move (**G**) / rotate (**R**) to align. 180° flip trick: select the model → **R** → **Z** → type `180`.
   - **Alignment priority order, in this exact sequence:** Grip (right-hand placement) → Trigger + trigger area → Magazine (left-hand grab point) → Handguard/barrel (left-hand idle placement). Recommended approach: match the trigger area first, then adjust outward from there.
   - **Apply transforms:** **Ctrl+A → All Transforms** (zeroes location/rotation, sets scale to `1,1,1`).
   - **Rename** the mesh clearly, e.g. **`SK_CustomRifle`** (`SK` = skeletal mesh convention).
6. **Separate the magazine mesh** (optional, but recommended — *"the included demo logic expects the magazine to be its own mesh"*):
   - Enter Edit Mode on the weapon mesh, hover the magazine, **L** to select linked vertices (also manually include internal parts — bullets/spring/etc. — if present).
   - **P → Selection** to split it into its own object.
   - Rename, e.g. **`SM_CustomRifle_Magazine`** (`SM` = static mesh convention).
7. **Clean up unused material slots** (optional): Materials tab → **Remove Unused Slots**.
8. **Skin the weapon to the existing armature:**
   - Enable the **Deform Bones** collection (hidden by default) — these are the bones actually exported to and used in UE5. Controller bones exist purely to make animating easier in Blender and are **not** the deform hierarchy.
   - Shift-select the weapon mesh, then shift-select the armature (**armature must be selected second** — order matters for parenting).
   - **Ctrl+P → With Empty Groups** — adds an Armature modifier and creates empty vertex groups per deform bone.
9. **Assign vertex groups** for moving parts. Named deform-bone groups: **`Grip`, `Trigger`, `Bolt`, `Dust_Cover`, `Charging_Handle`, `Magazine_Release`, `Fire_Selector`**.
   - Workflow: in Edit Mode, select the whole mesh → assign everything to `Grip` first as a baseline. For each moving part: hover + **L** to select its geometry → click **Remove** in the `Grip` group → select the part's real group (e.g. `Trigger`) → click **Assign**. Repeat per moving part.
   - **Test weights:** select the armature → Pose Mode → move/rotate bones, watch for stretching, popping, or geometry following the wrong bone.
10. **Fix pivot issues on moving parts** (trigger, fire selector, dust cover, etc. not rotating cleanly around the intended point):
    - Re-enable the Deform Bones collection, enable **Names** under Viewport Display.
    - Select **both** the deform bone and its paired controller bone (example: `Fire_Selector` + `CB_Fire_Selector`), move both so the bone pivot (endpoint) sits at the mesh's actual hinge center. Repeat per part, re-test in Pose Mode.
11. **Fix the magazine pivot** (magazine mesh origin must align to the magazine bone, or attachment/animation behavior breaks):
    - Pose Mode → select the magazine bone → **Shift+S → Cursor to Selected**.
    - Object Mode → select the magazine mesh → **Object → Set Origin → Origin to 3D Cursor**.
    - Sanity check: temporarily add a **Copy Transforms** constraint on the magazine mesh targeting the magazine bone — if it lines up perfectly, correct. **Remove/disable the constraint and re-apply transforms before export.**
12. **Export FBX — two separate exports:**
    - **Skeletal mesh** (receiver): select the receiver mesh **and** the armature together → **File → Export → FBX** (e.g. `SK_CustomRifle.fbx`).
    - **Static mesh** (magazine): select the magazine mesh only, ensure any temporary constraints are disabled first → **File → Export → FBX** (e.g. `SM_CustomRifle_Magazine.fbx`).

### Part 2 — Unreal Engine

1. **Folder structure:** create a per-weapon folder (e.g. `CustomRifle/`) with subfolders `Materials/`, `Meshes/`, `Textures/`.
2. **Import the magazine mesh first** — default settings, skip auto-importing textures/materials if you don't want them yet. Invisible faces after import = flipped normals; fix in Blender and re-export.
3. **Import the receiver skeletal mesh** — **critical: select the correct existing weapon skeleton on import**, or animations won't play without a full retarget. Naming pattern: **`SKEL_TFA_WeaponName`** (e.g. `SKEL_TFA_AR` for the assault rifle, `SKEL_TFA_Pistol` for a pistol). UE auto-generates a Physics Asset on import — fine to leave as-is initially. Verify bones drive the correct moving parts (trigger, bolt) in the Skeletal Mesh Editor afterward.
4. **Fix common texture/material issues:**
   - **Normal maps:** UE expects DirectX-format normals; many downloaded assets use OpenGL. If shading looks "inside out," open the normal map texture and enable **Flip Green Channel**.
   - **Data textures** (Roughness/Metallic/AO): must have **sRGB disabled** — open each texture, uncheck sRGB.
   - **Material Instance:** right-click Materials folder → **Material → Material Instance**, name it **`MI_CustomRifle`**, set **Parent = `M_TFA_Weapon_Master`**. If roughness/metallic aren't packed, uncheck **Use Packed ORM Texture** and assign each texture individually. For bullet-specific textures, duplicate the instance with a `_Bullet` suffix (e.g. `MI_CustomRifle_Bullet`).
5. **Set up weapon sockets** (skip if using your own attachment system entirely) on the receiver skeleton hierarchy — at minimum:
   - **`SOCKET_Muzzle`** — muzzle flash/VFX spawn point
   - **`SOCKET_Laser`** — laser attachment spawn
   - **`SOCKET_Grip`** — grip attachment spawn
   - **`SOCKET_Eject`** — casing ejection spawn (needed if using `AN_TFA_EjectCasing` — not listed in the pack's own minimal socket-setup step, easy to miss, see [08_Animation_Notifies_And_States.md](08_Animation_Notifies_And_States.md))
   - Tip: you can set a preview mesh on a socket to make placement easier.
6. **Test in the demo map:**
   - Open the weapon's data asset (e.g. `DA_TFA_AssaultRifle`).
   - Assign the custom receiver and magazine meshes in their respective slots.
   - Remove meshes you don't need (silencer, scope, etc. if your custom weapon doesn't have them); keep the ones you still want (bullet, casing, laser, grips).
   - Press Play.

### Most common causes of "it looks off"
- Forgetting to apply transforms before export.
- Missing sockets.
- Incorrect weight painting (multiple bones influencing the same part).
- Flipped normals (invisible faces in-engine).

### Troubleshooting
| Symptom | Cause |
|---|---|
| Attachments spawn at origin / no muzzle flash | Socket name/placement issue |
| Left-hand clipping through the gun/grip area | Very common after any weapon swap — see [11_LeftHand_Clipping_Fix.md](11_LeftHand_Clipping_Fix.md) |
| No bullets visible in magazine during mag-check/reload | Demo scans magazine sockets by a **prefix** (commonly `Bullet_`, but config-driven — confirm the actual value in `BP_TFA_BaseConfig`/`BP_TFA_BaseMagazine`); expected naming `Bullet_001`, `Bullet_002`, etc. Quick fix: duplicate the old magazine mesh, right-click → **Reimport with New File** to swap content while keeping the same asset reference |

## Checklist

- [ ] Complete the Blender workflow end-to-end for one weapon before attempting a second.
- [ ] Confirm both FBX exports (skeletal receiver + static magazine) happen with transforms already applied.
- [ ] Confirm the correct existing weapon skeleton is selected on skeletal mesh import.
- [ ] Fix normal-map/sRGB issues before judging final material quality.
- [ ] Create all four minimum sockets (`SOCKET_Muzzle`, `SOCKET_Laser`, `SOCKET_Grip`, `SOCKET_Eject`) even if the "official minimum" list only mentions three.
- [ ] Test in the demo map before considering the import complete.
