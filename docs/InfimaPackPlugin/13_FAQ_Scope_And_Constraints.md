# Step 13 — Official FAQ, Scope, and Constraints

Everything the official FAQ and Web Docs answer, organized by topic. Read this fully before making architectural decisions on top of the pack — several answers here directly determine what is and isn't worth attempting.

## Overview

- *"No, this is an art-focused pack (animations + weapon assets)."* Included Blueprints are demo/showcase only.
- Pack contents, stated plainly: high-quality FPS animations; weapon models + materials/textures; audio/VFX.

## Installation and requirements

- **Engine version floor: Unreal Engine 5.4 or higher.**
- *"Yes, it's an asset pack, so it's intended to be added to an existing Unreal project. It will not replace your config files or project specific settings."*
- Blender source files: *"Yes, the source files for the weapon rig are included with the purchase"* (found on the Fab product page) — but *"the Blender source files for the animations are not included. Only the static weapon model and rig/skeleton are included."*

## Finding content

Folder paths as covered in [01_Installation_And_Project_Structure.md](01_Installation_And_Project_Structure.md) — `Weapons/WeaponName/{Animations,Meshes,Materials,Textures,Audio}` for per-weapon content, `Common/` for shared/demo systems.

## Character compatibility and retargeting

- *"Yes, the animations are UE5 Manny compatible."*
- Using your own character: *"You can use your own character as long as it's rigged to the same skeleton hierarchy."* See [10_Custom_Character_And_Weapon_Import.md](10_Custom_Character_And_Weapon_Import.md).
- Retargeting to a fully custom (non-Manny) skeleton: *"Technically speaking, yes, but you'll need to retarget them to your custom skeleton. The exact steps depend on your skeleton and proportions. First-person animations can be tricky to retarget perfectly, and the finger/hand placement may not be perfect."* Explicit recommendation: *"highly recommended to use the animations with a UE5 Manny-compatible character model for the best results."*

## Animations and workflow

- **Camera shake:** *"it's super easy to disable or decrease the intensity of the camera shake, you can do it directly in an animation blueprint with one node. There is an example included inside the first person animation blueprint's AnimGraph."* Mechanism: *"The first person camera shake animations are baked directly onto the head bone of the character. You can use a Layered blend per bone node to limit how much the head bone moves, use the blend weights to adjust the intensity."* — see the head/camera toggle in [06_FirstPerson_AnimBP.md](06_FirstPerson_AnimBP.md).
- **Coverage:** *"The pack includes both first-person and third-person animations for actions like reloads, inspections, grenade throws, malfunctions, and interactions."*
- **No third-person locomotion — stated explicitly and emphatically:** *"there are no third-person movement (locomotion) animations included, those are only included for first-person."* Recommended workflow: *"use the third-person upper-body animations in combination with mocap (or similar) third-person locomotion data on the lower body."* Author's own stated reasoning: *"I am mainly focusing on high-quality upper-body movement animations because I don't feel confident in producing full-body movement animations. There are plenty of high-quality full-body movement packs available which can be integrated for the best results."*
- **No root motion:** *"They are animated in place, no root motion is included."*
- **Bodycam / head-attached TP camera:** *"Yes, technically, you can use the third-person animations with a camera attached to the head bone. You can try this in the demo build, which includes 'bodycam' perspectives... the third-person animations were not created with a true FPS setup in mind, so the head movements can be pretty motion-sickness-inducing by default. Therefore, it would require some manual work to get it feeling right."*

## Demo logic

- **The only sentence on multiplayer/replication in the entire documentation set: "No, the demo logic is not replicated."** No elaboration, no guidance on how to add it — that responsibility is entirely yours.
- *"No, you can use the animations and weapon assets without using the demo blueprints, they are not a requirement. The demo logic is mainly there to preview the content and demonstrate how to properly set up and use the animations in a game scenario."*
- **Static demo character explained as intentional, not a bug:** *"The character is intentionally static in the demo map... the third-person mode does not include movement animations, so it would look weird if the character just slid around."*
- **Ammo/magazine cosmetic system:** *"The ammo value in the demo is only used to drive the animated magazine, showing bullets being removed as you fire and the spring/follower adjusting based on how many rounds are left."*
- **No reload logic in the demo, deliberately:** *"Reloading logic wasn't included in the demo as it would have added unnecessary complexity for what is essentially a visual showcase."*

## Extending and customizing the pack

- Custom weapon mesh: *"Yes, but it will require manual work to adapt the custom weapon mesh to fit the skeleton included in the pack."* See [10_Custom_Character_And_Weapon_Import.md](10_Custom_Character_And_Weapon_Import.md).

## Licensing, updates, and support

- Redistribution/usage: *"Yes, as long as you follow the Fab license terms for the product"* (`fab.com/eula`).
- **No roadmap commitment:** *"This pack is released as a complete, ready-to-use product right out of the box!... there is no official roadmap for future content updates, so please base your purchase on the current state of the pack... While I may occasionally add new models or animations down the line, those should just be considered a free bonus rather than a guarantee."*
- Engine version maintenance: *"I will continuously make sure that the pack works with the latest engine versions."*
- Support/feedback channel: **`support@infimagames.com`** (email preferred over Fab's review section for feature requests).

## Product-line structure

Sold as **separate per-weapon products** (e.g. "Tactical FPS Animations Pack — Assault Rifle," "— Pistol"). The `WeaponName` placeholder used throughout every asset path is literally the purchased product's own folder name — owning only one weapon product means only that one `Weapons/<WeaponName>/` folder exists; other weapon types require separately purchasing and installing their own product. Melee/healing/interactions/malfunctions content is bundled into whichever weapon product you buy (each ships its own full animation category set), not sold as a separate add-on.

## Checklist

- [ ] Confirm your engine version is 5.4+.
- [ ] Decide up front how you'll source third-person locomotion content — this pack will never provide it.
- [ ] Decide up front how you'll implement multiplayer/replication if needed — this pack provides zero guidance here.
- [ ] Don't expect reload gameplay logic to already exist anywhere in the demo — you're building that yourself.
- [ ] If buying a second weapon type, expect to install a second, separate product with its own folder.
