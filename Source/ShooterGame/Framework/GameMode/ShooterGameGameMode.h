// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ShooterGame/Player/Character/ShooterGameCharacter.h"
#include "ShooterGameGameMode.generated.h"

class AShooterGamePlayerController;

UCLASS(abstract)
class AShooterGameGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	AShooterGameGameMode();
	
	virtual void PostLogin(APlayerController* NewPlayer) override;

	// -----------------------------------------------------------------------
	// Lifecycle
	// -----------------------------------------------------------------------

	virtual void RestartPlayer(AController* NewPlayer) override;

	// -----------------------------------------------------------------------
	// Combat events — called by DownedComponent (server only)
	// -----------------------------------------------------------------------

	// Called when a player enters downed state
	virtual void PlayerDowned(
		AShooterGameCharacter* DownedCharacter,
		AShooterGamePlayerController* VictimController);

	// Called when bleedout expires — triggers spectator possession
	virtual void PlayerDied(
		AShooterGameCharacter* DeadCharacter,
		AShooterGamePlayerController* VictimController);

	// Called when a player is successfully revived
	UFUNCTION(BlueprintCallable, Category = "Game Rules")
	void PlayerRevived(AShooterGameCharacter* RevivedPlayer);

	// -----------------------------------------------------------------------
	// Alive player list — used by spectator system
	// -----------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Game Rules")
	const TArray<AShooterGameCharacter*>& GetAlivePlayersList() const
	{
		return AlivePlayers;
	}

	// -----------------------------------------------------------------------
	// Friendly fire
	// -----------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Rules")
	bool bFriendlyFireEnabled = true;

	UFUNCTION(BlueprintCallable, Category = "Game Rules")
	void SetFriendlyFireEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Game Rules")
	bool IsFriendlyFireEnabled() const { return bFriendlyFireEnabled; }

	// PIE console command — type ToggleFriendlyFire in the backtick console
	UFUNCTION(Exec)
	void ToggleFriendlyFire();
	
	// -----------------------------------------------------------------------
	// Match flow
	// -----------------------------------------------------------------------

	// Called internally when AlivePlayers reaches 0
	void HandleMatchOver();

	// Called by end screen "Play Again" button — resets all players in-place
	UFUNCTION(BlueprintCallable, Category = "Match")
	void RestartMatch();

	// How long to wait on end screen before auto-restarting (fallback if no button press)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match")
	float PostMatchDelay = 10.f;

	
	UFUNCTION(BlueprintPure, Category = "Match")
	bool IsMatchInProgress() const { return bMatchInProgress; }
	

private:

	// Tracks all currently alive player characters — populated in RestartPlayer
	UPROPERTY()
	TArray<AShooterGameCharacter*> AlivePlayers;
	
	// Timer handle for auto-restart after PostMatchDelay
	FTimerHandle PostMatchTimerHandle;

	// Tracks whether match is currently active — blocks damage, pickups, etc. when false
	bool bMatchInProgress = false;
};