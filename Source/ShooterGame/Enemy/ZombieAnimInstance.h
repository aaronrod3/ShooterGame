// ZombieAnimInstance.h
// C++ base class for ABP_Zombie.
// Reads Speed and EZombieState from the owning AZombieCharacter each frame.
// All properties are BlueprintReadOnly so the Anim BP can bind them in the graph.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ShooterGame/Types/ZombieTypes.h"
#include "ZombieAnimInstance.generated.h"

class AZombieCharacter;

UCLASS()
class SHOOTERGAME_API UZombieAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaTime) override;
	
	FORCEINLINE void SetIsAttacking(bool bAttacking) { bIsAttacking = bAttacking; }
	
	UFUNCTION()
	void AnimNotify_AttackEnd();

private:
	// Cached reference to the owning zombie — set in NativeInitializeAnimation
	UPROPERTY(BlueprintReadOnly, Category = "Zombie|Anim", meta = (AllowPrivateAccess = "true"))
	AZombieCharacter* ZombieOwner;

	// Ground speed (XY only) — drives the locomotion blend space axis
	UPROPERTY(BlueprintReadOnly, Category = "Zombie|Anim", meta = (AllowPrivateAccess = "true"))
	float Speed;

	// Current AI behavioral state — drives state machine transitions
	UPROPERTY(BlueprintReadOnly, Category = "Zombie|Anim", meta = (AllowPrivateAccess = "true"))
	EZombieState ZombieState;

	// Convenience bools derived from ZombieState — easier to wire in the Anim BP
	UPROPERTY(BlueprintReadOnly, Category = "Zombie|Anim", meta = (AllowPrivateAccess = "true"))
	bool bIsCrawling;

	UPROPERTY(BlueprintReadOnly, Category = "Zombie|Anim", meta = (AllowPrivateAccess = "true"))
	bool bIsDead;

	UPROPERTY(BlueprintReadOnly, Category = "Zombie|Anim", meta = (AllowPrivateAccess = "true"))
	bool bIsAttacking;

	// Internal update — called every frame from NativeUpdateAnimation
	void UpdateAnimationProperties(float DeltaTime);
};