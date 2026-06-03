// Source/ShooterGame/Components/HitZoneComponent.cpp

#include "HitZoneComponent.h"

UHitZoneComponent::UHitZoneComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(false);

    // -----------------------------------------------------------------------
    // Default bone → zone mapping.
    //
    // Bone names below match the UE5 Mannequin skeleton used by the default
    // player and most marketplace zombie assets. Verify against your specific
    // Skeleton Asset in the Skeleton Tree panel (see Piece 6 editor steps).
    //
    // To override for a specific character:
    //   1. Open the character Blueprint
    //   2. Select HitZoneComponent in the Components panel
    //   3. Find HitZoneConfig → BoneToZone in the Details panel
    //   4. Add, remove, or remap entries without recompiling
    // -----------------------------------------------------------------------

    // --- Head ---
    HitZoneConfig.BoneToZone.Add(TEXT("head"),          EHitZone::EHZ_Head);

    // --- Neck ---
    HitZoneConfig.BoneToZone.Add(TEXT("neck_01"),       EHitZone::EHZ_Neck);
    HitZoneConfig.BoneToZone.Add(TEXT("neck_02"),       EHitZone::EHZ_Neck);   // present on some rigs

    // --- Torso ---
    HitZoneConfig.BoneToZone.Add(TEXT("spine_01"),      EHitZone::EHZ_Torso);
    HitZoneConfig.BoneToZone.Add(TEXT("spine_02"),      EHitZone::EHZ_Torso);
    HitZoneConfig.BoneToZone.Add(TEXT("spine_03"),      EHitZone::EHZ_Torso);
    HitZoneConfig.BoneToZone.Add(TEXT("spine_04"),      EHitZone::EHZ_Torso);  // present on some rigs
    HitZoneConfig.BoneToZone.Add(TEXT("clavicle_l"),    EHitZone::EHZ_Torso);
    HitZoneConfig.BoneToZone.Add(TEXT("clavicle_r"),    EHitZone::EHZ_Torso);
    HitZoneConfig.BoneToZone.Add(TEXT("pelvis"),        EHitZone::EHZ_Torso);

    // --- Arms ---
    HitZoneConfig.BoneToZone.Add(TEXT("upperarm_l"),    EHitZone::EHZ_Arm);
    HitZoneConfig.BoneToZone.Add(TEXT("lowerarm_l"),    EHitZone::EHZ_Arm);
    HitZoneConfig.BoneToZone.Add(TEXT("hand_l"),        EHitZone::EHZ_Arm);
    HitZoneConfig.BoneToZone.Add(TEXT("upperarm_r"),    EHitZone::EHZ_Arm);
    HitZoneConfig.BoneToZone.Add(TEXT("lowerarm_r"),    EHitZone::EHZ_Arm);
    HitZoneConfig.BoneToZone.Add(TEXT("hand_r"),        EHitZone::EHZ_Arm);

    // --- Legs ---
    HitZoneConfig.BoneToZone.Add(TEXT("thigh_l"),       EHitZone::EHZ_Leg);
    HitZoneConfig.BoneToZone.Add(TEXT("calf_l"),        EHitZone::EHZ_Leg);
    HitZoneConfig.BoneToZone.Add(TEXT("foot_l"),        EHitZone::EHZ_Leg);
    HitZoneConfig.BoneToZone.Add(TEXT("ball_l"),        EHitZone::EHZ_Leg);
    HitZoneConfig.BoneToZone.Add(TEXT("thigh_r"),       EHitZone::EHZ_Leg);
    HitZoneConfig.BoneToZone.Add(TEXT("calf_r"),        EHitZone::EHZ_Leg);
    HitZoneConfig.BoneToZone.Add(TEXT("foot_r"),        EHitZone::EHZ_Leg);
    HitZoneConfig.BoneToZone.Add(TEXT("ball_r"),        EHitZone::EHZ_Leg);

    // -----------------------------------------------------------------------
    // Default multipliers.
    //
    // Head: 4.0 — a 25-damage pistol round kills a 100 HP zombie in one headshot.
    //             A 30-damage rifle round kills with significant overkill (120 dmg)
    //             which is intentional — rifles should feel decisive on headshots.
    //
    // Neck: 2.5 — near-lethal. A 30-damage rifle = 75 dmg; requires 2 pistol shots.
    //             Neck shots with AP/battle rifle rounds should still one-tap —
    //             that is enforced by weapon BaseDamage, not by raising this value.
    //
    // Torso/Arm/Leg: carried through ApplyPointDamage so hit reaction intensity
    //             can scale with damage in future. Nonlethal kill path on zombies
    //             is enforced in AZombieCharacter::TakeDamage (Piece 3), not here.
    //
    // Default: 1.0 — unmapped bones do unmodified damage. Safe fallback.
    // -----------------------------------------------------------------------
    HitZoneConfig.Multipliers.Add(EHitZone::EHZ_Head,    4.0f);
    HitZoneConfig.Multipliers.Add(EHitZone::EHZ_Neck,    2.5f);
    HitZoneConfig.Multipliers.Add(EHitZone::EHZ_Torso,   1.0f);
    HitZoneConfig.Multipliers.Add(EHitZone::EHZ_Arm,     0.75f);
    HitZoneConfig.Multipliers.Add(EHitZone::EHZ_Leg,     0.75f);
    HitZoneConfig.Multipliers.Add(EHitZone::EHZ_Default, 1.0f);
}

float UHitZoneComponent::GetDamageMultiplier(FName BoneName) const
{
    return HitZoneConfig.GetMultiplierForBone(BoneName);
}

EHitZone UHitZoneComponent::GetHitZone(FName BoneName) const
{
    return HitZoneConfig.GetZoneForBone(BoneName);
}

bool UHitZoneComponent::IsLethalZone(FName BoneName) const
{
    return HitZoneConfig.IsLethalZone(BoneName);
}