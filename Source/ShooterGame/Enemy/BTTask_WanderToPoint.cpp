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