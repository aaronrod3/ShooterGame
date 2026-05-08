#include "Framework/Subsystems/ShooterSaveGame.h"

const FString UShooterSaveGame::SaveSlotName = TEXT("ShooterGame_Loadout");

UShooterSaveGame::UShooterSaveGame()
{
	SaveVersion = CurrentSaveVersion;
	InitializeDefaultLoadoutPresets();
}

void UShooterSaveGame::InitializeDefaultLoadoutPresets()
{
	SavedLoadoutPresets.Reset();
	SavedLoadoutPresets.SetNum(3);

	for (int32 PresetIndex = 0; PresetIndex < SavedLoadoutPresets.Num(); ++PresetIndex)
	{
		FLoadoutPreset& Preset = SavedLoadoutPresets[PresetIndex];
		Preset.PresetName = FString::Printf(TEXT("Preset %d"), PresetIndex + 1);
		Preset.InitializeDefaultSlots();
	}
}

void UShooterSaveGame::ResetPersistentInventoryState()
{
	SavedStashItems.Reset();
	SavedEquippedState.ClearAll();
	bHasUnreviewedExtraction = false;

	InitializeDefaultLoadoutPresets();
}