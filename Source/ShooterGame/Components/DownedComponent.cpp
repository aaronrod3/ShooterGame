#include "DownedComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "ShooterGame/Player/Character/ShooterGameCharacter.h"
#include "ShooterGame/Framework/GameMode/ShooterGameGameMode.h"
#include "ShooterGame/Player/Controller/ShooterGamePlayerController.h"

UDownedComponent::UDownedComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UDownedComponent::BeginPlay()
{
	Super::BeginPlay();
	Character = Cast<AShooterGameCharacter>(GetOwner());
}

void UDownedComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UDownedComponent, CombatState);
	DOREPLIFETIME(UDownedComponent, BleedoutTimeRemaining);
}

// -----------------------------------------------------------------------
// Tick — bleedout drain runs on server only, rep notifies clients
// -----------------------------------------------------------------------

void UDownedComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetOwner()->HasAuthority() || CombatState != EPlayerCombatState::EPCS_Downed) return;
	if (bBleedoutPaused) return;

	// Tick down hit spike timer
	if (HitSpikeTimeRemaining > 0.f)
	{
		HitSpikeTimeRemaining = FMath::Max(0.f, HitSpikeTimeRemaining - DeltaTime);
	}

	// Drain bleedout at current activity-based rate
	const float DrainRate = GetCurrentBleedoutRate();
	BleedoutTimeRemaining = FMath::Max(0.f, BleedoutTimeRemaining - DrainRate * DeltaTime);

	// Broadcast for HUD (server-local — clients receive via OnRep)
	OnBleedoutUpdated.Broadcast(BleedoutTimeRemaining, BleedoutDuration);

	LogBleedoutCountdown();

	if (BleedoutTimeRemaining <= 0.f)
	{
		OnBleedoutExpired();
	}

	// Clear fired flag — set again next frame if player fires
	bFiredThisTick = false;
}

// -----------------------------------------------------------------------
// Bleedout rate — activity state machine
// -----------------------------------------------------------------------

float UDownedComponent::GetCurrentBleedoutRate() const
{
	// Hit spike overrides everything
	if (HitSpikeTimeRemaining > 0.f)
	{
		return DebuffConfig.HitSpikeRate;
	}

	if (!Character) return DebuffConfig.BleedoutRate_Idle;

	const float Speed = Character->GetVelocity().Size2D();
	const bool bMoving = Speed > DebuffConfig.IdleSpeedThreshold;
	const bool bFiring = bFiredThisTick;

	if (bMoving && bFiring)  return DebuffConfig.BleedoutRate_Active;
	if (bMoving)             return DebuffConfig.BleedoutRate_Moving;
	if (bFiring)             return DebuffConfig.BleedoutRate_Firing;
	return                          DebuffConfig.BleedoutRate_Idle;
}

// -----------------------------------------------------------------------
// GoDown — server only
// -----------------------------------------------------------------------

void UDownedComponent::ServerGoDown_Implementation()
{
	if (CombatState != EPlayerCombatState::EPCS_Alive) return;

	CombatState = EPlayerCombatState::EPCS_Downed;
	BleedoutTimeRemaining = BleedoutDuration;
	bBleedoutPaused = false;
	HitSpikeTimeRemaining = 0.f;
	LastLoggedThreshold = BleedoutDuration + 1.f;

	// Notify game mode
	if (AShooterGameGameMode* GM = GetWorld()->GetAuthGameMode<AShooterGameGameMode>())
	{
		AShooterGamePlayerController* PC =
			Cast<AShooterGamePlayerController>(Character ? Character->GetController() : nullptr);
		GM->PlayerDowned(Character, PC);
	}

	// Apply downed visuals/movement on server — clients handle via OnRep_CombatState
	ApplyDownedState();

	UE_LOG(LogTemp, Warning, TEXT("UDownedComponent::ServerGoDown — %s is downed. Bleedout: %.1fs"),
		Character ? *Character->GetName() : TEXT("Unknown"),
		BleedoutDuration);
}

// -----------------------------------------------------------------------
// Hit spike — called by AProjectile::OnHit when downed player takes damage
// -----------------------------------------------------------------------

