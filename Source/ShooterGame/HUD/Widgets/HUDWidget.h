// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HUDWidget.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTERGAME_API UHUDWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD | Pickup")
	void ShowPickupMessage(const FString& Message);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD | Pickup")
	void HidePickupMessage();
	
	// ── Ammo counter ────────────────────────────────────────────────────
	/**
	 * Called whenever the equipped weapon's ammo state changes.
	 * Implement in the WBP_HUDWidget Blueprint to update the counter text.
	 *
	 * @param MagRounds    Rounds currently in the inserted magazine
	 * @param MagCapacity  Capacity of the inserted magazine (0 = no mag)
	 *
	 * Display pattern: "[MagRounds] / [MagCapacity]"
	 * e.g. "17 / 30"   "0 / 30"   "0 / 0" (no mag)
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Ammo")
	void UpdateAmmoDisplay(int32 MagRounds, int32 MagCapacity);
	
};
