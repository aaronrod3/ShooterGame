// Source/ShooterGame/Framework/Subsystems/ShooterSaveGameSubsystem.cpp
#include "ShooterSaveGameSubsystem.h"
#include "ShooterSaveGame.h"
#include "ShooterGame/Framework/PlayerState/ShooterPlayerState.h"
#include "ShooterGame/Framework/Subsystems/QuestTrackerSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Inventory/EquippedStateComponent.h"
#include "Inventory/InventoryComponent.h"
#include "HAL/FileManager.h"

void UShooterSaveGameSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Load or create save data immediately — before any level or PlayerState exists.
    // CachedSaveGame is guaranteed valid after this call.
    LoadLoadout();

#if !UE_BUILD_SHIPPING
    // -----------------------------------------------------------------------
    // Debug console commands
    // Available in PIE and non-shipping builds via the Output Log console (~)
    // Stripped completely from shipping builds by the preprocessor.
    // -----------------------------------------------------------------------
    IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("ShooterGame.SaveLoadout"),
        TEXT("Force-write the current loadout to disk immediately."),
        FConsoleCommandDelegate::CreateUObject(this, &UShooterSaveGameSubsystem::DebugSaveLoadout),
        ECVF_Default
    );

    IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("ShooterGame.LoadLoadout"),
        TEXT("Force-read loadout from disk (simulates a fresh login)."),
        FConsoleCommandDelegate::CreateUObject(this, &UShooterSaveGameSubsystem::DebugLoadLoadout),
        ECVF_Default
    );

    IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("ShooterGame.ClearSaveData"),
        TEXT("Deletes the .sav file and resets to defaults — simulates a new player."),
        FConsoleCommandDelegate::CreateUObject(this, &UShooterSaveGameSubsystem::DebugClearSaveData),
        ECVF_Default
    );

    IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("ShooterGame.PrintLoadout"),
        TEXT("Logs the current in-memory loadout state to the Output Log."),
        FConsoleCommandDelegate::CreateUObject(this, &UShooterSaveGameSubsystem::DebugPrintLoadout),
        ECVF_Default
    );
#endif
}

void UShooterSaveGameSubsystem::Deinitialize()
{
    // Flush any unsaved state on application exit.
    // Under normal operation WriteToDisk() is called immediately on every
    // change, so this is a safety net for unexpected shutdowns.
    if (CachedSaveGame)
    {
        WriteToDisk();
    }

    Super::Deinitialize();
}

// ============================================================================
// Public Interface
// ============================================================================

void UShooterSaveGameSubsystem::SaveLoadout(
    const FLoadoutData& Loadout, const FCharacterAppearance& Appearance)
{
    if (!CachedSaveGame)
    {
        UE_LOG(LogTemp, Error,
            TEXT("UShooterSaveGameSubsystem::SaveLoadout — CachedSaveGame is null. This should never happen."));
        return;
    }

    // Update the in-memory cache
    CachedSaveGame->SavedLoadout    = Loadout;
    CachedSaveGame->SavedAppearance = Appearance;

    // Write to disk immediately — loadout changes are infrequent (lobby only)
    // so synchronous save is fine here. If this ever moves to high-frequency
    // saves, add a timer-debounced dirty flag instead.
    WriteToDisk();
}

void UShooterSaveGameSubsystem::PushSaveDataToPlayerState(AShooterPlayerState* PlayerState)
{
    if (!PlayerState)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("UShooterSaveGameSubsystem::PushSaveDataToPlayerState — null PlayerState."));
        return;
    }

    if (!CachedSaveGame)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("UShooterSaveGameSubsystem::PushSaveDataToPlayerState — no save data cached."));
        return;
    }

    // Push saved loadout into PlayerState — bPushToCharacter=false here
    // because the character may not be possessed yet at login time.
    // PossessedBy() in ShooterGameCharacter will pull it when ready.
    PlayerState->SetFullLoadout(CachedSaveGame->SavedLoadout, false);
    PlayerState->Server_UpdateAppearance(CachedSaveGame->SavedAppearance);

    UE_LOG(LogTemp, Log,
        TEXT("UShooterSaveGameSubsystem::PushSaveDataToPlayerState — save data pushed to PlayerState."));
}

bool UShooterSaveGameSubsystem::DoesSaveExist() const
{
    return UGameplayStatics::DoesSaveGameExist(
        UShooterSaveGame::SaveSlotName, UShooterSaveGame::UserIndex);
}

// ============================================================================
// Internal Operations
// ============================================================================

