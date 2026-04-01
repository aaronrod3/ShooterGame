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
	AimOffset_Pitch = ShooterGameCharacter->GetAimOffset_Pitch();
	
	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && ShooterGameCharacter->GetMesh())
	{
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), RTS_World);
		FVector OutPosition;
		FRotator OutRotation;
		ShooterGameCharacter->GetMesh()->TransformToBoneSpace(FName("ik_hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));
		
		FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("ik_hand_r"), RTS_World);
	}
}




