# Infima Tactical FPS Animation Pack — Adoption Plan

> Compiled 2026-07-11 from a full read of the pack's official documentation (Blueprints reference, Animation reference, Guides & Tutorials, Web Docs/FAQ) cross-checked directly against ShooterGame's current C++ source. This is a **systems adoption roadmap** — it catalogs every system the pack ships and plans the work to properly build/wire each one into ShooterGame's native component architecture. It does not replace [`docs/infima_integration_plan.md`](../infima_integration_plan.md), which remains the authoritative, actively-maintained tracker for the specific AnimBP/camera bug-fix work already in flight. Where the two overlap, this plan cross-references rather than duplicates — see each phase's "Relationship to existing plan" note.

---

## Source material and a tooling caveat

Four PDFs were provided, but three reported page counts (445, 112) turned out to be wrong for the actual files — likely computed against a combined master document before the user split them:

| File | Reported | Actual | Coverage |
|---|---|---|---|
| `4. Infima Pack Blueprints Section.pdf` | 424.8KB | 28 pages | Read in full directly |
| `3. Infima Pack Animation Section.pdf` | 730.4KB | 54 pages | Read in full directly |
| `2. Infima Pack Guides Section.pdf` | "445 pages" | **79 pages** | Read in full, twice independently (two agents both read all 79 pages and produced consistent, corroborating reports) |
| `1. Infima Pack Web Docs Copied.pdf` | "112 pages" | **20 pages** | Read in full |

**Tooling limitation encountered:** this environment has `pdftotext` but not `pdftoppm`/poppler's renderer, Ghostscript, or ImageMagick. Any genuinely image-only PDF page (a screenshot with no embedded text layer) cannot be read here — confirmed on exactly 2 pages across the whole document set (both Blender FBX exporter-settings screenshots, contents described narratively in surrounding text but exact checkbox states not recoverable). This is a hard environment gap, not a missed step — if those exact export settings ever matter, view that PDF page directly.

---

## Reality check: what this pack actually is

Straight from Infima's own FAQ, stated three times for emphasis: **"This product is not a ready-to-use FPS template or gameplay system. It is an art pack."** Several concrete implications, confirmed directly from the docs, that should shape expectations for every phase below:

- **"No, the demo logic is not replicated."** — the single sentence, verbatim, covering multiplayer in the entire doc set. Every replication decision for every system below is ShooterGame's own responsibility; there is no upstream guidance to lean on. See [02_Multiplayer_Replication_Principles.md](02_Multiplayer_Replication_Principles.md).
- **Zero third-person locomotion (movement) animations are included, by deliberate authorial choice** ("I don't feel confident in producing full-body movement animations... there are plenty of high-quality full-body movement packs available"). This is not a gap in the port — it is a gap in the *source content*, and no amount of AnimBP wiring fixes it. See [07_ThirdPerson_Locomotion_Sourcing.md](07_ThirdPerson_Locomotion_Sourcing.md).
- **No root motion** on any animation (all in-place).
- **No reload logic in the demo at all** — deliberately omitted as "unnecessary complexity for what is essentially a visual showcase." ShooterGame's reload system (`CombatComponent::ReloadEquippedWeapon` → `ServerReload`/`MulticastReload`/`MulticastFinishReload`) was built from scratch, not un-ported from something that exists — worth knowing so it isn't mistaken for unfinished porting work.
- **Sold as separate per-weapon products** (e.g. "Tactical FPS Animations Pack — Assault Rifle"). Adding a second weapon type means installing a second Infima product with its own `Weapons/<WeaponName>/` folder, not reusing this one's assets. See [10_Reference_Custom_Weapon_And_Character_Procedures.md](10_Reference_Custom_Weapon_And_Character_Procedures.md).
- **Engine floor: UE 5.4+.** ShooterGame is on 5.8 — no compatibility concern.
- **Blender source files ship for the weapon rig only**, never for the animations themselves — you cannot open and tweak Infima's original animation curves in Blender, only author new clips against the same rig.
- Camera-shake dampening (the head-bone `LayeredBoneBlend` in the FP AnimGraph) is a **documented, intentional, tunable feature**, not an incidental node — confirmed by Infima's own FAQ describing exactly this technique.
- The "bodycam" perspective is **explicitly acknowledged by Infima as rough and "motion-sickness-inducing by default"** — deprioritize it; it was never meant to be a polished, ship-ready mode.

---

## Systems inventory — Infima asset/system → ShooterGame status → phase

