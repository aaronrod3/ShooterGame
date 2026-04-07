// DownedComponent.h
// Owns the downed state machine, bleedout timer, activity-based drain rate,
// hit spike system, and self-revive flow for AShooterGameCharacter.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShooterGame/Types/PlayerCombatState.h"
#include "DownedComponent.generated.h"

// Broadcast to UReviveComponent and HUD when state changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatStateChanged, EPlayerCombatState, NewState);

// Broadcast each tick while downed so HUD can update bleedout bar
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBleedoutUpdated, float, TimeRemaining, float, MaxTime);

class AShooterGameCharacter;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SHOOTERGAME_API UDownedComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDownedComponent();

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// -----------------------------------------------------------------------
	// Public API — called by AShooterGameCharacter, UReviveComponent
	// -----------------------------------------------------------------------

	// Server — transition from Alive → Downed
	UFUNCTION(Server, Reliable)
	void ServerGoDown();

	// Server — called by UReviveComponent when revive completes
	// ReviveHealth: HP to restore on revival
	UFUNCTION(Server, Reliable)
	void ServerCompleteRevive(float InReviveHealth);

	// Server — called by AProjectile::OnHit when downed player is hit
	// Triggers a bleedout rate spike for HitSpikeDuration seconds
	UFUNCTION(Server, Reliable)
	void ServerApplyHitSpike();

	// Called by UReviveComponent on the reviver to pause/resume bleedout
	void PauseBleedout();
	void ResumeBleedout();

	// Called by UCombatComponent to inform this component the player fired
	void NotifyFired();

	// -----------------------------------------------------------------------
	// State accessors — safe to call from any context
	// -----------------------------------------------------------------------

	UFUNCTION(BlueprintPure)
	EPlayerCombatState GetCombatState() const { return CombatState; }

	UFUNCTION(BlueprintPure)
	bool IsAlive()   const { return CombatState == EPlayerCombatState::EPCS_Alive; }

	UFUNCTION(BlueprintPure)
	bool IsDowned()  const { return CombatState == EPlayerCombatState::EPCS_Downed; }

	UFUNCTION(BlueprintPure)
	bool IsDead()    const { return CombatState == EPlayerCombatState::EPCS_Dead; }

	UFUNCTION(BlueprintPure)
	float GetBleedoutTimeRemaining() const { return BleedoutTimeRemaining; }

	UFUNCTION(BlueprintPure)
	float GetBleedoutDuration()      const { return BleedoutDuration; }

	// -----------------------------------------------------------------------
	// Self-revive — called by UCombatComponent / input binding
	// -----------------------------------------------------------------------

	// Begin self-revive hold (no nearby reviver)
	void BeginSelfRevive();

	// Cancel self-revive (player moved, took damage, etc.)
	void CancelSelfRevive();

	// -----------------------------------------------------------------------
	// Delegates — bind from HUD, other components
	// -----------------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "Downed|Events")
	FOnCombatStateChanged OnCombatStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Downed|Events")
	FOnBleedoutUpdated OnBleedoutUpdated;

	// -----------------------------------------------------------------------
	// Configuration — set in BP defaults per character class
	// -----------------------------------------------------------------------

	// Total bleedout time pool (seconds)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Downed|Bleedout")
	float BleedoutDuration = 60.f;

	// Debuff rates, crawl speed, weapon multipliers — all configurable in editor
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Downed|Config")
	FDownedDebuffConfig DebuffConfig;

	// Revive health restored on teammate revival (small amount)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Downed|Revive")
	float ReviveHealth = 30.f;

	// Self-revive duration (longer than teammate revive — set on UReviveComponent)
	// Exposed here for reference only; actual timer lives on UReviveComponent
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Downed|Revive")
	float SelfReviveDuration = 10.f;

protected:
	virtual void BeginPlay() override;

private:
	
	float LastLoggedThreshold = -1.f;
	
	
	// -----------------------------------------------------------------------
	// State machine
	// -----------------------------------------------------------------------

	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	EPlayerCombatState CombatState = EPlayerCombatState::EPCS_Alive;

	UFUNCTION()
	void OnRep_CombatState();

	// Apply state visuals/movement changes locally on all clients
	void ApplyDownedState();
	void ApplyDeadState();
	void ApplyAliveState(float RestoredHealth);

	// -----------------------------------------------------------------------
	// Bleedout tick
	// -----------------------------------------------------------------------

	// Remaining bleedout time — ticks down on server, replicated for HUD
	UPROPERTY(ReplicatedUsing = OnRep_BleedoutTimeRemaining)
	float BleedoutTimeRemaining = 0.f;
	
	void LogBleedoutCountdown();

	UFUNCTION()
	void OnRep_BleedoutTimeRemaining();

	// Whether bleedout is currently paused (teammate is actively reviving)
	bool bBleedoutPaused = false;

	// Whether the player fired this frame — set by NotifyFired(), cleared each tick
	bool bFiredThisTick = false;

	// Hit spike — remaining duration of the current spike
	float HitSpikeTimeRemaining = 0.f;

	// Self-revive in progress flag
	bool bSelfReviveInProgress = false;
	FTimerHandle SelfReviveTimerHandle;

	// Compute current drain rate based on activity + spike state
	float GetCurrentBleedoutRate() const;

	// Called when BleedoutTimeRemaining reaches 0
	void OnBleedoutExpired();
	

	// Owning character — cached in BeginPlay
	UPROPERTY()
	AShooterGameCharacter* Character = nullptr;
};