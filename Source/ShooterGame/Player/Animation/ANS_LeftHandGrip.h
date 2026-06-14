// Source/ShooterGame/Player/Animation/ANS_LeftHandGrip.h
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_LeftHandGrip.generated.h"

/**
 * Notify state that drives left-hand IK visibility through IShooterAnimStateInterface.
 * Place this on any montage to pull the left hand off the weapon for a window.
 *
 * NotifyBegin → UpdateLeftHandGrip(false, BlendOffSpeed)
 * NotifyEnd   → UpdateLeftHandGrip(true,  BlendOnSpeed)
 */
UCLASS(meta=(DisplayName="Left Hand Grip Override"))
class SHOOTERGAME_API UANS_LeftHandGrip : public UAnimNotifyState
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

	/** Blend speed when left hand is pulled away from the weapon. */
	UPROPERTY(EditAnywhere, Category = "Grip", meta=(ClampMin="0.1"))
	float BlendOffSpeed = 10.f;

	/** Blend speed when left hand returns to the weapon at notify end. */
	UPROPERTY(EditAnywhere, Category = "Grip", meta=(ClampMin="0.1"))
	float BlendOnSpeed = 8.f;

private:
	static void PushGripState(USkeletalMeshComponent* MeshComp,
							   bool bOnWeapon, float BlendSpeed);
};