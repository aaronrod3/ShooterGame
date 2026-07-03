// ZombieAnimInstance.cpp

#include "ZombieAnimInstance.h"
#include "ZombieCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

// ─────────────────────────────────────────────
// Init
// ─────────────────────────────────────────────

void UZombieAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	ZombieOwner = Cast<AZombieCharacter>(TryGetPawnOwner());
}

// ─────────────────────────────────────────────
// Update (called every frame by the Anim BP)
// ─────────────────────────────────────────────

void UZombieAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	// Lazy init — same pattern as UPlayerAnimInstance
	if (ZombieOwner == nullptr)
	{
		ZombieOwner = Cast<AZombieCharacter>(TryGetPawnOwner());
	}
	if (ZombieOwner == nullptr) return;

	UpdateAnimationProperties(DeltaTime);
}

// ─────────────────────────────────────────────
// Property reads from AZombieCharacter
// ─────────────────────────────────────────────

void UZombieAnimInstance::UpdateAnimationProperties(float /*DeltaTime*/)
{
	// Ground speed — flatten Z so vertical movement doesn't affect blend space
	FVector Velocity = ZombieOwner->GetVelocity();
	Velocity.Z = 0.f;
	Speed = Velocity.Size();

	// State — read directly from the replicated ZombieState on AZombieCharacter
	ZombieState = ZombieOwner->GetZombieState();

	// Derive convenience bools from state
	bIsCrawling  = ZombieState == EZombieState::EZS_Crawling;
	bIsDead      = ZombieState == EZombieState::EZS_Dead;
}


// ─────────────────────────────────────────────
// AnimNotify — Attack Window Close
// ─────────────────────────────────────────────

void UZombieAnimInstance::AnimNotify_AttackEnd()
{
	// Fired locally on each client at the exact frame the notify hits on the timeline.
	// Closes the attack window — anim BP can now transition back to locomotion.
	bIsAttacking = false;

	UE_LOG(LogTemp, Warning, TEXT("AnimNotify_AttackEnd fired on: %s"),
		ZombieOwner ? *ZombieOwner->GetName() : TEXT("NULL Owner"));
}