void UShooterSaveGameSubsystem::LoadLoadout()
{
    if (DoesSaveExist())
    {
        UShooterSaveGame* Loaded = Cast<UShooterSaveGame>(
            UGameplayStatics::LoadGameFromSlot(
                UShooterSaveGame::SaveSlotName, UShooterSaveGame::UserIndex));

        if (Loaded)
        {
            // Version check — discard incompatible saves rather than
            // risk corrupting data with mismatched field layouts.
            if (Loaded->SaveVersion != UShooterSaveGame::CurrentSaveVersion)
            {
                UE_LOG(LogTemp, Warning,
                    TEXT("UShooterSaveGameSubsystem — save version mismatch (file: %d, current: %d). Starting fresh."),
                    Loaded->SaveVersion, UShooterSaveGame::CurrentSaveVersion);

                // Create a clean save file at the current version
                CachedSaveGame = Cast<UShooterSaveGame>(
                    UGameplayStatics::CreateSaveGameObject(UShooterSaveGame::StaticClass()));
            }
            else
            {
                CachedSaveGame = Loaded;
                UE_LOG(LogTemp, Log,
                    TEXT("UShooterSaveGameSubsystem — save data loaded from disk (version %d)."),
                    CachedSaveGame->SaveVersion);
            }
        }
        else
        {
            // File exists but failed to load/cast — create fresh
            UE_LOG(LogTemp, Warning,
                TEXT("UShooterSaveGameSubsystem — failed to load save file. Creating fresh save."));
            CachedSaveGame = Cast<UShooterSaveGame>(
                UGameplayStatics::CreateSaveGameObject(UShooterSaveGame::StaticClass()));
        }
    }
    else
    {
        // No save file yet — first time player
        UE_LOG(LogTemp, Log,
            TEXT("UShooterSaveGameSubsystem — no save file found. Creating new save for first-time player."));
        CachedSaveGame = Cast<UShooterSaveGame>(
            UGameplayStatics::CreateSaveGameObject(UShooterSaveGame::StaticClass()));
    }
}

void UShooterSaveGameSubsystem::WriteToDisk()
{
    if (!CachedSaveGame) return;

    const bool bSuccess = UGameplayStatics::SaveGameToSlot(
        CachedSaveGame,
        UShooterSaveGame::SaveSlotName,
        UShooterSaveGame::UserIndex);

    if (bSuccess)
    {
        UE_LOG(LogTemp, Log,
            TEXT("UShooterSaveGameSubsystem — save written to disk successfully."));
    }
    else
    {
        UE_LOG(LogTemp, Error,
            TEXT("UShooterSaveGameSubsystem — FAILED to write save to disk."));
    }
}

bool UShooterSaveGameSubsystem::SaveEquippedState(UEquippedStateComponent* EquippedStateComponent)
{
	if (EquippedStateComponent == nullptr)
	{
		return false;
	}

	if (CachedSaveGame == nullptr)
	{
		LoadLoadout();
	}

	if (CachedSaveGame == nullptr)
	{
		return false;
	}

	CachedSaveGame->SavedEquippedState = BuildEquippedStateSnapshot(EquippedStateComponent);
	return WriteCurrentSaveToDisk();
}

bool UShooterSaveGameSubsystem::LoadEquippedState(UEquippedStateComponent* EquippedStateComponent)
{
	if (EquippedStateComponent == nullptr)
	{
		return false;
	}

	if (CachedSaveGame == nullptr)
	{
		LoadLoadout();
	}

	if (CachedSaveGame == nullptr)
	{
		return false;
	}

	ApplyEquippedStateSnapshot(EquippedStateComponent, CachedSaveGame->SavedEquippedState);
	return true;
}

bool UShooterSaveGameSubsystem::SaveLoadoutPresets(const TArray<FLoadoutPreset>& Presets)
{
	if (CachedSaveGame == nullptr)
	{
		LoadLoadout();
	}

	if (CachedSaveGame == nullptr)
	{
		return false;
	}

	CachedSaveGame->SavedLoadoutPresets = Presets;
	return WriteCurrentSaveToDisk();
}

TArray<FLoadoutPreset> UShooterSaveGameSubsystem::LoadLoadoutPresets() const
{
	if (CachedSaveGame == nullptr)
	{
		return TArray<FLoadoutPreset>();
	}

	return CachedSaveGame->SavedLoadoutPresets;
}


bool UShooterSaveGameSubsystem::SaveQuestState(UQuestTrackerSubsystem* QuestTracker)
{
	if (!CachedSaveGame || !IsValid(QuestTracker))
	{
		return false;
	}

	// Pull current state from the subsystem into the save object.
	// Matches the same pattern as SaveStash() — subsystem provides data,
	// save object stores it, WriteCurrentSaveToDisk flushes to disk.
	QuestTracker->GetSaveData(
		CachedSaveGame->SavedActiveQuests,
		CachedSaveGame->SavedAvailableQuests,
		CachedSaveGame->SavedCompletedQuests,
		CachedSaveGame->SavedVendorReputations
	);

	return WriteCurrentSaveToDisk();
}

