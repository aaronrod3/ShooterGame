// Source/ShooterGame/Components/CombatComponent.cpp

#include "CombatComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "Engine/DamageEvents.h"
#include "HUD/ShooterHUD.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ShooterGame/Player/Character/ShooterGameCharacter.h"
#include "ShooterGame/Items/Weapon/Weapon.h"
#include "ShooterGame/Items/Weapon/WeaponConfig.h"
#include "ShooterGame/Inventory/InventoryComponent.h"
#include "ShooterGame/Components/DownedComponent.h"
#include "ShooterGame/Components/HitZoneComponent.h"

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
	DOREPLIFETIME(UCombatComponent, CurrentCombatAction);
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Character && Character->IsLocallyControlled())
	{
		UpdateReticleWorldPosition();
		UpdateReticleState();
	}

	UpdatePendingHipFireShot(DeltaTime);
}

// -----------------------------------------------------------------------
// Equip
// -----------------------------------------------------------------------

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	UE_LOG(LogTemp, Warning, TEXT("[Combat] EquipWeapon ENTERED — Weapon: %d, Character: %d"),
		WeaponToEquip != nullptr, Character != nullptr);
	
	if (!Character || !WeaponToEquip) return;

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	const FName AttachSocket = (EquippedWeapon->GetWeaponConfig() && EquippedWeapon->GetWeaponConfig()->SocketGunAttachment != NAME_None)
	? EquippedWeapon->GetWeaponConfig()->SocketGunAttachment
	: FName("RightHandSocket"); // fallback for weapons without a config
	
	UE_LOG(LogTemp, Warning, TEXT("[Combat] EquipWeapon — socket exists: %d"),
		Character->GetMesh()->GetSocketByName(FName("RightHandSocket")) != nullptr);

	EquippedWeapon->GetWeaponMesh()->AttachToComponent(
		Character->GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		AttachSocket
	);

	// TPS orientation is managed by SetOrientationForAiming on the character.
	// Equipping a weapon should not force orientation state — leave it to the aim system.
	Character->SetOrientationForAiming(false);
	
	// Notify the HUD to bind to the new weapon's ammo delegate
	if (Character && Character->IsLocallyControlled())
	{
		AShooterHUD* HUD = nullptr;
		if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
		{
			HUD = Cast<AShooterHUD>(PC->GetHUD());
		}
		if (HUD)
		{
			HUD->BindToWeapon(EquippedWeapon);
			
			EquippedWeapon->OnAmmoChanged.Broadcast(
				EquippedWeapon->GetLoadedRounds(),
				EquippedWeapon->GetMagCapacity()
			);
		}
	}
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character)
	{
		const FName AttachSocket = (EquippedWeapon->GetWeaponConfig() && EquippedWeapon->GetWeaponConfig()->SocketGunAttachment != NAME_None)
		? EquippedWeapon->GetWeaponConfig()->SocketGunAttachment
		: FName("RightHandSocket");

		EquippedWeapon->GetWeaponMesh()->AttachToComponent(
			Character->GetMesh(),
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			AttachSocket
		);

		// Bind the HUD on the owning client — EquipWeapon() doesn't run here,
		// only OnRep fires, so we must bind the ammo delegate from this path too
		if (Character->IsLocallyControlled())
		{
			AShooterHUD* HUD = nullptr;
			if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
			{
				HUD = Cast<AShooterHUD>(PC->GetHUD());
			}
			if (HUD)
			{
				HUD->BindToWeapon(EquippedWeapon);
				
				EquippedWeapon->OnAmmoChanged.Broadcast(
					EquippedWeapon->GetLoadedRounds(),
					EquippedWeapon->GetMagCapacity()
				);
			}
		}
	}
}


