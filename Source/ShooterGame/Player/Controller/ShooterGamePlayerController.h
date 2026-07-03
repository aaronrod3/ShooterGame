// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ShooterGame/Player/Spectator/ShooterGameSpectatorPawn.h"
#include "ShooterGame/HUD/EndScreen/ShooterEndScreenWidget.h"
#include "ShooterGamePlayerController.generated.h"

class UInventoryComponent;
class UHUDWidget;
class UInputMappingContext;
class UInputAction;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
UCLASS(abstract)
class AShooterGamePlayerController : public APlayerController
{
	GENERATED_BODY()
	
	
public:
	AShooterGamePlayerController();
	virtual void Tick(float DeltaTime) override;
	
	// Called by GameMode when this player's bleedout expires
	void PossessSpectatorPawn(const TArray<AShooterGameCharacter*>& AlivePlayers);
	
	// Called by GameMode when alive player list changes
	void UpdateSpectatorTargets(const TArray<AShooterGameCharacter*>& AlivePlayers);

	FORCEINLINE AShooterGameSpectatorPawn* GetActiveSpectatorPawn() const
	{
		return ActiveSpectatorPawn;
	}

	// Spectator pawn class — set in BP defaults
	UPROPERTY(EditDefaultsOnly, Category = "Spectator")
	TSubclassOf<AShooterGameSpectatorPawn> SpectatorPawnClass;
	
	
	// -----------------------------------------------------------------------
	// Match flow — called by GameMode
	// -----------------------------------------------------------------------

	// Fires when all players are dead — disables input, shows end screen
	void HandleMatchOver();

	// Fires when match restarts — re-enables input, hides end screen
	void HandleMatchRestart();

	// Called by end screen "Play Again" button — tells GameMode to restart early
	UFUNCTION(BlueprintCallable, Category = "Match")
	void RequestMatchRestart();

	// -----------------------------------------------------------------------
	// End screen widget
	// -----------------------------------------------------------------------

	// Set in BP defaults — the WBP_EndScreen class to spawn
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UShooterEndScreenWidget> EndScreenWidgetClass;
	
protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	
private:
	
	TWeakObjectPtr<UInventoryComponent> InventoryComponent;
	
	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input")
	TArray<UInputMappingContext*> DefaultIMCs;

	void CreateHUDWidget();
	void TraceForItem();
	void UpdateCursorVisibility();
	bool bWasEquipped = false;
	bool bMatchOverHandled = false;
	
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TSubclassOf<UHUDWidget> HUDWidgetClass;
	UPROPERTY()
	TObjectPtr<UHUDWidget> HUDWidget;
	
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	double TraceLength;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TEnumAsByte<ECollisionChannel> ItemTraceChannel;
	TWeakObjectPtr<AActor> ThisActor;
	TWeakObjectPtr<AActor> LastActor;
	
	
	UPROPERTY()
	AShooterGameSpectatorPawn* ActiveSpectatorPawn = nullptr;
	
	
	// Live instance of the end screen widget
	UPROPERTY()
	UShooterEndScreenWidget* EndScreenWidget = nullptr;
	
	// RPC — client requests early restart from server
	UFUNCTION(Server, Reliable)
	void ServerRequestMatchRestart();
	
};
