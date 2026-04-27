// BTTask_InvestigateWander.h
// Picks a random nav point near LastKnownLocation and moves there.
// No idle dwell — loops immediately so the zombie appears to search restlessly.
// Only runs while bInvestigationTimerStarted is true (gated in BT).

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "Navigation/PathFollowingComponent.h"
#include "BTTask_InvestigateWander.generated.h"

UCLASS()
class SHOOTERGAME_API UBTTask_InvestigateWander : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_InvestigateWander();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

protected:
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:
	FAIRequestID MoveRequestID;

	// Cached refs for use inside the path following delegate
	TWeakObjectPtr<AAIController> OwnerController;
	UBehaviorTreeComponent* BTCompRef = nullptr;

	void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result);
};