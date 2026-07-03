// Source/ShooterGame/Player/Animation/ANS_BlockADS.h
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_BlockADS.generated.h"

/**
 * Notify state that blocks ADS entry for the duration of the window.
 * NotifyBegin → SetAimingBlocked(true)
 * NotifyEnd   → SetAimingBlocked(false)
 *
 * Use this on reload, equip, and any montage that must prevent ADS mid-animation.
 */
UCLASS(meta=(DisplayName="Block ADS"))
class SHOOTERGAME_API UANS_BlockADS : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp,
							 UAnimSequenceBase* Animation,
							 float TotalDuration,
							 const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp,
						   UAnimSequenceBase* Animation,
						   const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

private:
	static void PushBlockState(const USkeletalMeshComponent* MeshComp, bool bBlocked);
};