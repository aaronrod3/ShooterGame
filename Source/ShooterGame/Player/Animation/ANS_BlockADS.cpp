// Source/ShooterGame/Player/Animation/ANS_BlockADS.cpp
#include "ANS_BlockADS.h"
#include "ShooterAnimStateInterface.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"

void UANS_BlockADS::NotifyBegin(USkeletalMeshComponent* MeshComp,
								 UAnimSequenceBase* Animation,
								 float TotalDuration,
								 const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	PushBlockState(MeshComp, true);
}

void UANS_BlockADS::NotifyEnd(USkeletalMeshComponent* MeshComp,
							   UAnimSequenceBase* Animation,
							   const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	PushBlockState(MeshComp, false);
}

FString UANS_BlockADS::GetNotifyName_Implementation() const
{
	return TEXT("Block ADS");
}

void UANS_BlockADS::PushBlockState(const USkeletalMeshComponent* MeshComp, bool bBlocked)
{
	if (!MeshComp) return;
	UAnimInstance* AnimInst = MeshComp->GetAnimInstance();
	if (!AnimInst) return;

	if (AnimInst->Implements<UShooterAnimStateInterface>())
	{
		IShooterAnimStateInterface::Execute_SetAimingBlocked(AnimInst, bBlocked);
	}
}