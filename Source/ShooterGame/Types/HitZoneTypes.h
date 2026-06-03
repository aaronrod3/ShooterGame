// Source/ShooterGame/Types/HitZoneTypes.h
//
// Defines EHitZone and FHitZoneConfig — the type layer for the Phase 1B hit zone system.
//
// Consumers:
//   UHitZoneComponent (Piece 2)  — owns FHitZoneConfig, exposes zone lookup
//   CombatComponent  (Piece 4)  — calls GetZoneForBone() after server trace
//   AZombieCharacter (Piece 3)  — branches TakeDamage on EHitZone value
//   AShooterGameCharacter       — receives scaled point damage via Multipliers

#pragma once

#include "CoreMinimal.h"
#include "HitZoneTypes.generated.h"


// ---------------------------------------------------------------------------
// Body region enum.
//
// Values are ordered coarse-to-fine for readability; the switch blocks in
// AZombieCharacter::TakeDamage follow this ordering.
//
// Expansion notes (do not act on in Phase 1B):
//   - EHZ_Hand  : for glove/gauntlet armor coverage
//   - EHZ_Foot  : for boot armor or bear-trap mechanics
//   - EHZ_Spine : for special infected weak point
// ---------------------------------------------------------------------------
UENUM(BlueprintType)
enum class EHitZone : uint8
{
    EHZ_Head    UMETA(DisplayName = "Head"),     // Lethal zone for zombies; multiplied for players
    EHZ_Neck    UMETA(DisplayName = "Neck"),     // Near-head — high multiplier, lethal on most ammo
    EHZ_Torso   UMETA(DisplayName = "Torso"),    // Nonlethal on zombies; staggers + interrupts attacks
    EHZ_Arm     UMETA(DisplayName = "Arm"),      // Nonlethal on zombies; disables grab/swing
    EHZ_Leg     UMETA(DisplayName = "Leg"),      // Nonlethal on zombies; impairs or drops mobility
    EHZ_Default UMETA(DisplayName = "Default"),  // Fallback: unmapped bone — no reaction modifier

    EHZ_MAX     UMETA(Hidden)
};


// ---------------------------------------------------------------------------
// Per-character hit zone configuration.
// Owned by UHitZoneComponent (Piece 2).
//
// BoneToZone:
//   Maps skeleton bone names → EHitZone region.
//   Populated with defaults in UHitZoneComponent's constructor.
//   Both zombie and player Blueprint subclasses can override the mapping
//   in the Details panel without recompiling.
//
// Multipliers:
//   Per-zone float scalar applied to raw weapon damage before ApplyPointDamage.
//
//   For ZOMBIES:
//     Head/Neck multipliers are used to ensure headshots reach a lethal
//     threshold regardless of base weapon damage.
//     Non-head multipliers (Torso, Arm, Leg) are carried through the damage
//     call but AZombieCharacter::TakeDamage ignores them for kill evaluation
//     — body shots are unconditionally nonlethal on zombies by design.
//     The multipliers still matter if you later want hit reaction intensity
//     to scale with damage (e.g., heavier torso hit = longer stagger duration).
//
//   For PLAYERS:
//     All multipliers apply normally — players can be killed by any hit zone.
//
// Armor integration path (Phase 2+):
//   UHitZoneComponent::GetDamageMultiplier() will check whether a zone is
//   covered by armor before returning from the Multipliers map.
//   No structural changes to this struct are needed for that expansion.
// ---------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct FHitZoneConfig
{
    GENERATED_BODY()

    // Bone name → hit zone region.
    // Key:   exact bone name from the Skeleton Asset
    //        (use the Skeleton Tree in the Physics Asset Editor to verify names)
    // Value: EHitZone region that bone belongs to
    //
    // Bones absent from this map resolve to EHZ_Default.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitZone|Mapping")
    TMap<FName, EHitZone> BoneToZone;

    // Per-zone damage multiplier.
    // Key:   EHitZone
    // Value: scalar multiplied against raw weapon damage on the server
    //
    // Phase 1B suggested defaults (set in UHitZoneComponent constructor):
    //   EHZ_Head    -> 4.0f   (100 HP zombie dies to a 25-damage pistol headshot)
    //   EHZ_Neck    -> 2.5f   (near-lethal; guarantees death from rifle rounds)
    //   EHZ_Torso   -> 1.0f   (full damage recorded but nonlethal on zombies)
    //   EHZ_Arm     -> 0.75f  (reduced — arm mass absorbs some energy)
    //   EHZ_Leg     -> 0.75f  (reduced — leg mass absorbs some energy)
    //   EHZ_Default -> 1.0f   (no modifier for unmapped bones)
    //
    // Tune freely in the Blueprint Details panel; no recompile needed.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitZone|Multipliers")
    TMap<EHitZone, float> Multipliers;

    // -----------------------------------------------------------------------
    // Returns the scaled damage value for a given bone name.
    // Called by UHitZoneComponent::GetDamageMultiplier() — do not call
    // directly from weapon or combat code; always go through the component.
    //
    // Thread safety: read-only, safe to call from GameThread TakeDamage.
    // -----------------------------------------------------------------------
    float GetMultiplierForBone(FName BoneName) const
    {
        const EHitZone* FoundZone = BoneToZone.Find(BoneName);
        const EHitZone  Zone      = FoundZone ? *FoundZone : EHitZone::EHZ_Default;

        const float* FoundMult = Multipliers.Find(Zone);
        return FoundMult ? *FoundMult : 1.0f;
    }

    // -----------------------------------------------------------------------
    // Returns the EHitZone for a given bone name.
    // Used by AZombieCharacter::TakeDamage to determine reaction branch
    // (kill / stagger / leg impair / arm disable) independently of the
    // damage multiplier value.
    // -----------------------------------------------------------------------
    EHitZone GetZoneForBone(FName BoneName) const
    {
        const EHitZone* Found = BoneToZone.Find(BoneName);
        return Found ? *Found : EHitZone::EHZ_Default;
    }

    // -----------------------------------------------------------------------
    // Convenience: true if BoneName maps to a lethal zone (Head or Neck).
    // Used in AZombieCharacter::TakeDamage to decide whether to allow
    // the kill path or route to a nonlethal reaction.
    // Does NOT evaluate ammo type or damage threshold — that check is in
    // AZombieCharacter::HandleZombieHeadshot (Piece 3).
    // -----------------------------------------------------------------------
    bool IsLethalZone(FName BoneName) const
    {
        const EHitZone Zone = GetZoneForBone(BoneName);
        return Zone == EHitZone::EHZ_Head || Zone == EHitZone::EHZ_Neck;
    }
};