bool UShooterSaveGameSubsystem::LoadQuestState(UQuestTrackerSubsystem* QuestTracker)
{
	if (!CachedSaveGame || !IsValid(QuestTracker))
	{
		return false;
	}

	// Push saved data into the subsystem.
	// Subsystem rebuilds ActiveUnlockFlags from completed quests internally.
	QuestTracker->LoadFromSaveData(
		CachedSaveGame->SavedActiveQuests,
		CachedSaveGame->SavedAvailableQuests,
		CachedSaveGame->SavedCompletedQuests,
		CachedSaveGame->SavedVendorReputations
	);

	return true;
}


bool UShooterSaveGameSubsystem::SetHasUnreviewedExtraction(bool bInHasUnreviewedExtraction)
{
	if (CachedSaveGame == nullptr)
	{
		LoadLoadout();
	}

	if (CachedSaveGame == nullptr)
	{
		return false;
	}

	CachedSaveGame->bHasUnreviewedExtraction = bInHasUnreviewedExtraction;
	return WriteCurrentSaveToDisk();
}

bool UShooterSaveGameSubsystem::GetHasUnreviewedExtraction() const
{
	return CachedSaveGame != nullptr ? CachedSaveGame->bHasUnreviewedExtraction : false;
}


FEquippedStateSnapshot UShooterSaveGameSubsystem::BuildEquippedStateSnapshot(const UEquippedStateComponent* EquippedStateComponent)
{
	FEquippedStateSnapshot Snapshot{};

	if (EquippedStateComponent == nullptr)
	{
		return Snapshot;
	}

	const FItemInstance* PrimaryItem = EquippedStateComponent->GetEquippedItem(EEquipmentSlot::Primary);
	if (PrimaryItem != nullptr)
	{
		Snapshot.PrimaryWeapon.EquipmentSlot = EEquipmentSlot::Primary;
		Snapshot.PrimaryWeapon.ItemInstance = *PrimaryItem;
	}

	const FItemInstance* SecondaryItem = EquippedStateComponent->GetEquippedItem(EEquipmentSlot::Secondary);
	if (SecondaryItem != nullptr)
	{
		Snapshot.SecondaryWeapon.EquipmentSlot = EEquipmentSlot::Secondary;
		Snapshot.SecondaryWeapon.ItemInstance = *SecondaryItem;
	}

	const FItemInstance* SidearmItem = EquippedStateComponent->GetEquippedItem(EEquipmentSlot::Pistol);
	if (SidearmItem != nullptr)
	{
		Snapshot.SidearmWeapon.EquipmentSlot = EEquipmentSlot::Pistol;
		Snapshot.SidearmWeapon.ItemInstance = *SidearmItem;
	}

	const FItemInstance* ToolItem = EquippedStateComponent->GetEquippedItem(EEquipmentSlot::Tool);
	if (ToolItem != nullptr)
	{
		Snapshot.ToolItem.EquipmentSlot = EEquipmentSlot::Tool;
		Snapshot.ToolItem.ItemInstance = *ToolItem;
	}

	const FItemInstance* KnifeItem = EquippedStateComponent->GetEquippedItem(EEquipmentSlot::Knife);
	if (KnifeItem != nullptr)
	{
		Snapshot.KnifeItem.EquipmentSlot = EEquipmentSlot::Knife;
		Snapshot.KnifeItem.ItemInstance = *KnifeItem;
	}

	if (EquippedStateComponent->GetRigInventory() != nullptr)
	{
		Snapshot.RigState.Items = EquippedStateComponent->GetRigInventory()->GetItems();
	}

	if (EquippedStateComponent->GetBeltInventory() != nullptr)
	{
		Snapshot.BeltState.Items = EquippedStateComponent->GetBeltInventory()->GetItems();
	}

	if (EquippedStateComponent->GetBackpackInventory() != nullptr)
	{
		Snapshot.BackpackState.Items = EquippedStateComponent->GetBackpackInventory()->GetItems();
	}

	Snapshot.CurrentCarriedWeight = EquippedStateComponent->GetCurrentCarriedWeight();
	Snapshot.CurrentBurdenScore = EquippedStateComponent->GetCurrentBurdenScore();
	Snapshot.DeadPlayerBurdenMultiplier = EquippedStateComponent->GetDeadPlayerBurdenMultiplier();

	return Snapshot;
}

