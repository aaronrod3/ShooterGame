// ZombieObjectiveArea.h
// Placeable level actor that marks a map location for mission-driven zombie routing.
// Mission designers drop this in the level per-objective.
//
// AZombieSpawnManager::FireObjectiveSpawnEvent() references this actor to route
// newly spawned zombies toward the area. Once zombies arrive inside ArrivalRadius,
// they resume normal perception-driven behavior (wander, chase, etc.) automatically.
//
// Server-only logic: this actor itself carries no replication — it is a static
// map marker. All routing decisions that reference it execute on the server.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShooterGame/Enemy/Character/ZombieCharacter.h"
#include "ZombieObjectiveArea.generated.h"

class AZombieCharacter;
class USphereComponent;

UCLASS()
class SHOOTERGAME_API AZombieObjectiveArea : public AActor
{
    GENERATED_BODY()

public:
    AZombieObjectiveArea();

    virtual void Tick(float DeltaTime) override;

    // ── Public API ──────────────────────────────────────────────────────────

    // Returns a random nav-projected point inside ArrivalRadius.
    // Used by AZombieSpawnManager to assign each routed zombie a unique destination
    // so they don't all converge on the exact same point.
    // Returns false and fills OutLocation with a raw offset fallback if nav query fails.
    UFUNCTION(BlueprintCallable, Category = "Zombie|Objective")
    bool GetRandomPointInRadius(FVector& OutLocation) const;

    // ── Designer-Facing Properties ────────────────────────────────────────

    // Radius (cm) within which arriving zombies are considered "at the objective"
    // and resume normal perception-driven behavior.
    // Also used as the scatter radius when assigning unique destinations to each zombie.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Objective", meta = (ClampMin = "100.0"))
    float ArrivalRadius = 400.f;

    // Optional friendly name shown in debug visualization and logs.
    // Set this in the editor per-instance so you can tell objectives apart in PIE.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Objective")
    FString ObjectiveName = TEXT("Objective");
    
    
    UFUNCTION(BlueprintCallable, Category = "Zombie|Objective")
    void RegisterRoutedZombies(const TArray<AZombieCharacter*>& Zombies);

private:
    // Root sphere component — drives the editor visualizer radius in the viewport.
    // ArrivalRadius is the authoritative value; the sphere is kept in sync on Tick
    // so resizing ArrivalRadius in the Details panel immediately updates the preview.
    UPROPERTY(VisibleAnywhere, Category = "Zombie|Objective")
    USphereComponent* ArrivalSphere;
    
    UPROPERTY()
    TArray<TObjectPtr<AZombieCharacter>> RoutedZombies;
};