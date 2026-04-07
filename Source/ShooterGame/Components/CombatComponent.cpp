// Source/ShooterGame/Components/CombatComponent.cpp

#include "CombatComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "ShooterGame/Player/Character/ShooterGameCharacter.h"
#include "ShooterGame/Items/Weapon/Weapon.h"
#include "ShooterGame/Inventory/InventoryComponent.h"
#include "ShooterGame/Components/DownedComponent.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

	BaseWalkSpeed = 600.f;
	AimWalkSpeed  = 350.f;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<AShooterGameCharacter>(GetOwner());
	if (Character && Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	}
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Character && Character->IsLocallyControlled())
	{
		UpdateReticleWorldPosition();
		UpdateReticleState();
	}
}

// -----------------------------------------------------------------------
// Equip
// -----------------------------------------------------------------------

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (!Character || !WeaponToEquip) return;

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	EquippedWeapon->GetWeaponMesh()->AttachToComponent(
		Character->GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		FName("RightHandSocket")
	);

	if (Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	}

	// TEMPORARY — starter mags for testing Steps 7 & 8
	// Remove after Step 9 ammo pickup actors are complete
	if (Character->HasAuthority())
	{
		UInventoryComponent* Inventory = Character->GetInventory();
		if (Inventory)
		{
			FMagazine Mag1 = FMagazine::MakeFull(EquippedWeapon->GetSupportedAmmoType(), 15);
			FMagazine Mag2 = FMagazine::MakeFull(EquippedWeapon->GetSupportedAmmoType(), 15);
			FMagazine Mag3 = FMagazine::MakeFull(EquippedWeapon->GetSupportedAmmoType(), 15);
			Inventory->AddMagazine(Mag1);
			Inventory->AddMagazine(Mag2);
			Inventory->AddMagazine(Mag3);
		}
	}
	// END TEMPORARY
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character)
	{
		EquippedWeapon->GetWeaponMesh()->AttachToComponent(
			Character->GetMesh(),
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			FName("RightHandSocket")
		);
	}
}

// -----------------------------------------------------------------------
// Aiming
// -----------------------------------------------------------------------