void UCombatComponent::SpawnDefaultWeapon()
{
    // Server-only guard — clients receive the weapon via OnRep_EquippedWeapon
    if (!Character || !Character->HasAuthority()) return;

    // Skip if a weapon is already equipped (respawn safety)
    if (EquippedWeapon) return;

    if (!DefaultWeaponClass)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("UCombatComponent::SpawnDefaultWeapon — DefaultWeaponClass is null on %s. "
                 "Assign it in the character Blueprint defaults under Combat|Starter Weapon."),
            *GetNameSafe(Character));
        return;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner      = Character;
    SpawnParams.Instigator = Character;
    // Spawn deferred so we can call InitFromConfig before BeginPlay fires
    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AWeapon* NewWeapon = GetWorld()->SpawnActorDeferred<AWeapon>(
        DefaultWeaponClass,
        FTransform::Identity,
        Character,
        Character,
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn
    );

    if (!NewWeapon)
    {
        UE_LOG(LogTemp, Error,
            TEXT("UCombatComponent::SpawnDefaultWeapon — SpawnActorDeferred failed for class %s"),
            *DefaultWeaponClass->GetName());
        return;
    }

    // Initialize from config before BeginPlay so mesh and AnimBP are set
    // when the weapon's own BeginPlay runs
    if (DefaultWeaponConfig)
    {
        NewWeapon->InitFromConfig(DefaultWeaponConfig);
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("UCombatComponent::SpawnDefaultWeapon — DefaultWeaponConfig is null. "
                 "Weapon will spawn with no mesh. Assign it in BP defaults."));
    }

    UGameplayStatics::FinishSpawningActor(NewWeapon, FTransform::Identity);

    // Route through the existing EquipWeapon path so all state, HUD
    // binding, and replication are handled identically to a pickup equip
    EquipWeapon(NewWeapon);
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
	
	if (bIsAiming && EquippedWeapon)
	{
		EquippedWeapon->ApplySpreadMultiplier(AimAccuracyBonusMultiplier);
	}
}

