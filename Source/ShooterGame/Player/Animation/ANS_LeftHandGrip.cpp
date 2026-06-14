// Source/ShooterGame/Player/Animation/ANS_LeftHandGrip.cpp
#include "ANS_LeftHandGrip.h"
#include "ShooterAnimInstanceBase.h"
#include "ShooterAnimStateInterface.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"

void UANS_LeftHandGrip::NotifyBegin(USkeletalMeshComponent* MeshComp,
									 UAnimSequenceBase* Animation,
									 float TotalDuration,
									 const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	PushGripState(MeshComp, false, BlendOffSpeed);
}

void UANS_LeftHandGrip::NotifyEnd(USkeletalMeshComponent* MeshComp,
								   UAnimSequenceBase* Animation,
								   const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	PushGripState(MeshComp, true, BlendOnSpeed);

	UAnimInstance* AnimInst = MeshComp ? MeshComp->GetAnimInstance() : nullptr;
	if (UShooterAnimInstanceBase* ShooterInst = Cast<UShooterAnimInstanceBase>(AnimInst))
	{
		ShooterInst->ClearGripOverride();
	}
}

FString UANS_LeftHandGrip::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("LH Grip [Off:%.0f On:%.0f]"), BlendOffSpeed, BlendOnSpeed);
}

void UANS_LeftHandGrip::PushGripState(USkeletalMeshComponent* MeshComp,
										bool bOnWeapon, float BlendSpeed)
{
	if (!MeshComp) return;
	UAnimInstance* AnimInst = MeshComp->GetAnimInstance();
	if (!AnimInst) return;

	if (AnimInst->Implements<UShooterAnimStateInterface>())
	{
		IShooterAnimStateInterface::Execute_UpdateLeftHandGrip(AnimInst, bOnWeapon, BlendSpeed);
	}
}