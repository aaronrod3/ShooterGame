// BTTask_WanderToPoint.cpp

#include "BTTask_WanderToPoint.h"
#include "ZombieAIController.h"
#include "ZombieCharacter.h"
#include "NavigationSystem.h"
#include "AITypes.h"
#include "Navigation/PathFollowingComponent.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "DrawDebugHelpers.h"

UBTTask_WanderToPoint::UBTTask_WanderToPoint()
{
    NodeName = TEXT("Wander To Point");
    bCreateNodeInstance = true;
    bNotifyTaskFinished = true;
}

EBTNodeResult::Type UBTTask_WanderToPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AZombieAIController* ZombieController = Cast<AZombieAIController>(OwnerComp.GetAIOwner());
    if (!ZombieController) return EBTNodeResult::Failed;

    AZombieCharacter* ZombieOwner = Cast<AZombieCharacter>(ZombieController->GetPawn());
    if (!ZombieOwner) return EBTNodeResult::Failed;

    if (ZombieOwner->GetZombieState() != EZombieState::EZS_Wandering)
    {
        UE_LOG(LogTemp, Warning, TEXT("[WANDER] ExecuteTask — not in Wandering state, aborting"));
        return EBTNodeResult::Failed;
    }

    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(ZombieOwner->GetWorld());
    if (!NavSys)
    {
        UE_LOG(LogTemp, Warning, TEXT("[WANDER] No NavigationSystem found"));
        return EBTNodeResult::Failed;
    }

    FNavLocation RandomLocation;
    const float Radius = ZombieOwner->GetZombieConfig().WanderRadius;
    const bool bFound = NavSys->GetRandomReachablePointInRadius(
        ZombieOwner->GetActorLocation(), Radius, RandomLocation);

    if (!bFound)
    {
        UE_LOG(LogTemp, Warning, TEXT("[WANDER] GetRandomReachablePointInRadius failed"));
        return EBTNodeResult::Failed;
    }
    
    // --- Horde Cohesion ---
    const FZombieConfig& Config = ZombieOwner->GetZombieConfig();
    bool bCohesionApplied = false;  // tracked for debug visualization below
    FVector DebugCentroid = FVector::ZeroVector;

    if (Config.HordeCohesionRadius > 0.f)
    {
        TArray<AActor*> OverlappedActors;
        TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
        ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));

        UKismetSystemLibrary::SphereOverlapActors(
            ZombieOwner->GetWorld(),
            ZombieOwner->GetActorLocation(),
            Config.HordeCohesionRadius,
            ObjectTypes,
            AZombieCharacter::StaticClass(),
            { ZombieOwner },
            OverlappedActors
        );

        if (OverlappedActors.Num() >= Config.MinCohesionNeighbors)
        {
            FVector HordeCentroid = FVector::ZeroVector;
            for (AActor* Neighbor : OverlappedActors)
            {
                HordeCentroid += Neighbor->GetActorLocation();
            }
            HordeCentroid /= static_cast<float>(OverlappedActors.Num());

            const float DistFromCentroid = FVector::Dist(RandomLocation.Location, HordeCentroid);
            if (DistFromCentroid > Config.HordeCohesionRadius)
            {
                RandomLocation.Location = FMath::Lerp(
                    RandomLocation.Location,
                    HordeCentroid,
                    Config.CohesionBiasFactor
                );

                bCohesionApplied = true;
                DebugCentroid = HordeCentroid;

                UE_LOG(LogTemp, Warning,
                    TEXT("[WANDER] Cohesion bias applied — %d neighbors | Centroid: %s | Biased dest: %s"),
                    OverlappedActors.Num(),
                    *HordeCentroid.ToString(),
                    *RandomLocation.Location.ToString()
                );
            }
        }
    }
    // --- End Horde Cohesion ---

    // --- Debug Visualization (stripped from shipping builds automatically) ---
        #if ENABLE_DRAW_DEBUG
            if (Config.HordeCohesionRadius > 0.f)
            {
                const float DebugLifetime = 2.5f;

                // Cohesion radius ring:
                //   Green  = cohesion bias fired this tick
                //   Yellow = neighbors found but destination was already inside radius
                //   White  = no qualifying neighbors
                DrawDebugSphere(
                    ZombieOwner->GetWorld(),
                    ZombieOwner->GetActorLocation(),
                    Config.HordeCohesionRadius,
                    32,
                    bCohesionApplied ? FColor::Green : FColor::White,
                    false,
                    DebugLifetime
                );

                // When cohesion fired: draw the centroid and a line from this zombie to it
                if (bCohesionApplied)
                {
                    // Centroid marker
                    DrawDebugSphere(
                        ZombieOwner->GetWorld(),
                        DebugCentroid,
                        40.f,
                        12,
                        FColor::Orange,
                        false,
                        DebugLifetime
                    );

                    // Line from zombie to centroid
                    DrawDebugLine(
                        ZombieOwner->GetWorld(),
                        ZombieOwner->GetActorLocation(),
                        DebugCentroid,
                        FColor::Orange,
                        false,
                        DebugLifetime,
                        0,
                        2.f    // line thickness
                    );

                    // Line from zombie to biased destination
                    DrawDebugLine(
                        ZombieOwner->GetWorld(),
                        ZombieOwner->GetActorLocation(),
                        RandomLocation.Location,
                        FColor::Cyan,
                        false,
                        DebugLifetime,
                        0,
                        2.f
                    );
                }
            }
        #endif
    // --- End Debug Visualization ---
    
    
    

    FAIMoveRequest MoveRequest;
    MoveRequest.SetGoalLocation(RandomLocation.Location);
    MoveRequest.SetAcceptanceRadius(50.f);
    MoveRequest.SetAllowPartialPath(false);
    MoveRequest.SetProjectGoalLocation(true);

    FPathFollowingRequestResult Result = ZombieController->MoveTo(MoveRequest);

    if (Result.Code == EPathFollowingRequestResult::Failed)
    {
        UE_LOG(LogTemp, Warning, TEXT("[WANDER] MoveTo failed immediately"));
        return EBTNodeResult::Failed;
    }

    if (Result.Code == EPathFollowingRequestResult::AlreadyAtGoal)
    {
        ZombieController->StartIdleDwell();
        UE_LOG(LogTemp, Warning, TEXT("[WANDER] Already at goal — starting idle dwell"));
        return EBTNodeResult::Succeeded;
    }

    // Movement started — bind to path following delegate to get arrival callback
    MoveRequestID = Result.MoveId;
    
    // Cache refs for use inside the delegate callback
    OwnerController = ZombieController;
    BTCompRef = &OwnerComp;

    UPathFollowingComponent* PathComp = ZombieController->GetPathFollowingComponent();
    if (PathComp)
    {
        // Store BT component ref for use inside the lambda
        UBehaviorTreeComponent* BTComp = &OwnerComp;

        PathComp->OnRequestFinished.AddUObject(this, &UBTTask_WanderToPoint::OnMoveCompleted);
        UE_LOG(LogTemp, Warning, TEXT("[WANDER] MoveTo started — destination: %s | Radius: %.0f"),
            *RandomLocation.Location.ToString(), Radius);
    }

    return EBTNodeResult::InProgress;
}