Status is grounded in a direct read of current source (2026-07-11): `WeaponConfig.h/.cpp`, `CombatComponent.h`, `ShooterAnimInstanceBase.h`, `CombatTypes.h`, `Weapon.h/.cpp`, `CaseEject.h`, `ShooterGameCharacter.h`. Where this contradicts older memory/notes, trust this table — it was re-verified today.

| System | Infima Reference | ShooterGame Status | Phase |
|---|---|---|---|
| Weapon/character config data asset | `BP_TFA_BaseConfig` | **Built, but has 3-way field duplication** (legacy bare fields, current `TP_*`/`FP_*`/`*_WEP_*` fields, and a *third* system living directly on `ShooterGameCharacter`) needing consolidation before anything else is safe to build on | 1 |
| ADS/Recoil/Crouch procedural offsets | Spring-interpolated `FTransform` triplet | **Two parallel, non-identical representations exist** (`ADSLocationOffset`/`ADSRotationOffset`/`RecoilRotationPerShot`/`RecoilTranslationPerShot` vs. `OffsetAimDownSights`/`OffsetCrouch`/`OffsetCantedAim` as `FTransform`) — only one appears consumed | 1 |
| Multiplayer/replication | None — not covered by the pack at all | Core aiming/action state (`EquippedWeapon`, `bAiming`, `CurrentCombatAction`, `bHighReady`) is **already correctly replicated** via the `OnRep_X` + `Server_X()` pattern. `CurrentGrip` is the one flagged-but-likely-fine gap | 2 |
| Weapon mesh cosmetic assembly (handguard/scope/sights/silencer) | `BP_TFA_BaseWeapon` construction script + `AssignNewStaticMesh` | **Already built** — `AWeapon::AttachStaticMeshFromConfig()` + `MeshComp_Handguard/Scope/SightFront/SightRear/Silencer` exist and mirror the Infima pattern exactly | 3 |
| Grip mesh assembly (vertical/angled foregrip) | `RandomizeGripAttachment()` cycling None/Vertical/Angled | **Not yet wired** — `EWeaponGrip` enum and `WeaponConfig.WeaponGrip` exist, but no component slot/attach call consumes `MeshGripVertical`/`MeshGripAngled` yet. No runtime cycling action exists either (config-driven per equip instead — likely fine, see Phase 3) | 3 |
| Laser attachment | `BP_TFA_Attachment_Laser` | **Not started** — no equivalent actor exists | 4 |
| Casing ejection physics prop | `BP_TFA_PhysicsCasing` | **Already built and more sophisticated than Infima's own** — `ACaseEject` has surface-aware impact audio, rest-detection, lifetime-after-rest, and a world casing-count cap. `AWeapon::EjectCasing()` even already has a `CasingClass` property — it's just a stub that logs a warning instead of spawning it | 3–4 |
| Dropped magazine physics prop | `BP_TFA_PhysicsMagazine` | **Not started** — `AWeapon::SpawnDroppedMagazine()` is a log-only stub with no backing actor | 4 |
| Magazine cosmetic visibility (main/reserve) | `SetMagazineVisibility()`, socket-driven | **Stub only** — `AWeapon::SetMagazineVisibility()` logs a warning; unclear yet whether cosmetic magazine mesh actors/components even exist to hide | 3–4 |
| Bullet-socket magazine visualization | `BP_TFA_BaseMagazine` scans `Bullet_*` sockets | Not confirmed built; `WeaponConfig.PrefixBulletSocket` field exists (defaults `"Bullet_"`) | 4 |
| Animation notify classes (one-shot) | `AN_TFA_DropMagazine/EjectCasing/SpawnObjectAttached/ThrowPhysicsObject/UnlockActions` | **0 of 5 exist** in `Source/` (confirmed via prior Glob per project memory) | 5 |
| Animation notify-state classes | `ANS_TFA_BlockADS/HideMainMag/ShowReserveMag/LeftHandGrip` | **2 of 4 exist** (`ANS_BlockADS`, `ANS_LeftHandGrip`); `ANS_BlockADS`'s `ForceStopAiming()` call target **does not exist anywhere in `Source/`** — and the Guides now confirm Infima's own reference implementation treats this as a real gameplay-side aim-cancel, not a cosmetic flag | 5 |
| FP/TP core AnimBP systems | `ABP_TFA_FP_BaseCharacter`/`ABP_TFA_TP_BaseCharacter` | Actively tracked by `infima_integration_plan.md` Phases 1–4 — this plan only adds new corroborating detail, not a duplicate tracker | 6 |
| Third-person locomotion content | *(does not exist in the pack — confirmed authorial decision)* | `SM_TP_Locomotion`'s single-state design is **an intentional scope decision, not a content gap** — corrected 2026-07-12 after initially misreading it as incomplete; see [11_Refactoring_Recommendations.md](11_Refactoring_Recommendations.md). Infima still has zero TP locomotion reference architecture if/when more is ever needed | 7 |
| Melee combat | Animation assets only, no demo Blueprint logic | `ECombatAction::Meleeing` enum value **already exists**; `ShooterGameCharacter` already has `Montage_Melee_Bash/Swing_L/Swing_R` fields; `WeaponConfig` already has `FP_Melee`/`TP_Melee` arrays. No logic connects any of it yet | 8 |
| Healing | `AN_SpawnObjectAttached` + `AN_ThrowPhysicsObject` demo use case (syringe) | `ECombatAction::Healing` **already exists**; `WeaponConfig.FP_Healing`/`TP_Healing` arrays exist. No logic yet | 8 |
| Interactions (grab/punch/push) | Animation assets + generic interaction montages | `ECombatAction::Interacting` **already exists**; `ShooterGameCharacter` already has `InteractionMontage_Unarmed/Pistol/Rifle` — a *category*-based pattern, distinct from WeaponConfig's per-instance arrays | 8 |
| Weapon malfunctions/jam-clearing | Animation assets + arrays | **No `ECombatAction` value exists yet** for this at all, despite `ShooterGameCharacter` already having `Montage_ClearJam_MagSwipe`/`Montage_ClearJam_Rack` fields sitting unused | 8 |
| Grenade throw | Animation assets | `ECombatAction::ThrowingGrenade` and `Montage_Grenade_Throw` **already exist** — likely the closest of all Phase 8 systems to being wired already; verify before assuming it's unbuilt | 8 |
| Demo content migration/cleanup | `Common/Characters` (skeleton), `Common/Materials`, `Common/Audio`, `Common/AudioClasses` must be preserved; `Common/Core/Environment/Props/VFX` safe to delete after | Tracked as **Phase 6 in `infima_integration_plan.md`, confirmed NOT STARTED** (2026-07-11 Glob) | 9 |
| Custom weapon/character import pipeline | Guides 1–3 (Blender↔UE5, retargeting, left-hand clipping) | Reference procedure, not a one-time phase | 10 (reference) |

