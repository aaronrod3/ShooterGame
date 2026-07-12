# Step 1 — Installation and Project Structure

## Installing via Fab

1. Open the **Fab** window inside Unreal Engine. If Fab doesn't appear in the editor, consult Epic's own Fab setup documentation first.
2. In the Fab window, go to **Library**, and find your purchased product (worked example throughout the official docs: **"Tactical FPS Animations Pack — Assault Rifle"**).
3. Click **Add to Project** → choose the target engine version if prompted → click **Add to Project** again to confirm.
4. Wait for the download to complete. When finished, an **`InfimaGames`** folder appears at the root of your project's `Content/` directory.
5. Open **`InfimaGames/TacticalFPSAnimations`** — this is the root of everything the pack installs.

## Demo content and reference logic locations

- **Demo map:** `InfimaGames > TacticalFPSAnimations > Weapons > WeaponName > Demo > Maps` (example map name pattern: `L_TFA_AR_Showcase`, varies per weapon).
- **Demo Blueprint/reference logic:** `InfimaGames > TacticalFPSAnimations > Common > Core`.
- Treat everything under `Common/Core` as **reference and learning material**, not a ready-to-ship FPS template — this is the pack's own stated framing, not an external opinion.

## Blender source files (weapon rig only — not animations)

- Available in the Fab product page's **Additional/Included files** section, as a per-weapon ZIP: `blender_source_files_tacticalWeaponName.zip`.
- Extracted contents: two subfolders only —
  - **`Rigs/`** — the weapon rig (bone hierarchy, sockets, mesh) usable as a base for authoring new animations or importing a custom weapon mesh onto the same skeleton.
  - **`Textures/`** — material texture source files.
- **Explicitly not included:** Blender source files for the animations themselves. You get the rig to animate against, not the original animation curves to edit.

---

## Full project folder structure

The pack splits its content into two top-level areas under `InfimaGames/TacticalFPSAnimations/`: **`Common/`** (shared across every weapon product you own) and **`Weapons/`** (per-weapon-product content).

### `Common/` folder tree

| Path | Contents |
|---|---|
| `Common/Audio/` | Subfolders: `Foley/`, `Heal/`, `Melee/` |
| `Common/AudioClasses/` | Shared audio classes / sound control assets |
| `Common/Characters/` | Subfolders: `Mannequins/`, `Materials/`, `Instances/`, `Meshes/`, `Rigs/`, `Poses/`, `Manny/`, `Textures/` — **contains the shared mannequin skeleton every included animation is assigned to** |
| `Common/Core/` | Subfolders: `Animation/`, `Characters/`, `Configs/`, `Data/`, `Inputs/`, `UI/`, `Fonts/`, `Weapons/` — the pack's shared setup, config, UI, and data (this is where all the reference Blueprints/AnimBPs covered in this guide live) |
| `Common/Environment/` | Subfolders: `Animations/`, `Blueprints/`, `Materials/`, `Meshes/`, `Textures/` — demo-map environment set dressing |
| `Common/Materials/` | Shared base materials — includes the master weapon material (`M_TFA_Weapon_Master`) that per-weapon material instances parent to |
| `Common/Props/` | Subfolders: `Animations/`, `Blueprints/`, `Meshes/` — reusable prop assets (e.g. the healing syringe) |
| `Common/VFX/` | Subfolders: `Emitters/`, `Materials/`, `Systems/`, `Textures/` |

### `Weapons/<WeaponName>/` folder tree

| Path | Contents |
|---|---|
| `Weapons/<WeaponName>/Animations/Character/FP/` | Subfolders: `Combat/`, `Interactions/`, `Locomotion/`, `Poses/`, `Transitions/` |
| `Weapons/<WeaponName>/Animations/Character/TP/` | Subfolders: `Combat/`, `Interactions/`, `Locomotion/`, `Poses/`, `Transitions/` — note: per the FAQ, no real movement content lives under `TP/Locomotion/` even though the folder exists in the standard structure; see [13_FAQ_Scope_And_Constraints.md](13_FAQ_Scope_And_Constraints.md) |
| `Weapons/<WeaponName>/Animations/Weapon/` | Subfolders: `Common/`, `FP/`, `TP/` |
| `Weapons/<WeaponName>/Audio/` | Subfolders: `Firing/`, `Handling/`, `Bolt/`, `Foley/`, `Magazine/`, `Impacts/` |
| `Weapons/<WeaponName>/Demo/` | Subfolders: `Data/`, `Maps/` — the weapon-specific demo data asset and showcase map |
| `Weapons/<WeaponName>/Materials/` | Weapon-specific materials |
| `Weapons/<WeaponName>/Meshes/` | Weapon meshes |
| `Weapons/<WeaponName>/Textures/` | Weapon textures |

## Checklist

- [ ] Confirm Unreal Engine 5.4 or higher.
- [ ] Install the weapon product via Fab.
- [ ] Confirm the `InfimaGames/TacticalFPSAnimations` folder exists post-install.
- [ ] Locate and open the demo map to confirm the install is functional before doing anything else.
- [ ] If planning to author new weapon content later, download the weapon's Blender source ZIP now while you know exactly which product/version you're on.
