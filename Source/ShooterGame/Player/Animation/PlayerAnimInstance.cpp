// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerAnimInstance.h"
#include "KismetAnimationLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "ShooterGame/Player/Character/ShooterGameCharacter.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ShooterGame/Items/Weapon/Weapon.h"


void UPlayerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	
	ShooterGameCharacter = Cast<AShooterGameCharacter>(TryGetPawnOwner());
}


void UPlayerAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);
	
	if (ShooterGameCharacter == nullptr)
	{
		ShooterGameCharacter = Cast<AShooterGameCharacter>(TryGetPawnOwner());
	}
	if (ShooterGameCharacter == nullptr) return;
	
	FVector Velocity = ShooterGameCharacter->GetVelocity();
	Velocity.Z = 0.f;
	Speed = Velocity.Size();
	// Calculate direction angle between actor facing and velocity
	Direction = UKismetAnimationLibrary::CalculateDirection(
		ShooterGameCharacter->GetVelocity(),
		ShooterGameCharacter->GetActorRotation()
	);
	
	
	bIsAccelerating = ShooterGameCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;
	bIsCrouched = ShooterGameCharacter->IsCrouched();
	bWeaponEquipped = ShooterGameCharacter->IsWeaponEquipped();
	EquippedWeapon = ShooterGameCharacter->GetEquippedWeapon();
	bIsAiming = ShooterGameCharacter->IsAiming();
	TurningInPlace = ShooterGameCharacter->GetTurningInPlace();
	
	UCombatComponent* CombatComponent = ShooterGameCharacter->GetCombat();

	bIsReloading = CombatComponent ? CombatComponent->IsReloadAnimationActive() : false;
	bIsInteractionActive = ShooterGameCharacter->IsInteractionAnimationRequested();
	WeaponStance = CombatComponent ? CombatComponent->GetPlayerWeaponStance() : EPlayerWeaponStance::EPWS_Unarmed;
	bUseLeftHandIK = false;
	
	FRotator ActorFacing = ShooterGameCharacter->GetActorRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(ShooterGameCharacter->GetVelocity());
	YawOffset = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, ActorFacing).Yaw;
	
	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = ShooterGameCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaTime;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.f);
	Lean = FMath::Clamp(Interp, -45.f, 45.f);
	
	AimOffset_Yaw = ShooterGameCharacter->GetAimOffset_Yaw();
	AimOffset_Pitch = 0.f;
	
	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && ShooterGameCharacter->GetMesh())
	{
		const FName LeftHandSocketName = ShooterGameCharacter->GetLeftHandIKSocketName();
		const FName RightHandBoneName = ShooterGameCharacter->GetRightHandIKBoneName();

		if (EquippedWeapon->GetWeaponMesh()->DoesSocketExist(LeftHandSocketName))
		{
			FTransform SocketTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(LeftHandSocketName, RTS_World);

			SocketTransform.AddToTranslation(ShooterGameCharacter->GetLeftHandIKLocationOffset());
			SocketTransform.ConcatenateRotation(ShooterGameCharacter->GetLeftHandIKRotationOffset().Quaternion());

			FVector OutPosition;
			FRotator OutRotation;
			ShooterGameCharacter->GetMesh()->TransformToBoneSpace(
				RightHandBoneName,
				SocketTransform.GetLocation(),
				SocketTransform.Rotator(),
				OutPosition,
				OutRotation
			);

			LeftHandTransform.SetLocation(OutPosition);
			LeftHandTransform.SetRotation(FQuat(OutRotation));
			LeftHandTransform.SetScale3D(FVector::OneVector);

			bUseLeftHandIK = true;
		}
	}
}


void UPlayerAnimInstance::AnimNotify_InteractionFinished()
{
	if (ShooterGameCharacter)
	{
		ShooterGameCharacter->StopInteractionAnimation();
	}
}




