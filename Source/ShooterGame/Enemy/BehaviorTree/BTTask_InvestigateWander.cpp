// BTTask_InvestigateWander.cpp

#include "BTTask_InvestigateWander.h"
#include "ShooterGame/Enemy/Character/ZombieAIController.h"
#include "ShooterGame/Enemy/Character/ZombieCharacter.h"
#include "NavigationSystem.h"
#include "AITypes.h"
#include "Navigation/PathFollowingComponent.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_InvestigateWander::UBTTask_InvestigateWander()
{
    NodeName = TEXT("Investigate Wander");
    bCreateNodeInstance = true;
    bNotifyTaskFinished = true;
}

EBTNodeResult::Type UBTTask_InvestigateWander::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AZombieAIController* ZombieController = Cast<AZombieAIController>(OwnerComp.GetAIOwner());
    if (!ZombieController) return EBTNodeResult::Failed;

    AZombieCharacter* ZombieOwner = Cast<AZombieCharacter>(ZombieController->GetPawn());
    if (!ZombieOwner) return EBTNodeResult::Failed;

    // Only run while actively investigating
    if (ZombieOwner->GetZombieState() != EZombieState::EZS_Investigating)
    {
        UE_LOG(LogTemp, Warning, TEXT("[INVESTWANDER] Not in Investigating state — aborting"));
        return EBTNodeResult::Failed;
    }

    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!BB) return EBTNodeResult::Failed;

    // Pick a random point near LastKnownLocation — not the zombie's current position
    // This keeps the search centered around where the player was last seen
    const FVector SearchOrigin = BB->GetValueAsVector(AZombieAIController::BB_LastKnownLocation);
    const float Radius = ZombieOwner->GetZombieConfig().InvestigateWanderRadius;

    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(ZombieOwner->GetWorld());
    if (!NavSys)
    {
        UE_LOG(LogTemp, Warning, TEXT("[INVESTWANDER] No NavigationSystem found"));
        return EBTNodeResult::Failed;
    }

    FNavLocation RandomLocation;
    const bool bFound = NavSys->GetRandomReachablePointInRadius(SearchOrigin, Radius, RandomLocation);

    if (!bFound)
    {
        UE_LOG(LogTemp, Warning, TEXT("[INVESTWANDER] GetRandomReachablePointInRadius failed — retrying"));
        return EBTNodeResult::Failed;
    }

    FAIMoveRequest MoveRequest;
    MoveRequest.SetGoalLocation(RandomLocation.Location);
    MoveRequest.SetAcceptanceRadius(50.f);
    MoveRequest.SetAllowPartialPath(true);   // Allow partial — investigation area may be tight
    MoveRequest.SetProjectGoalLocation(true);

    FPathFollowingRequestResult Result = ZombieController->MoveTo(MoveRequest);

    if (Result.Code == EPathFollowingRequestResult::Failed)
    {
        UE_LOG(LogTemp, Warning, TEXT("[INVESTWANDER] MoveTo failed immediately — retrying"));
        return EBTNodeResult::Failed;
    }

    if (Result.Code == EPathFollowingRequestResult::AlreadyAtGoal)
    {
        // Already at the picked point — succeed immediately so BT picks a new point next cycle
        UE_LOG(LogTemp, Warning, TEXT("[INVESTWANDER] Already at goal — picking new point next cycle"));
        return EBTNodeResult::Succeeded;
    }

    // Movement started — cache refs and bind completion delegate
    MoveRequestID = Result.MoveId;
    OwnerController = ZombieController;
    BTCompRef = &OwnerComp;

    if (UPathFollowingComponent* PathComp = ZombieController->GetPathFollowingComponent())
    {
        PathComp->OnRequestFinished.AddUObject(this, &UBTTask_InvestigateWander::OnMoveCompleted);
        UE_LOG(LogTemp, Warning, TEXT("[INVESTWANDER] Moving to %s | Radius: %.0f"),
            *RandomLocation.Location.ToString(), Radius);
    }

    return EBTNodeResult::InProgress;
}

void UBTTask_InvestigateWander::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
    if (RequestID != MoveRequestID) return;

    if (OwnerController.IsValid())
    {
        if (UPathFollowingComponent* PathComp = OwnerController->GetPathFollowingComponent())
        {
            PathComp->OnRequestFinished.RemoveAll(this);
        }

        if (Result.IsSuccess())
        {
            // Arrived — succeed so BT immediately picks the next point (no idle dwell)
            UE_LOG(LogTemp, Warning, TEXT("[INVESTWANDER] Arrived — picking next search point"));
            FinishLatentTask(*BTCompRef, EBTNodeResult::Succeeded);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[INVESTWANDER] Move interrupted or failed — BT will retry"));
            FinishLatentTask(*BTCompRef, EBTNodeResult::Failed);
        }
    }
}

void UBTTask_InvestigateWander::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
    // Clean up delegate on any exit path
    if (AZombieAIController* ZombieController = Cast<AZombieAIController>(OwnerComp.GetAIOwner()))
    {
        if (UPathFollowingComponent* PathComp = ZombieController->GetPathFollowingComponent())
        {
            PathComp->OnRequestFinished.RemoveAll(this);
        }
    }

    Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

EBTNodeResult::Type UBTTask_InvestigateWander::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    if (AZombieAIController* ZombieController = Cast<AZombieAIController>(OwnerComp.GetAIOwner()))
    {
        if (UPathFollowingComponent* PathComp = ZombieController->GetPathFollowingComponent())
        {
            PathComp->OnRequestFinished.RemoveAll(this);
        }
        ZombieController->StopMovement();
        UE_LOG(LogTemp, Warning, TEXT("[INVESTWANDER] AbortTask — interrupted by higher priority branch"));
    }

    return Super::AbortTask(OwnerComp, NodeMemory);
}