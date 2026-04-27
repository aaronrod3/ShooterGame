// ZombieObjectiveArea.cpp

#include "ZombieObjectiveArea.h"
#include "Components/SphereComponent.h"
#include "NavigationSystem.h"
#include "DrawDebugHelpers.h"
#include "ShooterGame/Enemy/Character/ZombieCharacter.h"

AZombieObjectiveArea::AZombieObjectiveArea()
{
    // Tick is needed only for ENABLE_DRAW_DEBUG visualization.
    // It is compiled out entirely in Shipping, so the cost is zero in production.
    PrimaryActorTick.bCanEverTick = true;

    // This actor is a server-side map marker — it does not need to replicate.
    // Any Blueprint or C++ that references it does so via a direct UPROPERTY reference
    // set in the editor, not through a networked lookup.
    bReplicates = false;

    ArrivalSphere = CreateDefaultSubobject<USphereComponent>(TEXT("ArrivalSphere"));
    SetRootComponent(ArrivalSphere);

    // Make the sphere non-colliding — it is purely a visual/query helper.
    ArrivalSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ArrivalSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    ArrivalSphere->SetSphereRadius(ArrivalRadius);

    // Show a translucent sphere in the editor viewport so designers can see the area.
    // This is the built-in UE editor brush color; it is not visible in PIE or builds.
    ArrivalSphere->ShapeColor = FColor(0, 200, 255);  // Cyan
}

// ─────────────────────────────────────────────────────────────────────────────
// Tick — #if ENABLE_DRAW_DEBUG visualization only
// ─────────────────────────────────────────────────────────────────────────────

void AZombieObjectiveArea::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Keep the sphere component radius in sync if the designer changed ArrivalRadius
    // via the Details panel at runtime (PIE).
    if (ArrivalSphere->GetUnscaledSphereRadius() != ArrivalRadius)
    {
        ArrivalSphere->SetSphereRadius(ArrivalRadius);
    }

#if ENABLE_DRAW_DEBUG
    // Cyan wire sphere — shows the arrival radius in PIE.
    // Drawn every tick with zero lifetime so it persists as a continuous overlay.
    DrawDebugSphere(
        GetWorld(),
        GetActorLocation(),
        ArrivalRadius,
        32,
        FColor(0, 200, 255),  // Cyan — matches ShapeColor above
        false,                 // bPersistentLines = false
        -1.f,                  // lifetime = -1 means "draw this frame only" (persists via Tick)
        0,
        1.5f                   // line thickness
    );

    // Label: name + radius value, floated slightly above the actor pivot.
    DrawDebugString(
        GetWorld(),
        GetActorLocation() + FVector(0.f, 0.f, ArrivalRadius + 40.f),
        FString::Printf(TEXT("[%s]\nRadius: %.0f cm"), *ObjectiveName, ArrivalRadius),
        nullptr,          // base actor (nullptr = world space)
        FColor::White,
        0.f,              // duration — 0 means current frame only (persists via Tick)
        true              // bDrawShadow
    );
    
    for (int32 i = RoutedZombies.Num() - 1; i >= 0; --i)
    {
        AZombieCharacter* Zombie = RoutedZombies[i];

        if (!IsValid(Zombie) || Zombie->GetZombieState() == EZombieState::EZS_Dead)
        {
            RoutedZombies.RemoveAt(i);
            continue;
        }

        const EZombieState State = Zombie->GetZombieState();
        const FColor MarkerColor = (State == EZombieState::EZS_Investigating)
            ? FColor::Magenta
            : FColor::Green;

        // Small sphere at zombie's feet
        DrawDebugSphere(
            GetWorld(),
            Zombie->GetActorLocation(),
            30.f,
            8,
            MarkerColor,
            false,
            -1.f
        );

        // Line from zombie to objective center so you can see routing at a glance
        DrawDebugLine(
            GetWorld(),
            Zombie->GetActorLocation() + FVector(0.f, 0.f, 40.f),
            GetActorLocation(),
            MarkerColor,
            false,
            -1.f,
            0,
            1.f
        );
    }
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// GetRandomPointInRadius
// ─────────────────────────────────────────────────────────────────────────────

bool AZombieObjectiveArea::GetRandomPointInRadius(FVector& OutLocation) const
{
    const UNavigationSystemV1* NavSystem =
        FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

    if (NavSystem)
    {
        FNavLocation NavLocation;
        const bool bFound = NavSystem->GetRandomReachablePointInRadius(
            GetActorLocation(), ArrivalRadius, NavLocation);

        if (bFound)
        {
            OutLocation = NavLocation.Location;
            return true;
        }
    }

    // Fallback: raw XY scatter within radius, preserve actor Z.
    // Mirrors the same fallback pattern used in AZombieSpawnManager::GetValidSpawnLocation().
    const FVector RandomOffset = FMath::VRand() * FMath::FRandRange(0.f, ArrivalRadius);
    OutLocation = GetActorLocation() + FVector(RandomOffset.X, RandomOffset.Y, 0.f);

    UE_LOG(LogTemp, Warning,
        TEXT("AZombieObjectiveArea [%s]: Nav query failed — using raw offset fallback."),
        *GetName());

    return false;
}


void AZombieObjectiveArea::RegisterRoutedZombies(const TArray<AZombieCharacter*>& Zombies)
{
#if ENABLE_DRAW_DEBUG
    for (AZombieCharacter* Zombie : Zombies)
    {
        if (IsValid(Zombie))
        {
            RoutedZombies.AddUnique(Zombie);
        }
    }
#endif
}