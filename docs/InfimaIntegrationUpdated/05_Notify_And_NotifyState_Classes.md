# Phase 5 — Animation Notify & Notify-State Classes

**Relationship to existing plan:** `docs/infima_integration_plan.md` Phase 5 already tracks this exact scope (class list, default parameter values, status per class) and should remain the authoritative checklist for it — don't fork a second tracker here. This phase exists only to record **new, confirmed detail from the Guides** that changes how one of those classes should be implemented, and to sequence the remaining classes against Phases 3–4's now-real weapon functions.

---

## The one finding that changes an existing decision: `ForceStopAiming()`

`infima_integration_plan.md` Phase 5 flags `ForceStopAiming()` as a genuinely missing function and frames the choice as open: either implement it for real, or decide the existing anim-side-only suppression (`bIsAimingBlockedLocal` on `UShooterAnimInstanceBase`) was always sufficient by design.

**The Guides resolve this.** `ANS_TFA_BlockADS`'s own documentation states its mechanism in plain terms:

> "It casts to Base Character (`BP_TFA_BaseCharacter`) and executes **Force Stop Aiming**, which blocks aiming input for the duration of the notify state."

This is a real gameplay-side function call in Infima's own reference implementation, not a cosmetic anim-shadow flag. The intended design is: **when this notify state begins, it must actually cancel an in-progress aim at the authoritative level**, not just suppress the visual pose.

**Implication for ShooterGame:** implement `ForceStopAiming()` for real. It needs a public entry point that reaches the same place `UCombatComponent::ServerSetAiming(false)` does — `SetAiming(bool)` is currently a protected method, so either:
- Add a new public `UCombatComponent::ForceStopAiming()` that internally calls the same path `SetAiming(false)` would (respecting the existing server-authoritative RPC chain — don't bypass it with a client-only flag flip), or
- Expose the existing aiming-cancel path through `AShooterGameCharacter` (whichever your character/component boundary conventions prefer — check how other cross-component calls like `SetCombatAction` are exposed today and match that pattern).

Classify per [02_Multiplayer_Replication_Principles.md](02_Multiplayer_Replication_Principles.md): this is gameplay-affecting (category 1) — it must route through the server-authoritative aiming state, not just flip `bIsAimingBlockedLocal` on the local AnimInstance. The existing `bIsAimingBlockedLocal` shadow flag can and should stay for its documented purpose (a local, per-montage-window anim-side gate), but `ANS_BlockADS::NotifyBegin` should call the real `ForceStopAiming()` in addition to setting that shadow flag, matching Infima's own dual-purpose design (block *and* force-exit).

---

## Remaining classes to implement

Cross-reference `infima_integration_plan.md` Phase 5 for the full parameter/default-value tables — only implementation notes that depend on *this* plan's other phases are repeated here.

| Class | Depends on | Notes |
|---|---|---|
| `AN_DropMagazine` | Phase 3 (`AWeapon::SpawnDroppedMagazine`), Phase 4 (dropped magazine actor) | Used by Infima specifically on **Reload Quick** and **Reload Empty** montages — not every reload variant. Confirm which of ShooterGame's reload montage variants should carry this notify before blanket-adding it to all of them |
| `AN_EjectCasing` | Phase 3 (`AWeapon::EjectCasing`), Phase 4 (`ACaseEject`, already built) | Nearly free once Phase 3/4 land — this notify is a thin cast-and-forward |
| `AN_SpawnObjectAttached` | Phase 8 (Healing) | Infima's own demo use case is the healing syringe, but its docs explicitly note it "can also be used for things like grenade pins, flare caps, or similar props" — don't over-specialize the implementation to syringes only |
| `AN_ThrowPhysicsObject` | Phase 8 (Healing), the `DisposableItem` tag convention | Infima's demo mechanism is worth copying exactly: destroy the previously-spawned *static* prop (tagged `DisposableItem`) at the same instant a new *physics* prop spawns in its place — "creating the illusion that the same object gets thrown" without ever animating a mesh transitioning from kinematic to physics-simulated |
| `AN_UnlockActions` | none | Simplest class — casts to the character, sets `bIsBusy = false`. Placement matters more than logic: Infima's guidance is to place it "where the character's left hand returns to the weapon," not just "near the end" |
| `ANS_HideMainMag` / `ANS_ShowReserveMag` | Phase 3/4 (`AWeapon::SetMagazineVisibility`, persistent magazine actors) | Confirmed via the Guides: these are two separate classes (not one parameterized class) specifically because main and reserve magazines attach at **different sockets** (`SocketMagazineAttachment` vs `SocketMagazineReserveAttachment`), and visibility toggling is keyed off which socket a component is attached to, not off a shared "is this the reserve" boolean baked into a single class |

---

## Checklist

- [ ] Implement `UCombatComponent::ForceStopAiming()` (or equivalent public entry point) as a real, server-routed aim-cancel — not a cosmetic flag.
- [ ] Update `ANS_BlockADS::NotifyBegin` to call it, in addition to the existing `bIsAimingBlockedLocal` shadow-flag set.
- [ ] Implement `AN_DropMagazine`, scoped to the correct reload montage variants (Quick/Empty, matching Infima's own placement choice, unless a design reason argues otherwise).
- [ ] Implement `AN_EjectCasing` once Phase 3/4 land.
- [ ] Implement `AN_SpawnObjectAttached` and `AN_ThrowPhysicsObject`, generalized (not syringe-only), for use by Phase 8.
- [ ] Implement `AN_UnlockActions`.
- [ ] Implement `ANS_HideMainMag` and `ANS_ShowReserveMag` once Phase 3/4's magazine visibility system exists.
- [ ] Update `infima_integration_plan.md` Phase 5's own checkboxes as each class lands — this file should not become a second source of truth for status tracking.

## Exit criteria

- All 5 `AN_*` and remaining 2 `ANS_*` classes exist and compile clean.
- `ForceStopAiming()` demonstrably cancels an in-progress ADS when a montage carrying `ANS_BlockADS` begins, verified in PIE, including on a non-owning client (i.e. actually routes through the server).
