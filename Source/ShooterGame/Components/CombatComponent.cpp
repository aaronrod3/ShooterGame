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
			FHitResult CrosshairHit;
			TraceUnderCrosshairs(CrosshairHit);
			HitTarget = CrosshairHit.ImpactPoint;
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
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = false;
	}
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (Character == nullptr || WeaponToEquip == nullptr) return;
	
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	
	if (HandSocket)
	{
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
	}
	
	EquippedWeapon->SetOwner(Character);
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
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
	
	if (bFireButtonPressed)
	{
		ServerFire(HitTarget);
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
	if (Character)
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
	if (!PlayerController) return;

	float MouseX, MouseY;
	ReticleState.bCursorValid    = PlayerController->GetMousePosition(MouseX, MouseY);
	
	if (ReticleState.bCursorValid)
	{
		ReticleState.CursorScreenPos = FVector2D(MouseX, MouseY);
	}
	else
	{
		// Fallback to viewport center when mouse is captured (PIE, game-only input mode)
		int32 SizeX, SizeY;
		PlayerController->GetViewportSize(SizeX, SizeY);
		ReticleState.CursorScreenPos = FVector2D(SizeX * 0.5f, SizeY * 0.5f);
		ReticleState.bCursorValid = true; // center is always a valid draw position
	}

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
		ReticleState.ReachRadius = ComputeReachRadius();
		
	}
	else
	{
		ReticleState.SpreadAlpha = 0.f;
		ReticleState.ReachRadius = -1.f;
	}
}

float UCombatComponent::ComputeReachRadius() const
{
	if (!EquippedWeapon || !Character || !GetWorld()) return -1.f;

	APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
	if (!PlayerController) return -1.f;

	// Get cursor screen position
	float MouseX, MouseY;
	if (!PlayerController->GetMousePosition(MouseX, MouseY)) return -1.f;

	const FVector2D CursorScreen(MouseX, MouseY);

	// Deproject cursor to world ray
	FVector WorldOrigin, WorldDirection;
	if (!PlayerController->DeprojectScreenPositionToWorld(MouseX, MouseY, WorldOrigin, WorldDirection)) return -1.f;

	const float WeaponRange = EquippedWeapon->GetWeaponRange();
	if (WeaponRange <= 0.f) return -1.f;

	const FVector TraceEnd = WorldOrigin + WorldDirection * WeaponRange;

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ReticleReachTrace), false);
	Params.AddIgnoredActor(Character);

	GetWorld()->LineTraceSingleByChannel(Hit, WorldOrigin, TraceEnd, ECC_Visibility, Params);

	const FVector EndPoint = Hit.bBlockingHit ? Hit.ImpactPoint : TraceEnd;

	FVector2D EndScreen;
	if (!PlayerController->ProjectWorldLocationToScreen(EndPoint, EndScreen, true)) return -1.f;

	// ReachRadius is distance from CURSOR position to the projected endpoint, not screen center
	const float Distance = FVector2D::Distance(CursorScreen, EndScreen);
	if (Distance < 1.f) return -1.f;
	return Distance;
}


bool UCombatComponent::TraceUnderCrosshairs(FHitResult& OutHit)
{
	if (!Character || !GetWorld()) return false;

	APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
	if (!PlayerController) return false;

	// Reuse the cursor screen pos already computed this tick
	const FVector2D& ScreenPos = ReticleState.CursorScreenPos;

	FVector WorldOrigin, WorldDirection;
	if (!PlayerController->DeprojectScreenPositionToWorld(ScreenPos.X, ScreenPos.Y, WorldOrigin, WorldDirection))
		return false;

	const float Range = EquippedWeapon ? EquippedWeapon->GetWeaponRange() : 10000.f;
	const FVector TraceEnd = WorldOrigin + WorldDirection * Range;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(CrosshairTrace), false);
	Params.AddIgnoredActor(Character);
	if (EquippedWeapon) Params.AddIgnoredActor(EquippedWeapon);

	bool bHit = GetWorld()->LineTraceSingleByChannel(OutHit, WorldOrigin, TraceEnd, ECC_Visibility, Params);
	if (!bHit)
	{
		OutHit.ImpactPoint = TraceEnd;
	}


	return bHit;
}