void UBTTask_WanderToPoint::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
    // Only handle our specific move request
    if (RequestID != MoveRequestID) return;

    // Unbind immediately so we don't double-fire
    if (OwnerController.IsValid())
    {
        UPathFollowingComponent* PathComp = OwnerController->GetPathFollowingComponent();
        if (PathComp)
        {
            PathComp->OnRequestFinished.RemoveAll(this);
        }

        if (Result.IsSuccess())
        {
            AZombieAIController* ZombieController = Cast<AZombieAIController>(OwnerController.Get());
            if (ZombieController)
            {
                ZombieController->StartIdleDwell();
                UE_LOG(LogTemp, Warning, TEXT("[WANDER] Arrived at destination — starting idle dwell"));
            }
            FinishLatentTask(*BTCompRef, EBTNodeResult::Succeeded);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[WANDER] Move failed or was interrupted — BT will retry"));
            FinishLatentTask(*BTCompRef, EBTNodeResult::Failed);
        }
    }
}

void UBTTask_WanderToPoint::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
    // Clean up delegate binding if task was aborted externally
    AZombieAIController* ZombieController = Cast<AZombieAIController>(OwnerComp.GetAIOwner());
    if (ZombieController)
    {
        UPathFollowingComponent* PathComp = ZombieController->GetPathFollowingComponent();
        if (PathComp)
        {
            PathComp->OnRequestFinished.RemoveAll(this);
        }
    }

    Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

EBTNodeResult::Type UBTTask_WanderToPoint::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AZombieAIController* ZombieController = Cast<AZombieAIController>(OwnerComp.GetAIOwner());
    if (ZombieController)
    {
        UPathFollowingComponent* PathComp = ZombieController->GetPathFollowingComponent();
        if (PathComp)
        {
            PathComp->OnRequestFinished.RemoveAll(this);
        }
        ZombieController->StopMovement();
        UE_LOG(LogTemp, Warning, TEXT("[WANDER] AbortTask — movement stopped by higher priority branch"));
    }

    return Super::AbortTask(OwnerComp, NodeMemory);
}