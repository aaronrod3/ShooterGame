// ZombieSpawnManager.cpp

#include "ZombieSpawnManager.h"
#include "ShooterGame/Enemy/Character/ZombieCharacter.h"
#include "ZombieObjectiveArea.h" 
#include "ShooterGame/Enemy/Character/ZombieAIController.h"
#include "NavigationSystem.h"
#include "DrawDebugHelpers.h"
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
// FireObjectiveSpawnEvent — one-shot, server-authoritative mission event API
// This step only handles delayed/immediate spawning and one-shot gating.
// Objective routing is added in Step 3.
// ─────────────────────────────────────────────────────────────────────────────

void AZombieSpawnManager::FireObjectiveSpawnEvent(AZombieObjectiveArea* TargetArea, int32 Count, float DelaySeconds)
{
    // Server-authoritative — clients should never reach spawning logic
    if (!HasAuthority())
    {
        return;
    }

    if (bObjectiveEventFired)
    {
        UE_LOG(LogTemp, Log,
            TEXT("AZombieSpawnManager [%s]: Objective spawn event already fired. Ignoring duplicate call."),
            *GetName());
        return;
    }

    if (!IsValid(TargetArea))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("AZombieSpawnManager [%s]: FireObjectiveSpawnEvent called with invalid TargetArea."),
            *GetName());
        return;
    }

    if (Count <= 0)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("AZombieSpawnManager [%s]: FireObjectiveSpawnEvent called with invalid Count (%d)."),
            *GetName(), Count);
        return;
    }

    PendingObjectiveArea = TargetArea;
    bObjectiveEventFired = true;

    GetWorldTimerManager().ClearTimer(ObjectiveSpawnDelayHandle);

    if (DelaySeconds > 0.f)
    {
        GetWorldTimerManager().SetTimer(
            ObjectiveSpawnDelayHandle,
            FTimerDelegate::CreateWeakLambda(this, [this, Count]()
            {
                if (!IsValid(PendingObjectiveArea))
                {
                    UE_LOG(LogTemp, Warning,
                        TEXT("AZombieSpawnManager [%s]: Delayed objective spawn aborted — PendingObjectiveArea became invalid during delay. Resetting fired flag."),
                        *GetName());

                    bObjectiveEventFired = false;
                    PendingObjectiveArea = nullptr;
                    return;
                }

                ExecuteObjectiveSpawnEvent(Count);
            }),
            DelaySeconds,
            false
        );

        UE_LOG(LogTemp, Log,
            TEXT("AZombieSpawnManager [%s]: Objective spawn event scheduled in %.2fs for %d zombies."),
            *GetName(), DelaySeconds, Count);
    }
    else
    {
        ExecuteObjectiveSpawnEvent(Count);
    }
}

void AZombieSpawnManager::SetObjectiveEventFired(bool bFired)
{
    if (!HasAuthority())
    {
        return;
    }

    bObjectiveEventFired = bFired;

    if (!bObjectiveEventFired)
    {
        PendingObjectiveArea = nullptr;
        GetWorldTimerManager().ClearTimer(ObjectiveSpawnDelayHandle);
    }

    UE_LOG(LogTemp, Log,
        TEXT("AZombieSpawnManager [%s]: Objective event fired flag set to %s."),
        *GetName(),
        bObjectiveEventFired ? TEXT("TRUE") : TEXT("FALSE"));
}


void AZombieSpawnManager::ResetForNextWave(bool bClearTrackedZombies)
{
    if (!HasAuthority())
    {
        return;
    }

    // Clear the one-shot guard so FireObjectiveSpawnEvent can fire again
    bObjectiveEventFired = false;

    // Cancel any delay timer that hasn't fired yet — prevents a stale delayed
    // spawn from a previous wave firing unexpectedly into the next wave
    GetWorldTimerManager().ClearTimer(ObjectiveSpawnDelayHandle);

    // Clear the stored objective area reference — the next wave will pass a
    // fresh one via FireObjectiveSpawnEvent
    PendingObjectiveArea = nullptr;

    if (bClearTrackedZombies)
    {
        // Purge dead entries first so we only clear genuinely alive zombies
        PurgeDeadZombies();
        SpawnedZombies.Empty();

        UE_LOG(LogTemp, Log,
            TEXT("AZombieSpawnManager [%s]: ResetForNextWave — guard cleared, timer cancelled, SpawnedZombies emptied."),
            *GetName());
    }
    else
    {
        UE_LOG(LogTemp, Log,
            TEXT("AZombieSpawnManager [%s]: ResetForNextWave — guard cleared, timer cancelled, SpawnedZombies preserved (%d alive)."),
            *GetName(), SpawnedZombies.Num());
    }
}