void UCombatComponent::OnRep_Aiming()
{
	// Fires on simulated proxies when bAiming replicates from the server.
	// Apply orientation so Client 2 sees the correct body facing mode.
	if (Character)
	{
		Character->SetOrientationForAiming(bAiming);
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;
	if (Character && Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
	
	if (Character)
	{
		Character->SetOrientationForAiming(bIsAiming);
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

	if (EquippedWeapon)
	{
		EquippedWeapon->StopFire();
	}
}

void UCombatComponent::HandleFire()
{
    if (!EquippedWeapon) return;

    // Ammo gate — play dry fire click and bail, never reach Fire()
    if (!EquippedWeapon->CanFire())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("UCombatComponent::HandleFire — CanFire false, playing dry fire"));

        // Stop any running loop if mag just ran out mid-burst
    	if (EquippedWeapon->WeaponAudioComp)
    	{
    		UE_LOG(LogTemp, Warning,
				TEXT("UCombatComponent::HandleFire — stopping loop on empty mag"));
    		EquippedWeapon->WeaponAudioComp->StopLoop_ForMultiplayer();
    		EquippedWeapon->WeaponAudioComp->PlayDryFire_ForMultiplayer();
    	}
    	// Tell server to multicast dry fire to all other clients
    	if (Character && !Character->HasAuthority())
    	{
    		ServerPlayDryFire();
    	}
        return;
    }

    if (bPendingHipFireShot) return;

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
    {
        if (bFiredThisPress) return;
        bFiredThisPress = true;

        ApplyDownedDebuffsPreFire();

        if (ShouldDelayHipFireShot())
        {
            StartPendingHipFireShot(ComputeFinalHitTarget());
        }
        else
        {
        	const FVector FinalHitTarget = ComputeFinalHitTarget();
        	EquippedWeapon->Fire(FinalHitTarget);
        	ServerFire(FinalHitTarget);
        }
        break;
    }

    case EFireMode::EFM_Burst:
    {
        if (bBurstInProgress || bFiredThisPress) return;

        bFiredThisPress = true;
        bBurstInProgress = true;
        BurstShotsRemaining = EquippedWeapon->GetBurstCount();
        HandleBurstFire();
        break;
    }

    case EFireMode::EFM_FullAuto:
    {
        if (!bFullAutoFiring)
        {
            bFullAutoFiring = true;
        }

        ApplyDownedDebuffsPreFire();

        if (ShouldDelayHipFireShot())
        {
            StartPendingHipFireShot(ComputeFinalHitTarget());
        }
        else
        {
            const FVector FinalHitTarget = ComputeFinalHitTarget();
            EquippedWeapon->Fire(FinalHitTarget);
            ServerFire(FinalHitTarget);
        }

        GetWorld()->GetTimerManager().SetTimer(
            BurstFireTimerHandle,
            this,
            &UCombatComponent::HandleFire,
            EquippedWeapon->GetFullAutoFireRate(),
            false
        );
        break;
    }

    default:
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

	if (bPendingHipFireShot) return;

	ApplyDownedDebuffsPreFire();

	if (ShouldDelayHipFireShot())
	{
		StartPendingHipFireShot(ComputeFinalHitTarget());
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

		return;
	}

	const FVector FinalHitTarget = ComputeFinalHitTarget();

	EquippedWeapon->Fire(FinalHitTarget);
	ServerFire(FinalHitTarget);
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

	if (EquippedWeapon)
	{
		EquippedWeapon->ConsumeRound();
	}

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

	if (EquippedWeapon->WeaponAudioComp)
	{
		EquippedWeapon->WeaponAudioComp->PlaySwitchFireMode_ForMultiplayer();
	}

	// Send to server to multicast audio to all other clients
	if (Character && !Character->HasAuthority())
	{
		ServerCycleFireMode();
	}
}

// -----------------------------------------------------------------------
// Reload
// -----------------------------------------------------------------------

bool UCombatComponent::CanReload() const
{
	if (!EquippedWeapon) { UE_LOG(LogTemp, Warning, TEXT("CanReload — FALSE: No weapon")); return false; }
	if (EquippedWeapon->GetFeedType() == EWeaponFeedType::EFT_SingleShot) { UE_LOG(LogTemp, Warning, TEXT("CanReload — FALSE: SingleShot")); return false; }
	if (CurrentCombatAction == ECombatAction::Reloading) return false;
	if (!Character){ UE_LOG(LogTemp, Warning, TEXT("CanReload — FALSE: No character")); return false; }

	UInventoryComponent* Inventory = Character->GetInventory();
	if (!Inventory) { UE_LOG(LogTemp, Warning, TEXT("CanReload — FALSE: No inventory")); return false; }

	const bool bHasMag = Inventory->HasMagazineFor(EquippedWeapon->GetSupportedAmmoType());
	UE_LOG(LogTemp, Warning,
		TEXT("CanReload — HasMag: %s | Slots: %d | %s"),
		bHasMag ? TEXT("TRUE") : TEXT("FALSE"),
		Inventory->GetUsedMagazineSlots(),
		Character->HasAuthority() ? TEXT("Server") : TEXT("Client"));

	return bHasMag;
}

void UCombatComponent::ReloadEquippedWeapon()
{
	UE_LOG(LogTemp, Warning, TEXT("[Reload] Called — EquippedWeapon: %s"),
		EquippedWeapon ? *EquippedWeapon->GetName() : TEXT("NULL"));
	UE_LOG(LogTemp, Warning, TEXT("[Reload] CurrentAction: %d, bAiming: %d"),
	(int32)CurrentCombatAction, (int32)bAiming);
	UE_LOG(LogTemp, Warning, TEXT("[Reload] MagRounds: %d"),
		EquippedWeapon ? EquippedWeapon->GetMagRounds() : -1);
	
	
	if (!EquippedWeapon || !Character) return;
	if (bLocalReloadPending) return;  // ← client-local gate only, never touches bIsReloading
	if (EquippedWeapon->GetFeedType() == EWeaponFeedType::EFT_SingleShot) return;
	
	if (Character->HasAuthority() && CurrentCombatAction == ECombatAction::Reloading) return;

	// Quick local inventory pre-check for responsiveness — server re-validates authoritatively
	UInventoryComponent* Inventory = Character->GetInventory();
	if (!Inventory || !Inventory->HasMagazineFor(EquippedWeapon->GetSupportedAmmoType())) return;

	UE_LOG(LogTemp, Warning, TEXT("[Reload] bLocalReloadPending: %d, HasMag: %d, AmmoType: %d"),
	(int32)bLocalReloadPending,
	Character->GetInventory() ? (int32)Character->GetInventory()->HasMagazineFor(EquippedWeapon->GetSupportedAmmoType()) : -1,
	(int32)EquippedWeapon->GetSupportedAmmoType());
	
	
	bLocalReloadPending = true;

	// Play audio locally for immediate feedback
	if (EquippedWeapon->WeaponAudioComp)
	{
		EquippedWeapon->WeaponAudioComp->PlayReload_ForMultiplayer(
			EquippedWeapon->IsPistolClass()
		);
	}
	
	if (Character && Character->IsLocallyControlled())
	{
		Character->PlayReloadMontage();
	}

	// Client cosmetic timer — clears bLocalReloadPending after animation duration
	GetWorld()->GetTimerManager().SetTimer(
		ReloadTimerHandle,
		this,
		&UCombatComponent::FinishReload,
		ReloadDuration,
		false
	);

	// Send to server — server is the ONLY place bIsReloading is set
	ServerReload();
}

void UCombatComponent::ServerReload_Implementation()
{
	if (!EquippedWeapon || !Character) return;

	if (!CanReload()) return;

	// Determine reload variant before setting state
	const bool bMagEmpty = (EquippedWeapon->GetMagRounds() == 0);
	CurrentReloadType = bMagEmpty ? EReloadType::Empty : EReloadType::Normal;

	// Set authoritative action state
	CurrentCombatAction = ECombatAction::Reloading;
	bIsBusy = true;

	if (EquippedWeapon->WeaponAudioComp)
	{
		EquippedWeapon->WeaponAudioComp->PlayReload_ForMultiplayer(EquippedWeapon->IsPistolClass());
	}

	MulticastReload();

	GetWorld()->GetTimerManager().SetTimer(
		ServerReloadTimerHandle, this,
		&UCombatComponent::FinishReload_Server,
		ReloadDuration, false
	);
}


void UCombatComponent::MulticastReload_Implementation()
{
	// Play reload cosmetics on non-owning clients
	// Owning client already handles audio in ReloadEquippedWeapon()
	if (Character && !Character->IsLocallyControlled())
	{
		bLocalReloadPending = true;

		GetWorld()->GetTimerManager().SetTimer(
			ReloadTimerHandle,
			this,
			&UCombatComponent::FinishReload,
			ReloadDuration,
			false
		);

		if (EquippedWeapon->WeaponAudioComp)
		{
			EquippedWeapon->WeaponAudioComp->PlayReload_ForMultiplayer(EquippedWeapon->IsPistolClass());
		}

		Character->PlayReloadMontage();
	}
}

void UCombatComponent::FinishReload()
{
	bLocalReloadPending = false;
}

void UCombatComponent::MulticastFinishReload_Implementation()
{
	bLocalReloadPending = false;
	UE_LOG(LogTemp, Warning, TEXT("MulticastFinishReload — Server-authoritative clear on: %s"),
		Character->HasAuthority() ? TEXT("Server") : TEXT("Client"));
}

void UCombatComponent::FinishReload_Server()
{
	UE_LOG(LogTemp, Warning, TEXT("FinishReload_Server — Executing mag swap"));

	if (!EquippedWeapon || !Character)
	{
		CurrentCombatAction = ECombatAction::None;
		CurrentReloadType   = EReloadType::None;
		bIsBusy             = false;
		MulticastFinishReload();
		return;
	}

	UInventoryComponent* Inventory = Character->GetInventory();
	if (!Inventory)
	{
		CurrentCombatAction = ECombatAction::None;
		CurrentReloadType   = EReloadType::None;
		bIsBusy             = false;
		MulticastFinishReload();
		return;
	}

	const EAmmoType AmmoType = EquippedWeapon->GetSupportedAmmoType();

	FMagazine EjectedMag = EquippedWeapon->EjectMagazine();
	UE_LOG(LogTemp, Warning,
		TEXT("FinishReload_Server — Ejected: %d rounds"), EjectedMag.CurrentRounds);

	Inventory->ReturnMagazine(EjectedMag);

	FMagazine* BestMag = Inventory->GetBestMagazine(AmmoType);
	if (!BestMag)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("FinishReload_Server — FAILED: No magazine found. Slots: %d"),
			Inventory->GetUsedMagazineSlots());
		CurrentCombatAction = ECombatAction::None;
		CurrentReloadType   = EReloadType::None;
		bIsBusy             = false;
		MulticastFinishReload();
		return;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("FinishReload_Server — Inserting: %d rounds"), BestMag->CurrentRounds);

	EquippedWeapon->InsertMagazine(*BestMag);
	Inventory->RemoveMagazine(*BestMag);
	EquippedWeapon->ChamberRound();
	EquippedWeapon->ForceNetUpdate();

	EquippedWeapon->OnAmmoChanged.Broadcast(
		EquippedWeapon->GetLoadedRounds(),
		EquippedWeapon->GetMagCapacity()
	);

	CurrentCombatAction = ECombatAction::None;
	CurrentReloadType   = EReloadType::None;
	bIsBusy             = false;
	MulticastFinishReload();

	UE_LOG(LogTemp, Warning,
		TEXT("FinishReload_Server — COMPLETE: %d rounds | Chambered: %s"),
		EquippedWeapon->GetLoadedRounds(),
		EquippedWeapon->HasChamberedRound() ? TEXT("true") : TEXT("false"));
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

	// Get camera position and forward direction
	FVector CameraLocation;
	FRotator CameraRotation;
	PC->GetPlayerViewPoint(CameraLocation, CameraRotation);

	const FVector ShotDirection = CameraRotation.Vector();
	const float TraceDistance = 50000.f; // 50m — tune as needed

	const FVector TraceStart = CameraLocation;
	const FVector TraceEnd   = TraceStart + ShotDirection * TraceDistance;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WeaponLineTrace), /*bTraceComplex*/ true, Character);
	QueryParams.bReturnPhysicalMaterial = false;

	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		// Hit something along the reticle line
		ReticleWorldPosition = Hit.ImpactPoint;
		ReticleState.bCursorValid = true;
	}
	else
	{
		// No hit — use a point straight ahead at TraceDistance
		ReticleWorldPosition = TraceEnd;
		ReticleState.bCursorValid = false;
	}

	// Use this as the logical HitTarget for firing; muzzle check will refine it
	HitTarget = ReticleWorldPosition;
}

