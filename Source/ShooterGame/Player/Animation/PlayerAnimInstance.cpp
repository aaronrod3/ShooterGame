// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerAnimInstance.h"
#include "KismetAnimationLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "ShooterGame/Player/Character/ShooterGameCharacter.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"


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
	
	// Cursor aim
	APlayerController* PlayerController = Cast<APlayerController>(ShooterGameCharacter->GetController());
	if (PlayerController)
	{
		FHitResult Hit;
		if (PlayerController->GetHitResultUnderCursorByChannel(TraceTypeQuery1, false, Hit))
		{
			FVector LookAtCursor = (Hit.ImpactPoint - ShooterGameCharacter->GetActorLocation()).GetSafeNormal2D();
			CursorDirection = FMath::RadiansToDegrees(
				FMath::Atan2(
					FVector::DotProduct(LookAtCursor, ShooterGameCharacter->GetActorRightVector()),
					FVector::DotProduct(LookAtCursor, ShooterGameCharacter->GetActorForwardVector())
				)
			);
		}
	}
	
	
	bIsAccelerating = ShooterGameCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;
	bIsCrouched = ShooterGameCharacter->IsCrouched();
	bWeaponEquipped = ShooterGameCharacter->IsWeaponEquipped();
	bIsAiming = ShooterGameCharacter->IsAiming();
	
	FRotator AimRotation = ShooterGameCharacter->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(ShooterGameCharacter->GetVelocity());
	YawOffset = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation).Yaw;
	
	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = ShooterGameCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaTime;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 10.f);
	Lean = FMath::Clamp(Interp, -90.f, 90.f);
}