void UDownedComponent::ServerApplyHitSpike_Implementation()
{
	if (CombatState != EPlayerCombatState::EPCS_Downed) return;

	HitSpikeTimeRemaining = DebuffConfig.HitSpikeDuration;

	UE_LOG(LogTemp, Log, TEXT("UDownedComponent::ServerApplyHitSpike — Spike active for %.1fs at rate %.1f"),
		DebuffConfig.HitSpikeDuration,
		DebuffConfig.HitSpikeRate);
}

// -----------------------------------------------------------------------
// Bleedout pause/resume — called by UReviveComponent on reviver
// -----------------------------------------------------------------------

void UDownedComponent::PauseBleedout()
{
	bBleedoutPaused = true;
	UE_LOG(LogTemp, Log, TEXT("UDownedComponent::PauseBleedout — bleedout paused (%.1fs remaining)"),
		BleedoutTimeRemaining);
}

void UDownedComponent::ResumeBleedout()
{
	bBleedoutPaused = false;
	UE_LOG(LogTemp, Log, TEXT("UDownedComponent::ResumeBleedout — bleedout resumed (%.1fs remaining)"),
		BleedoutTimeRemaining);
}

// -----------------------------------------------------------------------
// Notify fired — called by UCombatComponent each time a shot is fired
// -----------------------------------------------------------------------

void UDownedComponent::NotifyFired()
{
	bFiredThisTick = true;
	UE_LOG(LogTemp, Log, TEXT("UDownedComponent::NotifyFired — bleedout rate spiked by firing"));
}

// -----------------------------------------------------------------------
// Complete revive — called by UReviveComponent when hold timer finishes
// -----------------------------------------------------------------------

void UDownedComponent::ServerCompleteRevive_Implementation(float InReviveHealth)
{
	if (CombatState != EPlayerCombatState::EPCS_Downed) return;

	CombatState = EPlayerCombatState::EPCS_Alive;
	BleedoutTimeRemaining = 0.f;
	bBleedoutPaused = false;
	bSelfReviveInProgress = false;
	GetWorld()->GetTimerManager().ClearTimer(SelfReviveTimerHandle);

	ApplyAliveState(InReviveHealth);

	// Notify GameMode — restores player to alive list and updates spectators
	if (AShooterGameGameMode* GM = GetWorld()->GetAuthGameMode<AShooterGameGameMode>())
	{
		GM->PlayerRevived(Character);
	}

	UE_LOG(LogTemp, Warning, TEXT("UDownedComponent::ServerCompleteRevive — %s revived with %.1f HP"),
		Character ? *Character->GetName() : TEXT("Unknown"),
		InReviveHealth);
}

// -----------------------------------------------------------------------
// Self-revive — called by input when no reviver is nearby
// -----------------------------------------------------------------------

void UDownedComponent::BeginSelfRevive()
{
	if (CombatState != EPlayerCombatState::EPCS_Downed) return;
	if (bSelfReviveInProgress) return;

	const bool bCanSelfRevive = true; // Medical kit stub — hook kit check here later
	if (!bCanSelfRevive) return;

	bSelfReviveInProgress = true;
	PauseBleedout();

	UE_LOG(LogTemp, Log, TEXT("UDownedComponent::BeginSelfRevive — self-revive started (%.1fs)"),
		SelfReviveDuration);

	GetWorld()->GetTimerManager().SetTimer(
		SelfReviveTimerHandle,
		[this]()
		{
			ServerCompleteRevive(ReviveHealth);
		},
		SelfReviveDuration,
		false
	);
}

void UDownedComponent::CancelSelfRevive()
{
	if (!bSelfReviveInProgress) return;

	bSelfReviveInProgress = false;
	GetWorld()->GetTimerManager().ClearTimer(SelfReviveTimerHandle);
	ResumeBleedout();

	UE_LOG(LogTemp, Log, TEXT("UDownedComponent::CancelSelfRevive — self-revive cancelled"));
}

// -----------------------------------------------------------------------
// Bleedout countdown logger
// -----------------------------------------------------------------------

