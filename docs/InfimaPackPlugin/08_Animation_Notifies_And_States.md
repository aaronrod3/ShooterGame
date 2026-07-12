# Step 8 — Animation Notifies and Notify States

> **The pack's own framing, stated directly:** *"These animation notifies and notify states are included for demonstration purposes. They show how to set up the animations correctly in your own project. The included code logic is made specifically for the demo setup. You'll need to modify it to fit your project and integrate it into your own systems."* Build these once the character/weapon Blueprints and both AnimBPs are working — they're the glue between montages and the gameplay logic already in place.

## Notify types and execution model

- **`AnimNotify`** — one-shot event at a single frame/time.
- **`AnimNotifyState`** — defines a time range with Begin, optional Tick, and End.
- Execution model: the montage/sequence advances time → Unreal gathers notifies for the current time slice → executes the notify Blueprint → the notify Blueprint talks back to the owning actor or active anim instance. Exact timing can vary during skipped time, section jumps, or blend-outs — treat notifies as best-effort gameplay signals, not fully authoritative state.
- **Important runtime note for notify *states* specifically:** if a montage jumps sections, blends out early, or gets interrupted, `End` may not fire exactly when expected — design any state driven by a notify state to be resilient to that (e.g. a fallback clear path).

---

## One-shot notifies (`AN_TFA_*`)

### Drop Magazine (`AN_TFA_DropMagazine`)
**Path:** `.../Common/Core/Animation/AN_TFA_DropMagazine`
Placed on the exact frame the current magazine should leave the weapon during a reload. **Weapon-owned** — only functions on a mesh owned by `BP_TFA_BaseWeapon`. Used in the demo specifically on **Reload Quick** and **Reload Empty** montages.