void AZombieSpawnManager::ExecuteObjectiveSpawnEvent(int32 Count)
{
    if (!HasAuthority())
    {
        return;
    }

    if (!ZombieClass)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("AZombieSpawnManager [%s]: ZombieClass is not set. Assign BP_ZombieCharacter in Class Defaults."),
            *GetName());
        return;
    }

    if (!IsValid(PendingObjectiveArea))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("AZombieSpawnManager [%s]: Objective spawn event aborted — PendingObjectiveArea is invalid."),
            *GetName());
        return;
    }

    // Purge dead/invalid entries before checking the alive cap
    PurgeDeadZombies();

    // Respect the same cap logic as TriggerSpawn()
    if (MaxAliveAtOnce > 0 && SpawnedZombies.Num() >= MaxAliveAtOnce)
    {
        UE_LOG(LogTemp, Log,
            TEXT("AZombieSpawnManager [%s]: MaxAliveAtOnce cap (%d) reached. Skipping objective spawn event."),
            *GetName(), MaxAliveAtOnce);
        return;
    }

    // Snapshot the count before spawning so we can identify which zombies are new.
    // SpawnGroup() appends to SpawnedZombies, so everything from PreSpawnCount onward
    // is a zombie that was just created by this event.
    const int32 PreSpawnCount = SpawnedZombies.Num();

    SpawnGroup(Count);

    // Build the list of newly spawned zombies only — do not re-route zombies that
    // were already alive and tracking their own targets before this event fired.
    TArray<AZombieCharacter*> NewlySpawned;
    for (int32 i = PreSpawnCount; i < SpawnedZombies.Num(); ++i)
    {
        if (IsValid(SpawnedZombies[i]))
        {
            NewlySpawned.Add(SpawnedZombies[i]);
        }
    }

    if (NewlySpawned.Num() > 0)
    {
        RouteZombiesToObjective(NewlySpawned, PendingObjectiveArea);
    }

    UE_LOG(LogTemp, Log,
        TEXT("AZombieSpawnManager [%s]: Objective spawn event fired — %d zombies spawned and routed to [%s]."),
        *GetName(), NewlySpawned.Num(), *PendingObjectiveArea->GetName());
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


void AZombieSpawnManager::RouteZombiesToObjective(
    const TArray<AZombieCharacter*>& NewZombies,
    AZombieObjectiveArea* TargetArea)
{
    if (!IsValid(TargetArea)) return;

    for (AZombieCharacter* Zombie : NewZombies)
    {
        if (!IsValid(Zombie)) continue;

        // Each zombie gets its own random point inside ArrivalRadius so the
        // horde scatters across the objective instead of stacking on one spot.
        FVector Destination;
        TargetArea->GetRandomPointInRadius(Destination);

        // The controller drives all state + blackboard writes — never touch
        // the blackboard directly from outside the controller.
        AZombieAIController* ZombieController =
            Cast<AZombieAIController>(Zombie->GetController());

        if (!ZombieController)
        {
            // Controller may not be possessed yet if this fires on the same
            // frame as SpawnGroup(). Retry next frame via a short timer.
            FTimerHandle RetryHandle;
            GetWorldTimerManager().SetTimer(
                RetryHandle,
                FTimerDelegate::CreateWeakLambda(this, [this, Zombie, TargetArea]()
                {
                    if (!IsValid(Zombie) || !IsValid(TargetArea)) return;

                    AZombieAIController* RetryController =
                        Cast<AZombieAIController>(Zombie->GetController());

                    if (!RetryController)
                    {
                        UE_LOG(LogTemp, Warning,
                            TEXT("AZombieSpawnManager [%s]: Zombie [%s] still has no controller on retry — skipping route."),
                            *GetName(), *Zombie->GetName());
                        return;
                    }

                    FVector RetryDestination;
                    TargetArea->GetRandomPointInRadius(RetryDestination);

                    RetryController->SetObjectiveDestination(RetryDestination);

                    UE_LOG(LogTemp, Log,
                        TEXT("AZombieSpawnManager [%s]: Zombie [%s] routed to objective on retry — dest: %s"),
                        *GetName(), *Zombie->GetName(), *RetryDestination.ToString());
                }),
                0.1f,   // 1-frame delay at 10Hz — gives possession time to complete
                false
            );

            UE_LOG(LogTemp, Warning,
                TEXT("AZombieSpawnManager [%s]: Zombie [%s] controller not ready — scheduled retry in 0.1s."),
                *GetName(), *Zombie->GetName());
            continue;
        }

        ZombieController->SetObjectiveDestination(Destination);

        UE_LOG(LogTemp, Log,
            TEXT("AZombieSpawnManager [%s]: Zombie [%s] routed to objective — dest: %s"),
            *GetName(), *Zombie->GetName(), *Destination.ToString());
    }
    
    // Register zombies with the objective area for debug visualization.
    // No-op in non-debug builds — RegisterRoutedZombies is compiled out.
#if ENABLE_DRAW_DEBUG
    TargetArea->RegisterRoutedZombies(NewZombies);

    // Draw a brief yellow sphere at this spawner's location so you can see
    // where the objective event originated from during PIE debugging.
    DrawDebugSphere(
        GetWorld(),
        GetActorLocation(),
        SpawnRadius,
        24,
        FColor::Yellow,
        false,
        5.f,    // persists for 5 seconds then disappears
        0,
        2.f
    );
#endif
}


