# Step 11 — Official Guide: Fix Left-Hand Clipping

Direct follow-on from [10_Custom_Character_And_Weapon_Import.md](10_Custom_Character_And_Weapon_Import.md) — the pack's own docs describe left-hand clipping as *"a super common issue when replacing existing weapon models,"* since a new model's proportions rarely match the original exactly.

**Prerequisite:** character mesh rigged to UE5 Manny with matching bone hierarchy (same as the custom-character guide).

## The one rule that matters most

> **Adjust the IK bone, not the hand bone directly.** For left-hand placement: edit **`ik_hand_l`** — **not** `hand_l`.

**Why:** the demo automatically attaches hands to the weapon via **FABRIK IK** for stability and to avoid jitter. Adjusting the raw hand bone directly fights against the IK solve instead of cooperating with it — any hand-placement fix must happen at the IK bone, which is what the FABRIK effector actually targets.

## Steps

1. **Locate the original idle pose** to use as an edit base: `InfimaGames/TacticalFPSAnimations/Weapons/WeaponName/Animations/Character/FP/Poses`.
2. Select **`ik_hand_l`**, nudge it (typically on the X axis) to pull the hand off the clipping mesh.
   - **Known editor quirk:** the pose preview does **not** update live while adjusting the bone — expect to iterate blind and re-check, rather than tuning interactively.
3. **Save as a new pose asset** — don't overwrite the original: **Create Asset → Create Animation → Current Pose**. Naming convention: **`A_FP_Idle_Pose_Custom`**. Keep custom poses in their own folder.
4. **Test via the weapon's data asset:** find the **FP Idle Pose** field, swap in the new custom pose, Play in the demo map to see the result immediately.
5. **Scope limit, stated directly by the pack's own docs:** *"This is a quick fix, and it's not meant for huge pose changes."* For a substantially different custom grip, either author a full new pose set from scratch, or build a more advanced IK setup capable of dynamically changing left-hand position (neither is covered by this guide).

## How the demo's left-hand grip and IK setup actually works

Understanding this is *why* editing `ik_hand_l` (not `hand_l`) is correct:

- **Pose selection (per grip/attachment):** each grip/attachment has its own custom pose; the demo swaps between them **via an enum switch** based on which grip is currently active.
- **Blending (so reloads don't visibly "pop"):** the left-hand pose is layered on top of base locomotion/montages using a **Layered Blend Per Bone** node, applied to the left arm including the left-hand IK bone. Blend weight is driven in real time by the montage notify state **Left Hand Grip** (`ANS_TFA_LeftHandGrip`) — blending out during reloads and back in near the end, avoiding an obvious pop between poses.
- **IK attachment (FABRIK):** hands attach to the weapon via FABRIK nodes; the IK bones control final placement.
- **Where to look directly:** **First Person Base Character AnimBP** (`ABP_TFA_FP_BaseCharacter`) and **Third Person Base Character AnimBP** (`ABP_TFA_TP_BaseCharacter`), both at `InfimaGames/TacticalFPSAnimations/Common/Core/Characters` — see [06_FirstPerson_AnimBP.md](06_FirstPerson_AnimBP.md) and [07_ThirdPerson_AnimBP.md](07_ThirdPerson_AnimBP.md).

## Checklist

- [ ] Confirm the character mesh is Manny-rigged before attempting this fix.
- [ ] Edit `ik_hand_l`, never `hand_l`, for any left-hand placement adjustment.
- [ ] Save the adjusted pose as a new asset, never overwriting the original.
- [ ] Test via the weapon data asset's FP Idle Pose field in the demo map.
- [ ] If the fix isn't enough (grip is substantially different), escalate to authoring a full new pose set rather than continuing to nudge this one bone.