---

## Naming convention map

Useful when reading Infima's docs side-by-side with ShooterGame source — the same concept often has three names across Infima's Blueprint layer, ShooterGame's `WeaponConfig` legacy fields, and ShooterGame's current fields:

| Concept | Infima name | ShooterGame current name |
|---|---|---|
| Weapon data asset | `BP_TFA_BaseConfig` | `UWeaponConfig` |
| Weapon actor | `BP_TFA_BaseWeapon` | `AWeapon` |
| Character | `BP_TFA_BaseCharacter` | `AShooterGameCharacter` + `UCombatComponent` |
| FP AnimBP | `ABP_TFA_FP_BaseCharacter` | `ABP_FP_Default` (Blueprint) backed by `UShooterAnimInstanceBase` |
| TP AnimBP | `ABP_TFA_TP_BaseCharacter` | `ABP_TP_Default` (Blueprint) backed by `UShooterAnimInstanceBase` |
| Casing physics prop | `BP_TFA_PhysicsCasing` | `ACaseEject` (already built, native) |
| Grip enum | `E_TFA_GripAttachment` (None/Vertical/Angled) | `EWeaponGrip` (None/GripVertical/GripAngled) |
| Aim-cancel function | `Force Stop Aiming` (on `BP_TFA_BaseCharacter`) | **missing** — needs a public entry point into `UCombatComponent`'s aiming state |
| Grip-blend interface | `BPI_TFA_AnimationState` (Blueprint interface) | `IShooterAnimStateInterface` (native C++ interface — same role, correctly ported as native rather than BP) |
| Casing eject socket | `SOCKET_Eject` | `SocketCasingEject` (`FName` field on `UWeaponConfig`) — different literal name, same purpose; not a bug, just confirm intentional |
| Skeleton | `SKEL_TFA_Mannequin` | `SK_Mannequin` (UE5 standard Manny) |

---

## How to use this plan

1. Work phases roughly in order — later phases assume earlier ones are done (Phase 1's config cleanup in particular blocks everything else from being built on a stable field set).
2. Phase 2 (multiplayer/replication principles) is a **decision framework**, not a one-time task — apply its checklist every time a phase below adds a new piece of state.
3. Where a phase says "cross-reference `infima_integration_plan.md`," that file's checkboxes remain the source of truth for that specific sub-scope — don't duplicate tracking in two places.
4. Log any bug or design decision discovered while executing a phase as a GitHub Issue per `CLAUDE.md`'s workflow, not just a checkbox here.
