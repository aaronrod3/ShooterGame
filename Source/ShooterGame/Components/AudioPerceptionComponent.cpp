// Source/ShooterGame/Components/AudioPerceptionComponent.cpp

#include "AudioPerceptionComponent.h"
#include "GameFramework/Character.h"
#include "Perception/AISense_Hearing.h"
#include "Engine/World.h"
#include "Engine/HitResult.h"
#include "CollisionQueryParams.h"

UAudioPerceptionComponent::UAudioPerceptionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UAudioPerceptionComponent::BeginPlay()
{
    Super::BeginPlay();
}

// -----------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------

void UAudioPerceptionComponent::EmitSoundEvent(float OverrideRadius, float OverrideLoudness)
{
    if (!GetOwner()) return;

    const float Radius    = OverrideRadius   > 0.f ? OverrideRadius   : DefaultEmitRadius;
    const float InLoudness = OverrideLoudness > 0.f ? OverrideLoudness : Loudness;

    // Owning client routes through server RPC to keep AI reactions authoritative
    if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
    {
        Server_EmitSoundEvent(Radius, InLoudness);
    }
    else if (GetOwner()->HasAuthority())
    {
        // Direct server call (AI weapon, listen server local player)
        EmitSoundEvent_Internal(Radius, InLoudness);
    }
}

// -----------------------------------------------------------------------
// Internal — server-only emission
// -----------------------------------------------------------------------

void UAudioPerceptionComponent::EmitSoundEvent_Internal(float Radius, float InLoudness)
{
    if (!GetOwner()) return;
    if (!GetOwner()->HasAuthority()) return;
    
    // TEMP DEBUG
    UE_LOG(LogTemp, Warning, TEXT("DEBUG: EmitSoundEvent_Internal running on server — HasAuthority=TRUE"));

    UWorld* World = GetWorld();
    if (!World) return;

    const FVector EmitLocation = GetOwner()->GetActorLocation();
    float EffectiveRadius = Radius;

    if (bUseOcclusion)
    {
        const FVector OcclusionEnd = EmitLocation + FVector(0.f, 0.f, 200.f);
        if (!HasLineOfSight(EmitLocation, OcclusionEnd))
        {
            EffectiveRadius *= OcclusionRadiusReduction;
        }
    }

    // -----------------------------------------------------------------------
    // Resolve Instigator
    // UAISense_Hearing::ReportNoiseEvent requires a valid APawn instigator
    // or it silently drops the event.
    // -----------------------------------------------------------------------
    APawn* Instigator = nullptr;

    // Try 1: weapon's Owner was set to Character via EquipWeapon->SetOwner(Character)
    // AShooterGameCharacter inherits APawn so this cast will succeed when held
    if (AActor* OwnerOwner = GetOwner()->GetOwner())
    {
        // Cast to ACharacter first (covers AShooterGameCharacter → ACharacter → APawn)
        if (ACharacter* OwnerChar = Cast<ACharacter>(OwnerOwner))
        {
            Instigator = OwnerChar;
        }
        // Fallback to raw APawn cast
        else if (APawn* OwnerPawn = Cast<APawn>(OwnerOwner))
        {
            Instigator = OwnerPawn;
        }
    }
    // Try 2: weapon itself has an Instigator (set via SpawnActorDeferred or SpawnParams)
    else if (GetOwner()->GetInstigator())
    {
        Instigator = GetOwner()->GetInstigator();
    }
    // Try 3: owner IS a pawn (e.g. this component is on the character directly)
    else if (APawn* OwnerAsPawn = Cast<APawn>(GetOwner()))
    {
        Instigator = OwnerAsPawn;
    }

    if (!Instigator)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("UAudioPerceptionComponent: Could not resolve Instigator on %s — hearing event suppressed. Check SetOwner() is called at equip."),
            *GetOwner()->GetName());
        return; // Don't fire a broken event
    }

    UAISense_Hearing::ReportNoiseEvent(
        World,
        EmitLocation,
        InLoudness,
        Instigator,
        EffectiveRadius,
        TEXT("Default")
    );

    // TEMP — confirm ReportNoiseEvent was called
    UE_LOG(LogTemp, Warning,
        TEXT("[NOISE] ReportNoiseEvent called — Location:%s Radius:%.0f Loudness:%.2f Instigator:%s"),
        *EmitLocation.ToString(),
        EffectiveRadius,
        InLoudness,
        Instigator ? *Instigator->GetName() : TEXT("NULL"));
}

bool UAudioPerceptionComponent::HasLineOfSight(const FVector& Start, const FVector& End) const
{
    if (!GetWorld() || !GetOwner()) return true;

    FHitResult HitResult;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(GetOwner());

    const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
        HitResult, Start, End, ECC_Visibility, Params);

    return !bBlocked;
}

// -----------------------------------------------------------------------
// Server RPC
// -----------------------------------------------------------------------

void UAudioPerceptionComponent::Server_EmitSoundEvent_Implementation(float Radius, float InLoudness)
{
    EmitSoundEvent_Internal(Radius, InLoudness);
}

bool UAudioPerceptionComponent::Server_EmitSoundEvent_Validate(float Radius, float InLoudness)
{
    // Reject obviously hacked values
    return Radius > 0.f && Radius < 100000.f && InLoudness >= 0.f && InLoudness <= 1.f;
}