- **Flow:** `Received_Notify` → `MeshComp.GetOwner()` → cast to `BP_TFA_BaseWeapon` → `Weapon.SpawnDroppedMagazine(ImpulseForce, RotationForce)`. Cast failure = no-op.
- **Parameters:** `ImpulseForce` (default **`-50.0`** — negative is intentional, since the weapon applies impulse along the socket's up vector, so a negative value pushes the opposite direction), `RotationForce` (default **`150.0`**).
- **To modify:** edit `SpawnDroppedMagazine` inside `BP_TFA_BaseWeapon`.
- **Placement tips:** exact frame where the magazine clears the magwell; must be on the montage playing on the *weapon* mesh, not the character mesh.
- **Troubleshooting:** nothing happens → wrong mesh ownership, or the weapon's function exits early. Magazine drops but looks wrong → tune force values; verify `SocketMagazineAttachment`, `MeshMagazine`, `SoundCue_WEP_MagDrop`.

### Eject Casing (`AN_TFA_EjectCasing`)
**Path:** `.../Common/Core/Animation/AN_TFA_EjectCasing`
Used on firing/inspection/weapon-clearing animations. **Weapon-owned.** Uses socket **`SOCKET_Eject`** on the weapon skeleton for spawn location/rotation.

- **Flow:** builds a randomized `RotationOffset` from configured min/max rotators (`MakeRotator(Roll = RandomFloatInRange(MinRoll,MaxRoll), Pitch = ..., Yaw = ...)`), then calls `Weapon.EjectCasing(RotationOffset, MinEjectForce, MaxEjectForce, RotationSpeed)`.
- **Parameters:** `MinEjectForce`/`MaxEjectForce` (defaults **`50.0`**/**`65.0`**), `RotationSpeed` (default **`0.0`**), `MinEjectRotationOffset`/`MaxEjectRotationOffset` (default `(Pitch=0,Yaw=0,Roll=0)` both — no extra rotation offset until you set these). Keep each min axis ≤ its max axis.
- **To modify:** edit `EjectCasing` inside `BP_TFA_BaseWeapon`.
- **Placement tips:** frame where the bolt/eject motion looks correct; keep rotation offsets small (large offsets make the casing look like it teleported sideways).
- **Troubleshooting:** nothing happens → wrong mesh ownership, or `EjectCasing` exits early for your setup.

### Spawn Object Attached (`AN_TFA_SpawnObjectAttached`)
**Path:** `.../Common/Core/Animation/AN_TFA_SpawnObjectAttached`
Spawns a temporary prop and attaches it to a character socket. **Character-owned** — only works on a mesh owned by `BP_TFA_BaseCharacter`. Demo use case: the healing syringe. Docs explicitly note it "can also be used for things like grenade pins, flare caps, or similar props."

- **Flow:** delegates to `BP_TFA_BaseCharacter::SpawnObjectAttached(...)` (implemented in the composite graph `CGraph_SpawnObjectAttached`), which: builds a spawn transform from the socket transform + offsets → spawns with `AlwaysSpawn` → tags the actor `DisposableItem` → attaches to `Mesh` at `SpawnSocketName` (snap-to-target rules) → if the spawned actor has a `SkeletalMeshComponent`, hides it, optionally plays `AnimationToPlay`, waits `DelayBeforeVisible`, then shows it again if the actor still exists.
- **Parameters:** `ObjectToSpawn` (default `None` — nothing spawns if unset), `Animation` (optional, only matters for skeletal-mesh props), `SpawnSocketName` (default **`ik_hand_l`**), `LocationOffset`/`RotationOffset` (default `(0,0,0)` both), `VisibilityDelay` (default `0.0`).
- **To modify:** edit `CGraph Spawn Object Attached` inside `BP_TFA_BaseCharacter`.
- **Placement tips:** place at the frame the prop should appear; keep socket naming consistent across FP and TP character meshes (the character swaps meshes between perspectives). The **SnapToTarget** attach rule can override your location/rotation offsets after attach — fix the prop's pivot first if offsets don't seem to "stick."
- **Example — magazine/prop in the left hand during a reload:** `SpawnSocketName = ik_hand_l`, small `LocationOffset`/`RotationOffset` tuned to taste, `VisibilityDelay` ≈ `0.05–0.15` to match hand-contact timing. Pair with **Throw Physics Object** or manual destroy for cleanup.
- **Troubleshooting:** nothing happens → wrong mesh ownership or `ObjectToSpawn` is `None`. Wrong position → check socket + offsets. Duplicates on repeat → notify firing multiple times from loops/section jumps/replays; add explicit cleanup.

### Throw Physics Object (`AN_TFA_ThrowPhysicsObject`)
**Path:** `.../Common/Core/Animation/AN_TFA_ThrowPhysicsObject`
Detaches an attached prop and sends it into free physics. **Character-owned.** Demo use case: the healing animation — spawns the syringe cap during removal, and the physics syringe at the end. Docs note it's generically useful for "grenades or magazines" too.

- **Flow:** delegates to `BP_TFA_BaseCharacter::ThrowPhysicsObject(...)`, which spawns the notify-provided class at a socket-derived transform, finds a `PrimitiveComponent` on it, applies linear impulse (`bVelChange = true`) and angular impulse via a random axis (`bVelChange = true`), and — when `bClearSocketItem = true` — destroys previously-attached actors tagged `DisposableItem`. This is how the demo creates "the illusion that the same syringe gets thrown": it deletes the previously-spawned *static* prop right as the *physics* prop spawns in its place.
- **Parameters:** `ObjectToSpawn` (default `None`), `SpawnSocketName` (default **`ik_hand_l`**), `LocationOffset`/`RotationOffset` (default `(0,0,0)`), `ThrowForce` (default **`80.0`**), `ThrowRotationForce` (default **`80.0`**), `DestroySocketItem` (default **`true`**, forwarded as `bClearSocketItem`).
- **To modify:** edit `Throw Physics Object` inside `BP_TFA_BaseCharacter`.
- **Placement tips:** the exact frame the object should release; use `DestroySocketItem = true` whenever a previous notify spawned/attached a disposable prop; test timing at a few montage play rates since throws are very sensitive to small timing shifts.
- **Troubleshooting:** nothing happens → wrong mesh ownership, or `ObjectToSpawn` is `None`. Throw direction wrong → check whether the underlying function uses socket-forward or socket-up as its basis; bias with `RotationOffset`.

### Unlock Actions (`AN_TFA_UnlockActions`)
**Path:** `.../Common/Core/Animation/AN_TFA_UnlockActions`
Reopens gameplay input after a montage-driven lock window. **Character-owned.** Typical uses: reloads, equip/unequip, inspects.

- **Flow:** `Received_Notify` → `MeshComp.GetOwner()` → cast to `BP_TFA_BaseCharacter` → `Character.bIsBusy = false`. That's the entire mechanism — no other logic.
- **Placement:** at the point the character's left hand returns to the weapon, so control returns at the visually correct moment — not just "near the end" generically.
- **Troubleshooting:** player stays locked → confirm this notify is reached on *every* montage branch/section path; check for other gates besides `bIsBusy` (ADS blocks, sprint restrictions).

---

## Notify states (`ANS_TFA_*`)

### Block ADS (`ANS_TFA_BlockADS`)
**Path:** `.../Common/Core/Animation/ANS_TFA_BlockADS`
Disables aiming down sights for the notify state's active duration. **Character-owned.** Used for animations that either have no aiming variation, or look broken while aiming.

- **Begin:** cast to `BP_TFA_BaseCharacter` → `Character.bIsAimingBlocked = true` → **`Character.ForceStopAiming()`** (a real gameplay-side call, not merely a cosmetic flag — it both blocks ADS and forces an immediate exit from an in-progress aim).
- **End:** cast to `BP_TFA_BaseCharacter` → `Character.bIsAimingBlocked = false`.
- **Placement tips:** cover the full time range where ADS would look wrong; keep the window slightly conservative since players notice visual flicker quickly.
- **Troubleshooting:** ADS still works → confirm mesh ownership and that your ADS logic actually checks `bIsAimingBlocked`. ADS never returns → confirm every montage path reaches `NotifyEnd`; add a fallback clear for cancelled/interrupted montages.

### Hide Main Mag (`ANS_TFA_HideMainMag`)
**Path:** `.../Common/Core/Animation/ANS_TFA_HideMainMag`
Hides the weapon's main magazine mesh during a time window. **Weapon-owned.** Typically paired with spawning/attaching a hand-held magazine prop, so the player sees a clean handoff instead of overlapping magazines.

- **Begin:** cast to `BP_TFA_BaseWeapon` → `Weapon.SetMagazineVisibility(bVisible = false, bIsReserve = false)`.
- **End:** `Weapon.SetMagazineVisibility(bVisible = true, bIsReserve = false)`.
- **No exposed parameters** — always targets the main magazine.
- **Placement tips:** start slightly before the magazine should visually leave the weapon; end after the replacement magazine is seated.
- **Troubleshooting:** nothing happens → wrong mesh ownership. Wrong magazine hides → confirm you used this notify and not Show Reserve Mag.

### Show Reserve Mag (`ANS_TFA_ShowReserveMag`)
**Path:** `.../Common/Core/Animation/ANS_TFA_ShowReserveMag`
Same mechanism as Hide Main Mag, but targets the reserve magazine. **Weapon-owned.**

- **Begin:** `Weapon.SetMagazineVisibility(bVisible = true, bIsReserve = true)`.
- **End:** `Weapon.SetMagazineVisibility(bVisible = false, bIsReserve = true)`.
- **Why two separate classes exist rather than one parameterized class:** main and reserve magazines attach at *different sockets* (`SocketMagazineAttachment` vs. `SocketMagazineReserveAttachment`), and visibility toggling is keyed to which socket a component is attached to.
- **Placement tips:** start when the off-hand reaches for the reserve magazine; end once it's no longer needed visually.

### Left Hand Grip (`ANS_TFA_LeftHandGrip`)
**Path:** `.../Common/Core/Animation/ANS_TFA_LeftHandGrip`
"Detaches" the left hand from the weapon during action animations (reloads, inspects, interactions) — matters most with grip attachments, since a reload playing on top of the wrong hand pose without this can look off and misalign with the magazine. **Requires the active anim instance to implement `BPI_TFA_AnimationState`.**

- **Begin:** `MeshComp.GetAnimInstance()` → `AnimInstance.UpdateLeftHandGrip(IsLeftHandOnWeapon = false, BlendSpeed = BlendOutSpeed)`.
- **End:** `AnimInstance.UpdateLeftHandGrip(IsLeftHandOnWeapon = true, BlendSpeed = BlendReturnSpeed)`.
- **Parameters:** `BlendOutSpeed` (default **`15.0`**), `BlendReturnSpeed` (default **`15.0`**).
- **Mechanism, in full:** grip poses apply via a **Layered Blend Per Bone** node (covering the left arm including the left-hand IK bone). While active, alpha = `0`; on end, alpha returns to `1`, snapping the hand back to the custom grip pose. This means only **one** pose animation is needed per grip variant, while still reusing animations authored against a different default hand pose. `CurrentGripAlpha` is recalculated **every frame** via a custom event, **`Interpolate Grip Alpha`**, interpolating based on `GripBlendSpeed` and `bIsLeftHandOnWeapon` (both sourced from `BPI_TFA_AnimationState`).
- **Where the values actually live:** both FP and TP AnimBPs implement `BPI_TFA_AnimationState` in Class Settings; the boolean and blend-speed values are modified *inside* the AnimBPs, both routed through the same interface function.
- **Placement tips:** span the *entire* time the left hand is off the weapon, with a little padding at start/end to smooth it; tune blend speed for a snappier or slower detach/reattach feel.
- **Troubleshooting:** no effect → confirm the mesh's anim instance implements `BPI_TFA_AnimationState`; confirm `UpdateLeftHandGrip` actually drives IK/grip placement inside the AnimBP.

---

## Animation State Interface (`BPI_TFA_AnimationState`)

**Path:** `.../Common/Core/Animation/BPI_TFA_AnimationState`

Why it exists: animation notifies run inside the animation system, not the gameplay actor directly — an interface gives a notify state a clean way to call into the owner/anim instance without hard-coupling to one concrete Blueprint class.

**Function:** `UpdateLeftHandGrip(bool IsLeftHandOnWeapon, float BlendSpeed)` — called by **Left Hand Grip**, sent to `MeshComp.GetAnimInstance()`. The implementation target is your AnimBP (or custom AnimInstance) — the interface asset itself is only the contract; the real grip-blending logic lives wherever you implement it. If a notify appears to fire but nothing changes visually, confirm your AnimBP actually implements this interface.

---

## Checklist

- [ ] Implement all 5 one-shot notifies (`AN_TFA_DropMagazine`, `AN_TFA_EjectCasing`, `AN_TFA_SpawnObjectAttached`, `AN_TFA_ThrowPhysicsObject`, `AN_TFA_UnlockActions`).
- [ ] Implement all 4 notify states (`ANS_TFA_BlockADS`, `ANS_TFA_HideMainMag`, `ANS_TFA_ShowReserveMag`, `ANS_TFA_LeftHandGrip`).
- [ ] Confirm both FP and TP AnimBPs implement `BPI_TFA_AnimationState` in Class Settings.
- [ ] Place `AN_TFA_DropMagazine`/`AN_TFA_EjectCasing` only on montages playing on the weapon mesh; place `AN_TFA_SpawnObjectAttached`/`AN_TFA_ThrowPhysicsObject`/`AN_TFA_UnlockActions`/`ANS_TFA_BlockADS` only on montages playing on the character mesh.
- [ ] Verify `SOCKET_Eject` exists on the weapon skeleton before relying on `AN_TFA_EjectCasing`.
- [ ] PIE-test a full reload: main mag hides, magazine drops with physics, reserve mag shows, left hand detaches/reattaches cleanly, action unlocks at the right frame.