void UDownedComponent::LogBleedoutCountdown()
{
	static const TArray<float> Thresholds = {
		50.f, 40.f, 30.f, 20.f, 10.f,
		5.f, 4.f, 3.f, 2.f, 1.f
	};

	for (float Threshold : Thresholds)
	{
		if (BleedoutTimeRemaining <= Threshold && LastLoggedThreshold > Threshold)
		{
			LastLoggedThreshold = Threshold;

			if (Threshold <= 5.f)
			{
				UE_LOG(LogTemp, Warning,
					TEXT("⚠️ BLEEDOUT — %s: %.0f second%s remaining!"),
					Character ? *Character->GetName() : TEXT("Unknown"),
					Threshold,
					Threshold == 1.f ? TEXT("") : TEXT("s"));
			}
			else
			{
				UE_LOG(LogTemp, Log,
					TEXT("UDownedComponent — %s bleedout: %.0fs remaining"),
					Character ? *Character->GetName() : TEXT("Unknown"),
					Threshold);
			}
			break;
		}
	}
}

// -----------------------------------------------------------------------
// Bleedout expiry — permanent death
// -----------------------------------------------------------------------

void UDownedComponent::OnBleedoutExpired()
{
	if (CombatState != EPlayerCombatState::EPCS_Downed) return;

	// -----------------------------------------------------------------------
	// Guard — if match is already over, skip the GameMode notify
	// Prevents a second bleedout from double-firing HandleMatchOver
	// -----------------------------------------------------------------------
	if (AShooterGameGameMode* GM = GetWorld()->GetAuthGameMode<AShooterGameGameMode>())
	{
		if (!GM->IsMatchInProgress())
		{
			UE_LOG(LogTemp, Log,
				TEXT("UDownedComponent::OnBleedoutExpired — match already over, skipping PlayerDied"));
			CombatState = EPlayerCombatState::EPCS_Dead;
			ApplyDeadState();
			return;
		}
	}

	CombatState = EPlayerCombatState::EPCS_Dead;
	ApplyDeadState();

	if (AShooterGameGameMode* GM = GetWorld()->GetAuthGameMode<AShooterGameGameMode>())
	{
		AShooterGamePlayerController* PC =
			Cast<AShooterGamePlayerController>(Character ? Character->GetController() : nullptr);
		GM->PlayerDied(Character, PC);
	}

	UE_LOG(LogTemp, Warning, TEXT("UDownedComponent::OnBleedoutExpired — %s has died (bleedout)"),
		Character ? *Character->GetName() : TEXT("Unknown"));
}

// -----------------------------------------------------------------------
// State application — runs locally on server + clients via rep notify
// -----------------------------------------------------------------------

void UDownedComponent::ApplyDownedState()
{
	if (!Character) return;

	if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = DebuffConfig.CrawlSpeed;
	}

	OnCombatStateChanged.Broadcast(EPlayerCombatState::EPCS_Downed);

	UE_LOG(LogTemp, Log, TEXT("UDownedComponent::ApplyDownedState — crawl speed: %.1f"),
		DebuffConfig.CrawlSpeed);
}

void UDownedComponent::ApplyDeadState()
{
	if (!Character) return;

	if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
	{
		Movement->DisableMovement();
		Movement->StopMovementImmediately();
	}

	Character->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
	{
		Character->DisableInput(PC);
	}

	OnCombatStateChanged.Broadcast(EPlayerCombatState::EPCS_Dead);
}

void UDownedComponent::ApplyAliveState(float RestoredHealth)
{
	if (!Character) return;

	if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = Character->GetBaseWalkSpeed();
	}

	Character->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
	{
		Character->EnableInput(PC);
	}

	Character->SetHealth(RestoredHealth);

	OnCombatStateChanged.Broadcast(EPlayerCombatState::EPCS_Alive);
}

// -----------------------------------------------------------------------
// Rep notifies — run on clients when replicated vars change
// -----------------------------------------------------------------------

void UDownedComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
	case EPlayerCombatState::EPCS_Downed: ApplyDownedState();            break;
	case EPlayerCombatState::EPCS_Dead:   ApplyDeadState();              break;
	case EPlayerCombatState::EPCS_Alive:  ApplyAliveState(ReviveHealth); break;
	default: break;
	}
}

void UDownedComponent::OnRep_BleedoutTimeRemaining()
{
	OnBleedoutUpdated.Broadcast(BleedoutTimeRemaining, BleedoutDuration);
}