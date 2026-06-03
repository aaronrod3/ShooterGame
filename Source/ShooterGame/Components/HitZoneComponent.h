// Source/ShooterGame/Components/HitZoneComponent.h
//
// Lightweight ActorComponent that owns a character's FHitZoneConfig.
// Added to AZombieCharacter and AShooterGameCharacter (Piece 2 integration step).
//
// Server usage only — all callers (CombatComponent::ServerFire,
// AZombieCharacter::TakeDamage) run on the server.
// The component itself holds no replicated state.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShooterGame/Types/HitZoneTypes.h"
#include "HitZoneComponent.generated.h"


UCLASS(ClassGroup = "ShooterGame", meta = (BlueprintSpawnableComponent),
       DisplayName = "Hit Zone Component")
class SHOOTERGAME_API UHitZoneComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    UHitZoneComponent();

    // -----------------------------------------------------------------------
    // Configuration — editable per character Blueprint subclass.
    // Defaults are populated in the constructor for both zombie and player
    // skeletons using UE5 Mannequin-compatible bone names.
    // Override BoneToZone in any BP subclass whose skeleton uses different
    // bone naming conventions (e.g., custom zombie rigs).
    // -----------------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitZone")
    FHitZoneConfig HitZoneConfig;

    // -----------------------------------------------------------------------
    // Primary server-side query interface.
    //
    // GetDamageMultiplier:
    //   Returns the damage scalar for the given bone name.
    //   Called from CombatComponent::ServerFire_Implementation before
    //   UGameplayStatics::ApplyPointDamage.
    //   Returns 1.0f if the bone is not mapped (safe default).
    //
    // GetHitZone:
    //   Returns the EHitZone for the given bone name.
    //   Called from AZombieCharacter::TakeDamage to branch kill vs. reaction.
    //   Returns EHZ_Default if the bone is not mapped.
    //
    // IsLethalZone:
    //   Convenience wrapper — true if bone maps to Head or Neck.
    //   Called from AZombieCharacter::TakeDamage to guard the kill path.
    // -----------------------------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "HitZone")
    float GetDamageMultiplier(FName BoneName) const;

    UFUNCTION(BlueprintCallable, Category = "HitZone")
    EHitZone GetHitZone(FName BoneName) const;

    UFUNCTION(BlueprintCallable, Category = "HitZone")
    bool IsLethalZone(FName BoneName) const;
};