void UShooterSaveGameSubsystem::ApplyEquippedStateSnapshot(UEquippedStateComponent* EquippedStateComponent, const FEquippedStateSnapshot& Snapshot)
{
	if (EquippedStateComponent == nullptr)
	{
		return;
	}

	if (UInventoryComponent* RigInventory = EquippedStateComponent->GetRigInventory())
	{
		TArray<FItemInstance> ExistingRigItems = RigInventory->GetItems();
		for (const FItemInstance& ExistingItem : ExistingRigItems)
		{
			FItemInstance RemovedItem;
			RigInventory->Server_RemoveItem(ExistingItem.InstanceID, RemovedItem);
		}

		for (FItemInstance SavedItem : Snapshot.RigState.Items)
		{
			FGameplayTag EmptySlotTag;
			RigInventory->Server_AddItem(SavedItem, EmptySlotTag);
		}
	}

	if (UInventoryComponent* BeltInventory = EquippedStateComponent->GetBeltInventory())
	{
		TArray<FItemInstance> ExistingBeltItems = BeltInventory->GetItems();
		for (const FItemInstance& ExistingItem : ExistingBeltItems)
		{
			FItemInstance RemovedItem;
			BeltInventory->Server_RemoveItem(ExistingItem.InstanceID, RemovedItem);
		}

		for (FItemInstance SavedItem : Snapshot.BeltState.Items)
		{
			FGameplayTag EmptySlotTag;
			BeltInventory->Server_AddItem(SavedItem, EmptySlotTag);
		}
	}

	if (UInventoryComponent* BackpackInventory = EquippedStateComponent->GetBackpackInventory())
	{
		TArray<FItemInstance> ExistingBackpackItems = BackpackInventory->GetItems();
		for (const FItemInstance& ExistingItem : ExistingBackpackItems)
		{
			FItemInstance RemovedItem;
			BackpackInventory->Server_RemoveItem(ExistingItem.InstanceID, RemovedItem);
		}

		for (FItemInstance SavedItem : Snapshot.BackpackState.Items)
		{
			FGameplayTag EmptySlotTag;
			BackpackInventory->Server_AddItem(SavedItem, EmptySlotTag);
		}
	}

	// Dedicated equipped slots will be applied in the next character wiring step,
	// after the equipment component exposes explicit snapshot setters.
	EquippedStateComponent->RecalculateWeight();
}

bool UShooterSaveGameSubsystem::WriteCurrentSaveToDisk()
{
	if (CachedSaveGame == nullptr)
	{
		return false;
	}

	WriteToDisk();
	return true;
}

// ============================================================================
// Debug Commands
// ============================================================================

#if !UE_BUILD_SHIPPING

void UShooterSaveGameSubsystem::DebugSaveLoadout()
{
    UE_LOG(LogTemp, Log, TEXT("[Debug] ShooterGame.SaveLoadout — forcing write to disk."));
    WriteToDisk();
}

void UShooterSaveGameSubsystem::DebugLoadLoadout()
{
    UE_LOG(LogTemp, Log, TEXT("[Debug] ShooterGame.LoadLoadout — reloading from disk."));
    LoadLoadout();
}

void UShooterSaveGameSubsystem::DebugClearSaveData()
{
    UE_LOG(LogTemp, Log, TEXT("[Debug] ShooterGame.ClearSaveData — deleting save file."));

    // Build the full path and delete via IFileManager
    const FString SaveDir = FPaths::ProjectSavedDir() / TEXT("SaveGames");
    const FString FilePath = SaveDir / UShooterSaveGame::SaveSlotName + TEXT(".sav");

    if (IFileManager::Get().Delete(*FilePath))
    {
        UE_LOG(LogTemp, Log, TEXT("[Debug] Save file deleted: %s"), *FilePath);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Debug] Could not delete save file (may not exist): %s"), *FilePath);
    }

    // Reset in-memory cache to fresh defaults
    CachedSaveGame = Cast<UShooterSaveGame>(
        UGameplayStatics::CreateSaveGameObject(UShooterSaveGame::StaticClass()));
}

void UShooterSaveGameSubsystem::DebugPrintLoadout()
{
    if (!CachedSaveGame)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Debug] ShooterGame.PrintLoadout — no cached save."));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[Debug] ShooterGame.PrintLoadout — Save Version: %d"),
        CachedSaveGame->SaveVersion);

    // Print each slot's item class and magazine count
    const FLoadoutData& Loadout = CachedSaveGame->SavedLoadout;
    for (int32 i = 0; i < Loadout.Slots.Num(); i++)
    {
        const FLoadoutSlot& Slot = Loadout.Slots[i];
        UE_LOG(LogTemp, Log, TEXT("  Slot[%d]: %s | Mags: %d | Variant: %s"),
            i,
            Slot.IsOccupied() ? *Slot.ItemClass.ToString() : TEXT("EMPTY"),
            
            
            Slot.MagazineCount,
            *Slot.VariantID.ToString());
    }

    const auto& [MeshSkinID, HelmetID, BackpackID, ColorVariantID] = CachedSaveGame->SavedAppearance;
    UE_LOG(LogTemp, Log, TEXT("  Appearance — Skin: %d | Helmet: %d | Backpack: %d | Color: %s"),
        MeshSkinID, HelmetID, BackpackID, *ColorVariantID.ToString());
}

#endif