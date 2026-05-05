// Source/ShooterGame/Framework/Subsystems/ShooterSaveGame.cpp
#include "ShooterSaveGame.h"

// The save slot filename written to Saved/SaveGames/
const FString UShooterSaveGame::SaveSlotName = TEXT("ShooterGame_Loadout");

UShooterSaveGame::UShooterSaveGame()
{
	// Default values are set in the header via member initializers.
	// SaveVersion and SaveSlotName are the only fields that need
	// special attention here — data fields default via their struct constructors.
}