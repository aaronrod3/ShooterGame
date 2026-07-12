# Infima Tactical FPS Animation Pack — Official Implementation Guide

> Compiled entirely from Infima Games' own published documentation for the Tactical FPS Animations pack (Web Docs, Guides & Tutorials, Blueprints Reference, Animation Reference). This guide treats the pack as if being implemented in a **brand-new Unreal Engine 5 project**, following the pack's own recommended order and conventions exactly as documented — it does not reference or adapt to any specific existing project's architecture. Where the pack's own docs use `WeaponName` as a placeholder (e.g. `SKEL_TFA_WeaponName`, `BP_TFA_WeaponName`), substitute the actual purchased product name (the docs' own worked example throughout is the Assault Rifle pack, asset prefix `AR`).

---

## What this pack actually is

Stated three times across the official docs for emphasis:

> "This product is not a ready-to-use FPS template or gameplay system. It is an art pack."

The included Blueprints, AnimBPs, and notify classes are **demonstration/reference logic** — they show how to wire the animation and weapon assets together correctly, but are explicitly not meant to be extended in place as a real game's gameplay systems. Infima's own phrasing: *"The included code logic is made specifically for the demo setup. You'll need to modify it to fit your project and integrate it into your own systems."*

### Concrete scope boundaries, stated directly in the docs

- **Engine requirement: Unreal Engine 5.4 or higher.**
- **No multiplayer/replication of any kind.** The one sentence in the entire documentation set that touches this: *"No, the demo logic is not replicated."* No further guidance exists anywhere in the docs.
- **No third-person locomotion (movement) animations are included, ever, by design.** Only first-person locomotion is provided. Direct quote: *"I am mainly focusing on high-quality upper-body movement animations because I don't feel confident in producing full-body movement animations. There are plenty of high-quality full-body movement packs available which can be integrated for the best results."* Recommended workflow: pair the included third-person **upper-body** animations with a separate, external full-body locomotion/mocap pack for the lower body.
- **No root motion anywhere in the pack.** All animations are authored in-place.
- **No reload gameplay logic in the demo**, deliberately: *"Reloading logic wasn't included in the demo as it would have added unnecessary complexity for what is essentially a visual showcase."*
- **The demo character is intentionally static/stationary in the demo map** — not a bug. Since there's no third-person locomotion content, letting the character actually move around would look wrong.
- **Sold as separate per-weapon products** (e.g. "Tactical FPS Animations Pack — Assault Rifle," "...— Pistol"). Each purchased product ships its own `Weapons/<WeaponName>/` folder. A second weapon type is a second product installation, not a reuse of the first product's assets for a different gun.
- **Blender source files ship for the weapon rig only** — never for the animations themselves. You can build a new weapon mesh on the provided rig, but you cannot open and edit Infima's original animation curves in Blender.
- **Retargeting note:** the animations are built for and best-suited to **UE5 Manny** (`SKEL_TFA_Mannequin`). A fully custom, non-Manny skeleton is technically possible via retargeting but explicitly discouraged: *"First-person animations can be tricky to retarget perfectly, and the finger/hand placement may not be perfect."*
- **No official roadmap or update commitment** beyond keeping pace with new engine versions: *"This pack is released as a complete, ready-to-use product right out of the box!... there is no official roadmap for future content updates."*

---

## Naming legend (used throughout every asset name in this pack)

| Prefix/Term | Meaning |
|---|---|
| `FP` | First-Person |
| `TP` | Third-Person |
| `WEP` | Weapon (weapon-mesh-specific, as distinct from character-mesh animations) |
| `PROP` | Prop (environment/interactive object) |
| `WeaponName` | Generic placeholder substituted with the actual purchased weapon product name (e.g. `AR` for Assault Rifle) |
| `SK_` | Skeletal Mesh |
| `SM_` | Static Mesh |
| `SKEL_` | Skeleton asset |
| `MI_` | Material Instance |
| `M_` | Material |
| `BP_TFA_` | Core Tactical FPS Animations Blueprint prefix |
| `ABP_TFA_` | Core Tactical FPS Animations Animation Blueprint prefix |
| `AN_TFA_` | One-shot Animation Notify prefix |
| `ANS_TFA_` | Animation Notify State (time-window) prefix |
| `BPI_TFA_` | Blueprint Interface prefix |
| `A_TFA_` | Animation asset prefix |

---

## How this guide is organized

Read in order — each numbered file assumes the previous ones are done, matching the order you'd naturally set this pack up in a fresh project:

1. **[01_Installation_And_Project_Structure.md](01_Installation_And_Project_Structure.md)** — get the pack installed and understand what you now have.
2. **[02_Core_Data_Asset_BaseConfig.md](02_Core_Data_Asset_BaseConfig.md)** — understand the single data asset everything else reads from.
3. **[03_Character_Blueprint_BaseCharacter.md](03_Character_Blueprint_BaseCharacter.md)** and **[04_Weapon_Blueprint_BaseWeapon.md](04_Weapon_Blueprint_BaseWeapon.md)** — the two core actors.
4. **[05_Supporting_Actors.md](05_Supporting_Actors.md)** — the smaller cosmetic actors these two spawn.
5. **[06_FirstPerson_AnimBP.md](06_FirstPerson_AnimBP.md)** and **[07_ThirdPerson_AnimBP.md](07_ThirdPerson_AnimBP.md)** — the animation Blueprints that make it all move.
6. **[08_Animation_Notifies_And_States.md](08_Animation_Notifies_And_States.md)** — the montage-driven glue between animation and gameplay.
7. **[09_Animation_Asset_Catalog.md](09_Animation_Asset_Catalog.md)** — full reference of every animation asset included.
8. **[10_Custom_Character_And_Weapon_Import.md](10_Custom_Character_And_Weapon_Import.md)**, **[11_LeftHand_Clipping_Fix.md](11_LeftHand_Clipping_Fix.md)**, **[12_Cleanup_Remove_Demo_Logic.md](12_Cleanup_Remove_Demo_Logic.md)** — the pack's official how-to guides, for once you're customizing.
9. **[13_FAQ_Scope_And_Constraints.md](13_FAQ_Scope_And_Constraints.md)** — everything else the official FAQ answers.
10. **[14_Master_Execution_Checklist.md](14_Master_Execution_Checklist.md)** — the condensed, ordered checklist tying all of the above together.