FVector UCombatComponent::ComputeFinalHitTarget() const
{
	if (!Character || !EquippedWeapon) return HitTarget;

	const USkeletalMeshComponent* WeaponMesh = EquippedWeapon->GetWeaponMesh();
	if (!WeaponMesh) return HitTarget;

	const FName MuzzleSocketName("MuzzleFlash");
	const FVector MuzzleLocation = WeaponMesh->GetSocketLocation(MuzzleSocketName);

	// Direction from muzzle toward the current HitTarget
	const FVector ToTarget = (HitTarget - MuzzleLocation);
	const FVector ShootDir = ToTarget.GetSafeNormal();

	const float DistanceToTarget = ToTarget.Size();
	const FVector MuzzleTraceEnd = MuzzleLocation + ShootDir * DistanceToTarget;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MuzzleLineTrace), true, Character);
	QueryParams.bReturnPhysicalMaterial = false;
	QueryParams.AddIgnoredActor(EquippedWeapon);

	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, MuzzleLocation, MuzzleTraceEnd, ECC_Visibility, QueryParams))
	{
		// Something between muzzle and reticle target — shoot that instead
		return Hit.ImpactPoint;
	}

	// Path is clear — shoot at the reticle target
	return HitTarget;
}


bool UCombatComponent::ShouldDelayHipFireShot() const
{
	return Character && EquippedWeapon && !bAiming;
}

