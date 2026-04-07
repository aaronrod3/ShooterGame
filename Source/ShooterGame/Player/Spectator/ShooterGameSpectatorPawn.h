// ShooterGameSpectatorPawn.h
// Possessed by AShooterGamePlayerController when a player permanently dies (bleedout expires).
// Currently supports FollowPlayer mode — cycles through alive teammates.
// ESpectatorMode enum is extensible for future modes (overhead, free cam, etc.)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpectatorPawn.h"
#include "ShooterGame/Types/PlayerCombatState.h"
#include "InputAction.h"  
#include "ShooterGameSpectatorPawn.generated.h"

class AShooterGameCharacter;
class AShooterGamePlayerController;

UCLASS()
class SHOOTERGAME_API AShooterGameSpectatorPawn : public ASpectatorPawn
{
	GENERATED_BODY()

public:
	AShooterGameSpectatorPawn();

	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// -----------------------------------------------------------------------
	// Public API — called by AShooterGamePlayerController
	// -----------------------------------------------------------------------

	// Initialize spectator with the list of living players to follow
	// Called by GameMode immediately after possession
	void InitSpectator(const TArray<AShooterGameCharacter*>& AlivePlayers);

	// Cycle to the next alive player (forward)
	UFUNCTION(BlueprintCallable, Category = "Spectator")
	void CycleToNextTarget();

	// Cycle to the previous alive player (backward)
	UFUNCTION(BlueprintCallable, Category = "Spectator")
	void CycleToPreviousTarget();

	// Called by GameMode when a new player dies or respawns — refreshes target list
	void UpdateAlivePlayerList(const TArray<AShooterGameCharacter*>& AlivePlayers);

	// -----------------------------------------------------------------------
	// State accessors
	// -----------------------------------------------------------------------

	UFUNCTION(BlueprintPure)
	AShooterGameCharacter* GetCurrentTarget() const { return CurrentTarget; }

	UFUNCTION(BlueprintPure)
	ESpectatorMode GetSpectatorMode() const { return SpectatorMode; }

	// -----------------------------------------------------------------------
	// Configuration
	// -----------------------------------------------------------------------

	// Input action for cycling forward through spectate targets
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* SpectateNextAction;

	// Input action for cycling backward through spectate targets
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* SpectatePrevAction;

protected:
	// Sets the camera to follow CurrentTarget
	void AttachToTarget(AShooterGameCharacter* Target);

private:
	// Current spectator mode — extensible via ESpectatorMode enum
	ESpectatorMode SpectatorMode = ESpectatorMode::ESM_FollowPlayer;

	// Ordered list of alive players to cycle through
	UPROPERTY()
	TArray<AShooterGameCharacter*> AliveTargets;

	// Index into AliveTargets currently being followed
	int32 CurrentTargetIndex = 0;

	// Currently followed character
	UPROPERTY()
	AShooterGameCharacter* CurrentTarget = nullptr;

	// Input handlers
	void OnSpectateNext();
	void OnSpectatePrev();
};