void UCombatComponent::SetAiming(bool bIsAiming)
{
	bAiming = bIsAiming;
	ServerSetAiming(bIsAiming);

	if (Character && Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;
	if (Character && Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

// -----------------------------------------------------------------------
// Fire Input
// -----------------------------------------------------------------------

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;

	if (bPressed)
	{
		bFiredThisPress = false;
		HandleFire();
	}
	else
	{
		bFiredThisPress = false;
		bBurstInProgress = false;

		if (bFullAutoFiring)
		{
			GetWorld()->GetTimerManager().ClearTimer(BurstFireTimerHandle);
			bFullAutoFiring = false;
		}
	}
}

void UCombatComponent::FireButtonReleased()
{
	FireButtonPressed(false);
}

void UCombatComponent::HandleFire()
{
	if (!EquippedWeapon) return;

	// Ammo gate — hard stop if nothing to fire
	if (!EquippedWeapon->CanFire())
	{
		UE_LOG(LogTemp, Warning, TEXT("UCombatComponent::HandleFire — Weapon empty, cannot fire"));
		return;
	}

	// Fire rate gate
	const float Now = GetWorld()->GetTimeSeconds();
	const float RequiredRate = GetActiveFireRate();
	if (Now - LastFireTime < RequiredRate) return;
	LastFireTime = Now;

	// Fire mode dispatch
	switch (EquippedWeapon->GetCurrentFireMode())
	{
	case EFireMode::EFM_Safe:
		return;

	case EFireMode::EFM_SemiAuto:
		if (bFiredThisPress) return;
		bFiredThisPress = true;

		ApplyDownedDebuffsPreFire();
		EquippedWeapon->Fire(HitTarget);
		EquippedWeapon->ConsumeRound();
		ServerFire(HitTarget);
		break;

	case EFireMode::EFM_Burst:
		if (bBurstInProgress || bFiredThisPress) return;

		bFiredThisPress = true;
		bBurstInProgress = true;
		BurstShotsRemaining = EquippedWeapon->GetBurstCount();
		HandleBurstFire();
		break;

	case EFireMode::EFM_FullAuto:
		if (!bFullAutoFiring)
		{
			bFullAutoFiring = true;
		}
		ApplyDownedDebuffsPreFire();
		EquippedWeapon->Fire(HitTarget);
		EquippedWeapon->ConsumeRound();
		ServerFire(HitTarget);

		GetWorld()->GetTimerManager().SetTimer(
			BurstFireTimerHandle,
			this,
			&UCombatComponent::HandleFire,
			EquippedWeapon->GetFullAutoFireRate(),
			false
		);
		break;
	}
}

void UCombatComponent::HandleBurstFire()
{
	if (!EquippedWeapon || BurstShotsRemaining <= 0)
	{
		BurstShotsRemaining = 0;
		bBurstInProgress = false;
		return;
	}

	if (!EquippedWeapon->CanFire())
	{
		BurstShotsRemaining = 0;
		bBurstInProgress = false;

		if (CanReload()) ReloadEquippedWeapon();
		return;
	}

	ApplyDownedDebuffsPreFire();
	EquippedWeapon->Fire(HitTarget);
	EquippedWeapon->ConsumeRound();
	ServerFire(HitTarget);
	--BurstShotsRemaining;

	if (BurstShotsRemaining > 0)
	{
		GetWorld()->GetTimerManager().SetTimer(
			BurstFireTimerHandle,
			this,
			&UCombatComponent::HandleBurstFire,
			EquippedWeapon->GetBurstFireRate(),
			false
		);
	}
	else
	{
		bBurstInProgress = false;
	}
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& InHitTarget)
{
	HitTarget = InHitTarget;
	MulticastFire();
}

void UCombatComponent::MulticastFire_Implementation()
{
	// Replay cosmetics on non-owning clients only
	// Owning client and server already called Fire() locally in HandleFire()
	if (EquippedWeapon && !Character->IsLocallyControlled())
	{
		EquippedWeapon->Fire(HitTarget);
	}
}

// -----------------------------------------------------------------------
// Fire Mode Cycle
// -----------------------------------------------------------------------

void UCombatComponent::CycleFireMode()
{
	if (!EquippedWeapon) return;
	EquippedWeapon->CycleFireMode();
	// TODO: ServerCycleFireMode RPC for proper replication
}

// -----------------------------------------------------------------------
// Reload
// -----------------------------------------------------------------------

bool UCombatComponent::CanReload() const
{
	if (!EquippedWeapon) return false;
	if (EquippedWeapon->GetFeedType() == EWeaponFeedType::EFT_SingleShot) return false;
	if (bIsReloading) return false;
	if (!Character) return false;

	UInventoryComponent* Inventory = Character->GetInventory();
	if (!Inventory) return false;

	return Inventory->HasMagazineFor(EquippedWeapon->GetSupportedAmmoType());
}

void UCombatComponent::ReloadEquippedWeapon()
{
	if (!EquippedWeapon) return;
	if (!Character) return;

	UInventoryComponent* Inventory = Character->GetInventory();
	if (!Inventory) return;

	if (!CanReload()) return;

	bIsReloading = true;
	ServerReload();

	GetWorld()->GetTimerManager().SetTimer(
		ReloadTimerHandle,
		this,
		&UCombatComponent::FinishReload,
		ReloadDuration,
		false
	);
}

void UCombatComponent::ServerReload_Implementation()
{
	if (!EquippedWeapon || !Character) return;

	UInventoryComponent* Inventory = Character->GetInventory();
	if (!Inventory) return;

	EAmmoType AmmoType = EquippedWeapon->GetSupportedAmmoType();

	// Eject current magazine — return partial mag to inventory
	FMagazine EjectedMag = EquippedWeapon->EjectMagazine();
	Inventory->ReturnMagazine(EjectedMag);

	// Find best available magazine
	FMagazine* BestMag = Inventory->GetBestMagazine(AmmoType);
	if (!BestMag)
	{
		UE_LOG(LogTemp, Warning, TEXT("ServerReload — No magazine found in inventory after eject"));
		bIsReloading = false;
		return;
	}

	EquippedWeapon->InsertMagazine(*BestMag);
	Inventory->RemoveMagazine(*BestMag);
	EquippedWeapon->ChamberRound();

	UE_LOG(LogTemp, Log, TEXT("ServerReload — Loaded %d rounds | Chambered: %s"),
		EquippedWeapon->GetLoadedRounds(),
		EquippedWeapon->HasChamberedRound() ? TEXT("true") : TEXT("false"));

	MulticastReload();
}

void UCombatComponent::MulticastReload_Implementation()
{
	// TODO: Play reload montage on character mesh here
	// if (Character && ReloadMontage)
	// {
	//     Character->GetMesh()->GetAnimInstance()->Montage_Play(ReloadMontage);
	// }
}

void UCombatComponent::FinishReload()
{
	bIsReloading = false;
}

// -----------------------------------------------------------------------
// Magazine Pickup
// -----------------------------------------------------------------------

void UCombatComponent::PickupMagazine(const FMagazine& Mag)
{
	if (!Character) return;

	UInventoryComponent* Inventory = Character->GetInventory();
	if (!Inventory) return;

	bool bAdded = Inventory->AddMagazine(Mag);
	UE_LOG(LogTemp, Log, TEXT("UCombatComponent::PickupMagazine — %s (%d rounds)"),
		bAdded ? TEXT("Added") : TEXT("Slots full, dropped"),
		Mag.CurrentRounds);
}

// -----------------------------------------------------------------------
// Reticle
// -----------------------------------------------------------------------

float UCombatComponent::GetActiveFireRate() const
{
	if (!EquippedWeapon) return 0.15f;

	float BaseRate = 0.15f;

	switch (EquippedWeapon->GetCurrentFireMode())
	{
	case EFireMode::EFM_FullAuto:	BaseRate = EquippedWeapon->GetFullAutoFireRate(); break;
	case EFireMode::EFM_Burst:		BaseRate = EquippedWeapon->GetBurstFireRate();    break;
	default:						BaseRate = EquippedWeapon->GetFireRate();         break;
	}

	// Apply downed fire rate debuff — multiplier < 1 slows fire rate
	// by increasing the minimum time between shots
	if (Character)
	{
		UDownedComponent* DownedComp = Character->GetDownedComponent();
		if (DownedComp && DownedComp->IsDowned())
		{
			const float Multiplier = DownedComp->DebuffConfig.FireRateMultiplier;
			// FireRateMultiplier < 1 = slower — divide rate to increase delay
			// e.g. rate 0.1s / 0.6 = 0.167s between shots (slower)
			BaseRate = (Multiplier > 0.f) ? (BaseRate / Multiplier) : BaseRate;
		}
	}

	return BaseRate;
}

void UCombatComponent::UpdateReticleState()
{
	if (!EquippedWeapon) return;

	ReticleState.bIsEquipped = true;
	ReticleState.bIsAiming   = bAiming;
	ReticleState.bIsCrouched = Character ? Character->bIsCrouched : false;

	float SpreadBase = EquippedWeapon->GetCurrentSpread() / EquippedWeapon->GetMaxSpread();

	if (Character && Character->GetCharacterMovement())
	{
		float Speed     = Character->GetCharacterMovement()->Velocity.Size2D();
		float MaxSpeed  = Character->GetCharacterMovement()->MaxWalkSpeed;
		float MoveFraction = FMath::Clamp(Speed / MaxSpeed, 0.f, 1.f);
		SpreadBase += MoveFraction * MovementSpreadMultiplier;
	}

	if (ReticleState.bIsCrouched)
	{
		SpreadBase *= CrouchSpreadMultiplier;
	}

	ReticleState.SpreadAlpha = FMath::Clamp(SpreadBase, 0.f, 1.f);
}

void UCombatComponent::UpdateReticleWorldPosition()
{
	if (!Character || !EquippedWeapon) return;

	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	if (!PC) return;

	// For top-down: project mouse cursor onto a horizontal plane at character height
	// This gives the world position the player is aiming at, not a screen-center trace
	FVector PlaneOrigin = Character->GetActorLocation();
	FVector PlaneNormal = FVector(0.f, 0.f, 1.f); // horizontal plane

	FHitResult CursorHit;
	PC->GetHitResultUnderCursorByChannel(
		UEngineTypes::ConvertToTraceType(ECollisionChannel::ECC_Visibility),
		true,
		CursorHit
	);

	if (CursorHit.bBlockingHit)
	{
		// Use the cursor hit point but clamp Z to muzzle/torso height
		ReticleWorldPosition = CursorHit.ImpactPoint;
		ReticleWorldPosition.Z = EquippedWeapon->GetWeaponMesh()->GetSocketLocation(FName("MuzzleFlash")).Z;

		ReticleState.bCursorValid = true;
	}
	else
	{
		// Fallback — deproject cursor to world and intersect with horizontal plane
		FVector WorldLocation, WorldDirection;
		if (PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
		{
			// Ray-plane intersection: t = (PlaneOrigin - WorldLocation) · PlaneNormal / (WorldDirection · PlaneNormal)
			float Denom = FVector::DotProduct(WorldDirection, PlaneNormal);
			if (!FMath::IsNearlyZero(Denom))
			{
				float T = FVector::DotProduct(PlaneOrigin - WorldLocation, PlaneNormal) / Denom;
				ReticleWorldPosition = WorldLocation + WorldDirection * T;
				ReticleWorldPosition.Z = Character->GetActorLocation().Z + ProjectileFireHeightOffset;
			}
			ReticleState.bCursorValid = false;
		}
	}

	HitTarget = ReticleWorldPosition;
}

void UCombatComponent::ApplyDownedDebuffsPreFire()
{
	if (!Character || !EquippedWeapon) return;

	UDownedComponent* DownedComp = Character->GetDownedComponent();
	if (!DownedComp || !DownedComp->IsDowned()) return;

	// Notify downed component that the player fired this tick
	// Used by bleedout rate calculator (firing = faster drain)
	DownedComp->NotifyFired();

	// Apply spread multiplier — temporarily boost weapon spread before Fire()
	// Weapon's DecaySpread tick will still run normally
	const float Multiplier = DownedComp->DebuffConfig.SpreadMultiplier;
	EquippedWeapon->ApplySpreadMultiplier(Multiplier);
}