float UCombatComponent::GetHipFireYawDeltaToControl() const
{
	if (!Character) return 0.f;

	const float ActorYaw = Character->GetActorRotation().Yaw;
	const float ControlYaw = Character->GetControlRotation().Yaw;

	return FMath::Abs(FMath::FindDeltaAngleDegrees(ActorYaw, ControlYaw));
}

void UCombatComponent::StartPendingHipFireShot(const FVector& InTarget)
{
	if (!Character) return;

	bPendingHipFireShot = true;
	PendingHipFireTarget = InTarget;

	// Temporarily enter aim-facing orientation so the body turns toward control yaw
	Character->SetOrientationForAiming(true);
}

void UCombatComponent::UpdatePendingHipFireShot(float DeltaTime)
{
	if (!bPendingHipFireShot || !Character) return;

	const FRotator CurrentRotation = Character->GetActorRotation();
	const FRotator ControlRotation = Character->GetControlRotation();

	const FRotator DesiredRotation(
		CurrentRotation.Pitch,
		ControlRotation.Yaw,
		CurrentRotation.Roll
	);

	const FRotator NewRotation = FMath::RInterpTo(
		CurrentRotation,
		DesiredRotation,
		DeltaTime,
		HipFireTurnInterpSpeed
	);

	Character->SetActorRotation(NewRotation);

	const float YawDelta = GetHipFireYawDeltaToControl();
	if (YawDelta <= HipFireTurnYawThreshold)
	{
		ExecutePendingHipFireShot();
	}
}

