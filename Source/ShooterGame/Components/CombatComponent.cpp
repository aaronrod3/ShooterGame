// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Items/Weapon/Weapon.h"
#include "Player/Character/ShooterGameCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerController.h"


UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	BaseWalkSpeed = 300.f;
	AimWalkSpeed = 250.f;
}


void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	}
	
}


void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                     FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Character && Character->IsLocallyControlled())
	{
		UpdateReticleState();
		
		if (EquippedWeapon)
		{
			UpdateReticleWorldPosition();
			HitTarget = ReticleWorldPosition;
		}
	}
	
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character)
	{
		Character->GetCharacterMovement()->bOrientRotationToMovement = true;
		Character->bUseControllerRotationYaw = false;
	}
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (Character == nullptr || WeaponToEquip == nullptr) return;
	
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	
	EquippedWeapon->AttachToComponent(
		Character->GetMesh(),
		FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true),
		FName("RightHandSocket")
	);
	
	EquippedWeapon->SetOwner(Character);
	Character->GetCharacterMovement()->bOrientRotationToMovement = true;
	Character->bUseControllerRotationYaw = false;
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	bAiming = bIsAiming;
	ServerSetAiming(bIsAiming);
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}


void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;
	if (!bFireButtonPressed || !EquippedWeapon) return;

	switch (EquippedWeapon->GetCurrentFireMode())
	{
	case EFireMode::EFM_Safe:
		break;

	case EFireMode::EFM_SemiAuto:
		if (!bFiredThisPress)          // only fire on the first call per press
		{
			bFiredThisPress = true;
			HandleFire();
		}
		break;

	case EFireMode::EFM_Burst:
		if (!bFiredThisPress && BurstShotsRemaining <= 0)
		{
			bFiredThisPress = true;
			BurstShotsRemaining = EquippedWeapon->GetBurstCount();
			HandleBurstFire();
		}
		break;

	case EFireMode::EFM_FullAuto:
		bFullAutoFiring = true;
		HandleFire();
		break;

	default:
		break;
	}
}

void UCombatComponent::FireButtonReleased()
{
	UE_LOG(LogTemp, Warning, TEXT("FireButtonReleased called"));
	bFireButtonPressed = false;
	bFullAutoFiring = false;
	bFiredThisPress = false;

	
}

void UCombatComponent::HandleFire()
{
	if (!EquippedWeapon || !GetWorld() || !Character) return;

	const float Rate = (EquippedWeapon->GetCurrentFireMode() == EFireMode::EFM_FullAuto)
		? EquippedWeapon->GetFullAutoFireRate()
		: EquippedWeapon->GetFireRate();

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastFireTime < Rate) return;

	LastFireTime = Now;

	Character->PlayFireMontage(bAiming);
	EquippedWeapon->Fire(HitTarget);
	ServerFire(HitTarget);

	// FullAuto: schedule next shot automatically while held
	if (bFullAutoFiring)
	{
		GetWorld()->GetTimerManager().SetTimer(
			BurstFireTimerHandle,
			this,
			&UCombatComponent::HandleFire,
			Rate,
			false
		);
	}
}

void UCombatComponent::HandleBurstFire()
{
	if (!EquippedWeapon || !GetWorld() || !Character) return;

	if (BurstShotsRemaining <= 0)
	{
		// Burst complete — reset so next press can trigger
		BurstShotsRemaining = 0;
		return;
	}

	BurstShotsRemaining--;

	Character->PlayFireMontage(bAiming);
	EquippedWeapon->Fire(HitTarget);
	ServerFire(HitTarget);

	GetWorld()->GetTimerManager().SetTimer(
		BurstFireTimerHandle,
		this,
		&UCombatComponent::HandleBurstFire,
		EquippedWeapon->GetFullAutoFireRate(),
		false
	);
}

void UCombatComponent::ResetCanFire()
{
	if (bFullAutoFiring && EquippedWeapon)
	{
		HandleFire();
	}
}

void UCombatComponent::CycleFireMode()
{
	if (!EquippedWeapon) return;

	EquippedWeapon->CycleFireMode();

	const FString DisplayName = EquippedWeapon->GetCurrentFireModeDisplayName();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			42,
			3.f,
			FColor::Yellow,
			FString::Printf(TEXT("Fire Mode: %s"), *DisplayName)
		);
	}
}



void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& InHitTarget)
{
	HitTarget = InHitTarget;
	MulticastFire();
}

void UCombatComponent::MulticastFire_Implementation()
{
	if (EquippedWeapon == nullptr) return;
	if (Character && !Character->IsLocallyControlled())
	{
		Character->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(HitTarget);
	}
}

void UCombatComponent::UpdateReticleState()
{
	ReticleState.bIsEquipped = EquippedWeapon != nullptr;
	ReticleState.bIsAiming   = bAiming;
	ReticleState.bIsCrouched = Character->bIsCrouched;

	APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
	if (!PlayerController)
	{
		ReticleState.bReticleValid = false;
		return;
	}

	ReticleState.bReticleValid = true;

	if (EquippedWeapon)
	{
		const float MaxSpread = FMath::Max(EquippedWeapon->GetMaxSpread(), 0.001f);
		float SpreadAlpha = FMath::Clamp(EquippedWeapon->GetCurrentSpread() / MaxSpread, 0.f, 1.f);

		// Movement modifier — normalize velocity against max walk speed
		const float CurrentSpeed = Character->GetVelocity().Size();
		const float MaxSpeed = FMath::Max(Character->GetCharacterMovement()->MaxWalkSpeed, 1.f);
		const float MovementAlpha = FMath::Clamp(CurrentSpeed / MaxSpeed, 0.f, 1.f);
		SpreadAlpha = FMath::Clamp(SpreadAlpha + MovementAlpha * MovementSpreadMultiplier, 0.f, 1.f);

		ReticleState.SpreadAlpha = SpreadAlpha;
		
	}
	else
	{
		ReticleState.SpreadAlpha = 0.f;
	}
}

void UCombatComponent::UpdateReticleWorldPosition()
{
	if (!Character || !GetWorld())
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
	if (!PlayerController)
	{
		return;
	}

	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);

	const float ScreenX = ViewportSizeX * 0.5f;
	const float ScreenY = ViewportSizeY * 0.5f;

	FVector TraceStart;
	FVector WorldDirection;
	if (!PlayerController->DeprojectScreenPositionToWorld(ScreenX, ScreenY, TraceStart, WorldDirection))
	{
		return;
	}

	const FVector TraceEnd = TraceStart + (WorldDirection * ReticleMaxDistance);

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ReticleWorldTrace), false);
	Params.AddIgnoredActor(Character);

	if (EquippedWeapon)
	{
		Params.AddIgnoredActor(EquippedWeapon);
	}

	GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params);

	ReticleWorldPosition = Hit.bBlockingHit ? Hit.ImpactPoint : TraceEnd;
}



















