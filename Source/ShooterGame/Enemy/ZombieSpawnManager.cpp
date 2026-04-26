// ZombieSpawnManager.cpp

#include "ZombieSpawnManager.h"
#include "ZombieCharacter.h"
#include "NavigationSystem.h"
#include "ShooterGame/Types/ZombieTypes.h"

AZombieSpawnManager::AZombieSpawnManager()
{
    PrimaryActorTick.bCanEverTick = false;

    // This actor itself does not need to replicate — it is a server-only manager.
    // The zombies it spawns already have bReplicates = true on AZombieCharacter.
    bReplicates = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// BeginPlay
// ─────────────────────────────────────────────────────────────────────────────

void AZombieSpawnManager::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority() && bAutoSpawnOnBeginPlay)
    {
        TriggerSpawn();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// TriggerSpawn — public entry point, callable from Blueprint or C++
// ─────────────────────────────────────────────────────────────────────────────

void AZombieSpawnManager::TriggerSpawn()
{
    // Server-authoritative — clients should never reach spawning logic
    if (!HasAuthority())
    {
        return;
    }

    if (!ZombieClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("AZombieSpawnManager [%s]: ZombieClass is not set. Assign BP_ZombieCharacter in Class Defaults."), *GetName());
        return;
    }

    // Purge dead/invalid entries before checking the alive cap
    PurgeDeadZombies();

    // Enforce MaxAliveAtOnce cap — 0 means no cap
    if (MaxAliveAtOnce > 0 && SpawnedZombies.Num() >= MaxAliveAtOnce)
    {
        UE_LOG(LogTemp, Log, TEXT("AZombieSpawnManager [%s]: MaxAliveAtOnce cap (%d) reached. Skipping spawn."), *GetName(), MaxAliveAtOnce);
        return;
    }

    // Branch on spawn mode
    switch (SpawnMode)
    {
        case ESpawnMode::ESM_Single:
            SpawnSingleZombie();
            break;

        case ESpawnMode::ESM_Group:
            SpawnGroup(FMath::Max(1, MinCount));
            break;

        case ESpawnMode::ESM_Random:
        {
            // Re-roll count every trigger call for replayability
            const int32 RandomCount = FMath::RandRange(
                FMath::Max(1, MinCount),
                FMath::Max(1, MaxCount)
            );
            SpawnGroup(RandomCount);
            break;
        }

        default:
            break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// GetAliveZombieCount — callable from Blueprint for wave/objective systems
// ─────────────────────────────────────────────────────────────────────────────

int32 AZombieSpawnManager::GetAliveZombieCount()
{
    PurgeDeadZombies();
    return SpawnedZombies.Num();
}

// ─────────────────────────────────────────────────────────────────────────────
// PurgeDeadZombies — removes invalid or dead entries from SpawnedZombies
// ─────────────────────────────────────────────────────────────────────────────

void AZombieSpawnManager::PurgeDeadZombies()
{
    SpawnedZombies.RemoveAll([](AZombieCharacter* Zombie)
    {
        // Remove if the pointer is invalid or the zombie has reached the Dead state
        return !IsValid(Zombie) || Zombie->GetZombieState() == EZombieState::EZS_Dead;
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// GetValidSpawnLocation — nav mesh reachable point, with raw offset fallback
// ─────────────────────────────────────────────────────────────────────────────

bool AZombieSpawnManager::GetValidSpawnLocation(FVector& OutLocation) const
{
    const UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

    if (NavSystem)
    {
        FNavLocation NavLocation;
        const bool bFound = NavSystem->GetRandomReachablePointInRadius(GetActorLocation(), SpawnRadius, NavLocation);

        if (bFound)
        {
            OutLocation = NavLocation.Location;
            return true;
        }
    }

    // Fallback: scatter randomly within radius in the XY plane, keep Z of spawner
    // Used when nav mesh is not built or point query fails
    const FVector RandomOffset = FMath::VRand() * FMath::FRandRange(0.f, SpawnRadius);
    OutLocation = GetActorLocation() + FVector(RandomOffset.X, RandomOffset.Y, 0.f);

    UE_LOG(LogTemp, Warning, TEXT("AZombieSpawnManager [%s]: Nav mesh query failed — using raw offset fallback for spawn location."), *GetName());
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// SpawnSingleZombie
// ─────────────────────────────────────────────────────────────────────────────

void AZombieSpawnManager::SpawnSingleZombie()
{
    FVector SpawnLocation;
    GetValidSpawnLocation(SpawnLocation);

    // Spawn slightly above the surface to avoid geometry clipping on uneven terrain
    SpawnLocation.Z += 50.f;

    const FRotator SpawnRotation = FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f);

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AZombieCharacter* NewZombie = GetWorld()->SpawnActor<AZombieCharacter>(
        ZombieClass,
        SpawnLocation,
        SpawnRotation,
        SpawnParams
    );

    if (NewZombie)
    {
        SpawnedZombies.Add(NewZombie);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("AZombieSpawnManager [%s]: SpawnActor failed at location %s."), *GetName(), *SpawnLocation.ToString());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// SpawnGroup — calls SpawnSingleZombie Count times
// Respects MaxAliveAtOnce cap mid-loop so a large Group/Random count
// does not blow past the cap when some slots are already occupied
// ─────────────────────────────────────────────────────────────────────────────

void AZombieSpawnManager::SpawnGroup(int32 Count)
{
    for (int32 i = 0; i < Count; ++i)
    {
        // Re-check cap each iteration in case MaxAliveAtOnce is tight
        if (MaxAliveAtOnce > 0 && SpawnedZombies.Num() >= MaxAliveAtOnce)
        {
            UE_LOG(LogTemp, Log, TEXT("AZombieSpawnManager [%s]: Hit MaxAliveAtOnce cap mid-group at %d/%d zombies."), *GetName(), i, Count);
            break;
        }

        SpawnSingleZombie();
    }
}