void UCombatComponent::ExecutePendingHipFireShot()
{
	if (!bPendingHipFireShot || !EquippedWeapon) return;

	const FVector FinalHitTarget = ComputeFinalHitTarget();
	
	EquippedWeapon->Fire(FinalHitTarget);
	ServerFire(FinalHitTarget);

	if (Character && !Character->HasAuthority())
	{
		const float SnappedYaw = Character->GetActorRotation().Yaw;
		Character->ServerSetFacingYaw(SnappedYaw);
	}

	bPendingHipFireShot = false;
	PendingHipFireTarget = FVector::ZeroVector;

	if (Character && !bAiming)
	{
		Character->SetOrientationForAiming(false);
	}
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

void UCombatComponent::SetAimAccuracyBonus(float InMultiplier)
{
	AimAccuracyBonusMultiplier = FMath::Clamp(InMultiplier, 0.1f, 2.0f);

	// Apply immediately to the equipped weapon if one is active
	if (EquippedWeapon)
	{
		EquippedWeapon->ApplySpreadMultiplier(AimAccuracyBonusMultiplier);
	}

	UE_LOG(LogTemp, Log,
		TEXT("UCombatComponent::SetAimAccuracyBonus — Multiplier set to %.2f"),
		AimAccuracyBonusMultiplier);
}


void UCombatComponent::EquipSuppressor()
{
	if (!EquippedWeapon) return;
	if (EquippedWeapon->HasSuppressor()) return;

	EquippedWeapon->SetSuppressor(true);

	if (EquippedWeapon->WeaponAudioComp)
	{
		EquippedWeapon->WeaponAudioComp->PlayEquipUnequipSuppressor_ForMultiplayer();
	}

	if (Character && !Character->HasAuthority())
	{
		ServerToggleSuppressor(true);
	}
}

void UCombatComponent::RemoveSuppressor()
{
	if (!EquippedWeapon) return;
	if (!EquippedWeapon->HasSuppressor()) return;

	EquippedWeapon->SetSuppressor(false);

	if (EquippedWeapon->WeaponAudioComp)
	{
		EquippedWeapon->WeaponAudioComp->PlayEquipUnequipSuppressor_ForMultiplayer();
	}

	if (Character && !Character->HasAuthority())
	{
		ServerToggleSuppressor(false);
	}
}

bool UCombatComponent::CurrentWeaponHasSuppressor() const
{
	return EquippedWeapon ? EquippedWeapon->HasSuppressor() : false;
}

void UCombatComponent::ToggleSuppressor()
{
	// Guard: must have an equipped weapon
	if (!EquippedWeapon) return;

	if (EquippedWeapon->HasSuppressor())
	{
		RemoveSuppressor();
	}
	else
	{
		EquipSuppressor();
	}
}


// -----------------------------------------------------------------------
// Server RPCs — Audio replication for locally-initiated actions
// -----------------------------------------------------------------------

void UCombatComponent::ServerPlayDryFire_Implementation()
{
	// Server received dry fire from owning client — multicast audio to all other clients
	if (EquippedWeapon && EquippedWeapon->WeaponAudioComp)
	{
		EquippedWeapon->WeaponAudioComp->PlayDryFire_ForMultiplayer();
	}
}
bool UCombatComponent::ServerPlayDryFire_Validate() { return true; }

void UCombatComponent::ServerCycleFireMode_Implementation()
{
	// Server received fire mode switch from owning client — multicast audio to all other clients
	if (EquippedWeapon && EquippedWeapon->WeaponAudioComp)
	{
		EquippedWeapon->WeaponAudioComp->PlaySwitchFireMode_ForMultiplayer();
	}
}
bool UCombatComponent::ServerCycleFireMode_Validate() { return true; }

void UCombatComponent::ServerToggleSuppressor_Implementation(bool bEquipping)
{
	// Server received suppressor toggle from owning client — multicast audio to all other clients
	if (EquippedWeapon && EquippedWeapon->WeaponAudioComp)
	{
		EquippedWeapon->WeaponAudioComp->PlayEquipUnequipSuppressor_ForMultiplayer();
	}
}
bool UCombatComponent::ServerToggleSuppressor_Validate(bool bEquipping) { return true; }

bool UCombatComponent::IsReloadAnimationActive() const
{
	return CurrentCombatAction == ECombatAction::Reloading || bLocalReloadPending;
}

EPlayerWeaponStance UCombatComponent::GetPlayerWeaponStance() const
{
	if (!EquippedWeapon)
	{
		return EPlayerWeaponStance::EPWS_Unarmed;
	}

	if (EquippedWeapon->IsPistolClass())
	{
		return EPlayerWeaponStance::EPWS_Pistol;
	}

	return EPlayerWeaponStance::EPWS_Rifle;
}

void UCombatComponent::SetCombatAction(ECombatAction NewAction)
{
	CurrentCombatAction = NewAction;
	bIsBusy = (NewAction != ECombatAction::None);
}

void UCombatComponent::OnRep_CombatAction()
{
	// Anim instance on simulated proxies polls GetCombatAction() directly.
	// No additional work needed here yet — Milestone 5 will consume this.
}


void UCombatComponent::OnLoadoutUpdated(const FLoadoutData& NewLoadout)
{
	UE_LOG(LogTemp, Warning, TEXT("[Combat] OnLoadoutUpdated fired — %d slots, HasAuthority: %d"),
		NewLoadout.Slots.Num(), Character ? (int32)Character->HasAuthority() : -1);

	if (!Character || !Character->HasAuthority()) return;

	if (EquippedWeapon)
	{
		EquippedWeapon->Destroy();
		EquippedWeapon = nullptr;
	}

	for (const FLoadoutSlot& Slot : NewLoadout.Slots)
	{
		if (!Slot.IsOccupied()) continue;
		UE_LOG(LogTemp, Warning, TEXT("[Combat] Found occupied slot, spawning weapon"));
		SpawnAndEquipWeaponFromSlot(Slot);
		break;
	}
}

void UCombatComponent::SpawnAndEquipWeaponFromSlot(const FLoadoutSlot& Slot)
{
	UClass* ResolvedClass = Slot.ItemClass.LoadSynchronous();
	if (!ResolvedClass || !Character) return;

	AWeapon* SpawnedWeapon = GetWorld()->SpawnActorDeferred<AWeapon>(
		ResolvedClass,
		FTransform::Identity,
		Character,
		Character,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn
	);

	if (!SpawnedWeapon) return;

	// Get config from the weapon's CDO
	if (UWeaponConfig* CDOConfig = Cast<AWeapon>(ResolvedClass->GetDefaultObject())->GetWeaponConfig())
	{
		SpawnedWeapon->InitFromConfig(CDOConfig);
	}

	UGameplayStatics::FinishSpawningActor(SpawnedWeapon, FTransform::Identity);

	EquipWeapon(SpawnedWeapon);

	UInventoryComponent* Inventory = Character->GetInventory();
	if (Inventory)
	{
		const EAmmoType AmmoType = SpawnedWeapon->GetSupportedAmmoType();
		FMagazine* BestMag = Inventory->GetBestMagazine(AmmoType);
		if (BestMag)
		{
			SpawnedWeapon->InsertMagazine(*BestMag);
			Inventory->RemoveMagazine(*BestMag);
			SpawnedWeapon->ChamberRound();
			SpawnedWeapon->OnAmmoChanged.Broadcast(
				SpawnedWeapon->GetLoadedRounds(),
				SpawnedWeapon->GetMagCapacity());
		}
	}
}