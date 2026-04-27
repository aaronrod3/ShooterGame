#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "Navigation/PathFollowingComponent.h"
#include "BTTask_WanderToPoint.generated.h"

UCLASS()
class SHOOTERGAME_API UBTTask_WanderToPoint : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_WanderToPoint();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

protected:
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:
	FAIRequestID MoveRequestID;

	// Cached refs needed inside the delegate callback (which has no OwnerComp parameter)
	TWeakObjectPtr<AAIController> OwnerController;
	UBehaviorTreeComponent* BTCompRef = nullptr;

	// Bound to UPathFollowingComponent::OnRequestFinished
	void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result);
};