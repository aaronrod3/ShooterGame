// Source/ShooterGame/HUD/Inventory/PostExtractionWidget.cpp
#include "HUD/Inventory/PostExtractionWidget.h"

#include "Framework/Subsystems/ShooterSaveGameSubsystem.h"
#include "Framework/Subsystems/ShooterSaveGame.h" 
#include "HUD/Inventory/EquipmentPanelWidget.h"
#include "HUD/Inventory/StashWindowWidget.h"
#include "Inventory/EquippedStateComponent.h"
#include "Inventory/StashComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/Character/ShooterGameCharacter.h"

void UPostExtractionWidget::InitializePostExtractionScreen(AShooterGameCharacter* InPlayerCharacter)
{
	OwningCharacter = InPlayerCharacter;

	UShooterSaveGameSubsystem* SaveSubsystem = GetSaveSubsystem();
	if (!SaveSubsystem || !OwningCharacter)
	{
		return;
	}

	// Load the extraction snapshot from save data — this is what the player
	// extracted with, preserved exactly as-is until manually reorganized
	UShooterSaveGame* SaveGame = SaveSubsystem->GetCachedSaveGame();
	if (SaveGame)
	{
		ExtractionSnapshot = SaveGame->SavedEquippedState;
	}

	// Left panel: equipment state as-extracted — bound to live EquippedStateComponent
	// so drag operations update it immediately
	if (ExtractionEquipmentPanel && OwningCharacter->GetEquippedState())
	{
		ExtractionEquipmentPanel->InitializeEquipmentPanel(OwningCharacter->GetEquippedState());
	}

	// Right panel: stash — player drags extracted items here to reorganize
	if (StashPanel && OwningCharacter->GetStash())
	{
		StashPanel->InitializeStashWindow(OwningCharacter->GetStash());
	}

	BP_OnPostExtractionInitialized(ExtractionSnapshot);
}

void UPostExtractionWidget::CommitAndClose()
{
	UShooterSaveGameSubsystem* SaveSubsystem = GetSaveSubsystem();
	if (SaveSubsystem && OwningCharacter)
	{
		// Commit whatever state the player has reorganized to disk
		if (OwningCharacter->GetStash())
		{
			SaveSubsystem->SaveStash(OwningCharacter->GetStash());
		}

		if (OwningCharacter->GetEquippedState())
		{
			SaveSubsystem->SaveEquippedState(OwningCharacter->GetEquippedState());
		}

		// Clear the unreviewed extraction flag — player has reviewed their kit
		SaveSubsystem->SetHasUnreviewedExtraction(false);
	}

	BP_OnCommitted();
	RemoveFromParent();
}

bool UPostExtractionWidget::HasUnreviewedExtraction() const
{
	if (UShooterSaveGameSubsystem* SaveSubsystem = GetSaveSubsystem())
	{
		return SaveSubsystem->GetHasUnreviewedExtraction();
	}

	return false;
}

void UPostExtractionWidget::NativeDestruct()
{
	OwningCharacter = nullptr;
	Super::NativeDestruct();
}

UShooterSaveGameSubsystem* UPostExtractionWidget::GetSaveSubsystem() const
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		return GameInstance->GetSubsystem<UShooterSaveGameSubsystem>();
	}

	return nullptr;
}