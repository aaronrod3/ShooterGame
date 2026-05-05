// Source/ShooterGame/Framework/Subsystems/ShooterSaveGameSubsystem.cpp
#include "ShooterSaveGameSubsystem.h"
#include "ShooterSaveGame.h"
#include "ShooterGame/Framework/PlayerState/ShooterPlayerState.h"
#include "Kismet/GameplayStatics.h"
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

    const FCharacterAppearance& App = CachedSaveGame->SavedAppearance;
    UE_LOG(LogTemp, Log, TEXT("  Appearance — Skin: %d | Helmet: %d | Backpack: %d | Color: %s"),
        App.MeshSkinID, App.HelmetID, App.BackpackID, *App.ColorVariantID.ToString());
}

#endif