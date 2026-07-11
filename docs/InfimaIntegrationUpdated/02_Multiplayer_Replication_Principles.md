# Phase 2 — Multiplayer & Replication Principles

**This phase is a decision framework, not a one-time task.** Apply its checklist every time a later phase adds new state or a new spawned actor. Front-loading it here (before Weapon Actor Assembly, Physics Props, or Notify classes) means those phases can just say "apply Phase 2" instead of re-deriving the same reasoning three times.

---

## The finding that makes this phase necessary

Searched the entire Infima documentation set (Blueprints reference, Animation reference, all 5 Guides, full Web Docs/FAQ) for any mention of multiplayer, replication, networking, RPCs, dedicated server, or listen server. There is exactly **one relevant sentence in the whole doc set**, from the FAQ's "Demo Logic" section:

> **"No, the demo logic is not replicated."**

No elaboration, no caveats, no "here's how you'd add it." That's the entirety of Infima's networking guidance. Every replication decision below is ShooterGame's own responsibility — there is no upstream pattern to lean on, only `CLAUDE.md`'s own established convention:

> Every stateful system uses: `UPROPERTY(ReplicatedUsing=OnRep_X)` + server-only `Server_X()` mutator + `OnRep_X()` re-broadcasting a `BlueprintAssignable` delegate. Never poll replicated arrays/structs directly — always bind to the `On*Changed` delegate.

---

## What ShooterGame already got right (confirmed 2026-07-11)

`UCombatComponent` and `AWeapon` already apply this pattern correctly to the systems that matter most for gameplay-affecting state:

- `EquippedWeapon` — `ReplicatedUsing = OnRep_EquippedWeapon`
- `bAiming` — `ReplicatedUsing = OnRep_Aiming`
- `CurrentCombatAction` — `ReplicatedUsing = OnRep_CombatAction`
- `bHighReady` — `ReplicatedUsing = OnRep_HighReady`
- `AWeapon::WeaponState` — `ReplicatedUsing = OnRep_WeaponState`
- `AWeapon::bRoundChambered` / `ReplicatedMagState` — `ReplicatedUsing = OnRep_LoadState`
- `AWeapon::bHasSuppressor` — `ReplicatedUsing = OnRep_SuppressorState`
- Full `Server_X()` RPC set already exists: `ServerSetAiming`, `ServerFire`, `ServerReload`, `ServerPlayDryFire`, `ServerCycleFireMode`, `ServerToggleSuppressor`
- Cosmetic broadcast already uses `NetMulticast`: `MulticastFire`, `MulticastReload`, `MulticastFinishReload`

This is a mature, correct foundation. Nothing in this phase asks you to retrofit any of the above — it's here so later phases can point at it as the template to copy.

---

## The one flagged gap: `CurrentGrip`

`UCombatComponent::CurrentGrip` (`EWeaponGrip`) has **no `Replicated`/`ReplicatedUsing` tag** — the only piece of combat-visible state that doesn't follow the pattern above.

**Before "fixing" this, check whether it needs fixing at all.** `CurrentGrip` is set from `WeaponConfig.WeaponGrip` on equip, and `WeaponConfig` is a data asset — every client that has the same cooked build already has the same `WeaponGrip` value for a given weapon, because it's not per-instance runtime state, it's baked content. Since `EquippedWeapon` (which carries a reference to that config) *is* already replicated, every client can already derive the correct grip pose without `CurrentGrip` itself needing to replicate.

- [ ] Confirm no runtime "cycle grip attachment" player action exists (unlike Infima's `RandomizeGripAttachment()`, ShooterGame appears to set grip once per equip from config, not toggle it live). If true, `CurrentGrip` is fine as-is — leave it unreplicated and note why in a comment, rather than blindly adding `ReplicatedUsing`.
- [ ] If a future feature lets a player swap grip attachments mid-session (not just per weapon-equip), `CurrentGrip` **will** need the standard `ReplicatedUsing = OnRep_CurrentGrip` + `Server_SetGrip()` treatment at that point — flag this as a design trigger, not something to build preemptively.

---

## Decision framework for every new system in this plan

For each new piece of state or spawned actor introduced in later phases, ask:

1. **Does it affect gameplay outcome or something every client must agree on** (damage, ammo count, whether a magazine is actually loaded)? → Full replicated-state pattern: `Replicated`/`ReplicatedUsing` + server-authoritative mutator.
2. **Is it purely cosmetic and derivable from already-replicated state** (which grip pose to show, given an already-replicated equipped weapon)? → No new replication needed; derive it client-side from what's already synced.
3. **Is it a one-shot cosmetic event with no ongoing state to keep in sync** (a casing ejecting, a magazine dropping, a muzzle flash)? → `NetMulticast` fire-and-forget, exactly like the existing `MulticastFire`/`MulticastReload` pattern. Each client spawns/plays its own local cosmetic actor/VFX; no need to replicate the spawned actor's ongoing transform if it's non-gameplay-affecting (a casing bouncing on the floor doesn't need to look pixel-identical on every client).

### Applied to this plan's upcoming systems

| System | Category | Recommended treatment |
|---|---|---|
| Magazine cosmetic visibility (hide main/show reserve) | (3) one-shot-ish, tied to reload | Extend the existing `MulticastReload`/`MulticastFinishReload` calls to also drive `SetMagazineVisibility()` locally on every client — don't add new replicated bools |
| Dropped magazine physics prop spawn | (3) one-shot cosmetic | `NetMulticast` spawn, exactly like a `MulticastFire`-style event; each client spawns its own local `ACaseEject`-pattern actor, no ongoing replication of its physics simulation needed |
| Casing ejection (`ACaseEject`) | (3) one-shot cosmetic | Same — multicast trigger, client-local spawn. `ACaseEject` already exists; only the multicast trigger point is new |
| Laser attachment (new actor) | (2) derivable from equipped weapon + config | If every client can determine "this weapon has a laser" purely from the already-replicated `EquippedWeapon`'s config, spawn it locally per-client in the same `BeginPlay`/equip-response path already used for other attachment meshes — no new replicated state |
| `ForceStopAiming()` (Phase 5) | (1) gameplay-affecting | Must route through the server-authoritative aiming path — call (or add) a public entry point that ultimately reaches the same place `ServerSetAiming(false)` does, not just a local anim-side flag |
| Melee/Healing/Interaction/Malfunction actions (Phase 8) | (1) gameplay-affecting | Each needs the full pattern: a new or reused `ECombatAction` value pushed through the already-existing `CurrentCombatAction`/`OnRep_CombatAction` replication, with server-authoritative validation before triggering (damage application, heal amount, etc.) |

---

## Checklist

- [ ] Before building any new actor/notify class in Phases 3–5 and 8, classify it using the 3-category framework above and write the classification into that phase's own notes.
- [ ] Confirm the `CurrentGrip` non-replication is intentional (per the reasoning above) or add proper replication if a grip-cycling feature is ever planned.
- [ ] Extend `MulticastReload`/`MulticastFinishReload` to drive magazine cosmetic visibility once that system exists (Phase 3/4).
- [ ] For every new spawned cosmetic actor (dropped magazine, casing via `ACaseEject`, laser attachment), confirm the spawn is either (a) purely client-local and derivable from replicated state, or (b) an explicit `NetMulticast` trigger — never a spawn that only happens on one client without one of these two justifications.

## Exit criteria

There is no "done" state for this phase in the traditional sense — it's a standing checklist referenced by Phases 3, 4, 5, and 8. Treat it as complete once every system introduced by this plan has an explicit, written replication classification (even if that classification is "none needed, derived from X").
