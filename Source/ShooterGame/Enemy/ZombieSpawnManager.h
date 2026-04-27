// ZombieSpawnManager.h
// Designer-placeable actor that spawns AZombieCharacter instances on the server.
// Supports Single, Group, and Random spawn modes.
// Tracks all spawned zombies in SpawnedZombies for external systems (waves, objectives, etc.)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShooterGame/Types/ZombieTypes.h"
#include "ZombieSpawnManager.generated.h"

// Forward declare to avoid circular includes — full include is in the .cpp
class AZombieCharacter;
class AZombieObjectiveArea; 

UCLASS()
class SHOOTERGAME_API AZombieSpawnManager : public AActor
{
    GENERATED_BODY()

public:
    AZombieSpawnManager();

    virtual void BeginPlay() override;

    // ── Public API ──────────────────────────────────────────────────────

    // Call from Blueprint or C++ (trigger volumes, mission objectives, wave managers, etc.)
    // Server-authoritative — silently returns on clients
    UFUNCTION(BlueprintCallable, Category = "Zombie|Spawn")
    void TriggerSpawn();

    // Fires a one-shot, server-authoritative mission spawn event.
    // Spawns Count zombies after an optional DelaySeconds.
    // This step only handles spawning + one-shot gating; objective routing is added in Step 3.
    UFUNCTION(BlueprintCallable, Category = "Zombie|Spawn")
    void FireObjectiveSpawnEvent(AZombieObjectiveArea* TargetArea, int32 Count, float DelaySeconds = 0.f);

    // Allows mission/wave systems to reset or inspect the one-shot guard between waves.
    UFUNCTION(BlueprintCallable, Category = "Zombie|Spawn")
    void SetObjectiveEventFired(bool bFired);

    UFUNCTION(BlueprintPure, Category = "Zombie|Spawn")
    bool IsObjectiveEventFired() const { return bObjectiveEventFired; }
    
    UFUNCTION(BlueprintCallable, Category = "Zombie|Spawn")
    void ResetForNextWave(bool bClearTrackedZombies = true);

    // Returns current count of alive tracked zombies from this spawner
    // Useful for wave managers / objective systems checking if a wave is cleared
    UFUNCTION(BlueprintCallable, Category = "Zombie|Spawn")
    int32 GetAliveZombieCount();

    // ── Designer-Facing Properties ───────────────────────────────────────

    // The zombie class to spawn — assign BP_ZombieCharacter (or any subclass) in Class Defaults
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Spawn")
    TSubclassOf<AZombieCharacter> ZombieClass;

    // Determines how many zombies are spawned per TriggerSpawn() call
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Spawn")
    ESpawnMode SpawnMode = ESpawnMode::ESM_Single;

    // Radius around this actor's location to scatter spawn points (uses nav mesh reachability)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Spawn", meta = (ClampMin = "100.0"))
    float SpawnRadius = 500.f;

    // Minimum zombies spawned — used by Group (exact count) and Random (lower bound)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Spawn", meta = (ClampMin = "1"))
    int32 MinCount = 1;

    // Maximum zombies spawned — used by Random mode upper bound (ignored by Single/Group)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Spawn", meta = (ClampMin = "1"))
    int32 MaxCount = 10;

    // If true, TriggerSpawn() is called automatically on BeginPlay (server only)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Spawn")
    bool bAutoSpawnOnBeginPlay = false;

    // Maximum number of alive zombies tracked by this spawner allowed at once.
    // 0 = no cap (unlimited). TriggerSpawn() early-outs if alive count >= this value.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Spawn", meta = (ClampMin = "0"))
    int32 MaxAliveAtOnce = 0;

    // ── Tracking ─────────────────────────────────────────────────────────

    // All zombies currently spawned by this manager.
    // Dead/invalid entries are purged at the start of each TriggerSpawn() call.
    // Read from external systems (wave managers, objective trackers) via GetAliveZombieCount().
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Zombie|Spawn|Debug")
    TArray<AZombieCharacter*> SpawnedZombies;

private:

    // ── Internal Helpers ──────────────────────────────────────────────────

    // Removes dead or invalid entries from SpawnedZombies
    void PurgeDeadZombies();

    // Attempts to find a reachable nav mesh point within SpawnRadius.
    // Returns true and sets OutLocation on success; falls back to a raw offset on failure.
    bool GetValidSpawnLocation(FVector& OutLocation) const;

    // Spawns a single zombie at a valid location — appends to SpawnedZombies on success
    void SpawnSingleZombie();

    // Spawns Count zombies by calling SpawnSingleZombie() Count times
    void SpawnGroup(int32 Count);
    
    void RouteZombiesToObjective(const TArray<AZombieCharacter*>& NewZombies, AZombieObjectiveArea* TargetArea);

    // Executes the delayed/immediate one-shot objective spawn after authority + guard checks.
    void ExecuteObjectiveSpawnEvent(int32 Count);

private:
    // True once the mission/objective spawn event has been consumed.
    // Resettable via SetObjectiveEventFired() so wave-based modes can reuse the same manager.
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Zombie|Spawn|Debug", meta = (AllowPrivateAccess = "true"))
    bool bObjectiveEventFired = false;

    // Stored only so delayed objective events can validate that a target area was provided.
    // Routing is added in Step 3.
    UPROPERTY()
    TObjectPtr<AZombieObjectiveArea> PendingObjectiveArea = nullptr;

    // Delay handle for one-shot mission/wave spawn events.
    FTimerHandle ObjectiveSpawnDelayHandle;
};