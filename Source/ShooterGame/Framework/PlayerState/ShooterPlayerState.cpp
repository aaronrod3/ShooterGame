// Source/ShooterGame/Framework/PlayerState/ShooterPlayerState.cpp


#include "ShooterPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "ShooterGame/Framework/Subsystems/ShooterSaveGameSubsystem.h"
#include "ShooterGame/Components/LoadoutComponent.h"
#include "ShooterGame/Player/Character/ShooterGameCharacter.h"

// Step 5B include — subsystem is not implemented yet, stubbed out below.
// Uncomment when ShooterSaveGameSubsystem.h exists:
// #include "ShooterGame/Framework/Subsystems/ShooterSaveGameSubsystem.h"

AShooterPlayerState::AShooterPlayerState()
{
    // PlayerState is always replicated by default in UE5 — no extra setup needed
}

void AShooterPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // Replicate to all clients so lobby can display everyone's loadout/appearance
    DOREPLIFETIME(AShooterPlayerState, SavedLoadout);
    DOREPLIFETIME(AShooterPlayerState, SavedAppearance);
}

// ============================================================================
// Loadout Interface
// ============================================================================

void AShooterPlayerState::Server_UpdateLoadoutSlot_Implementation(
    EEquipmentSlot Slot, const FLoadoutSlot& NewSlotData)
{
    // Update our local saved copy
    SavedLoadout.GetSlot(Slot) = NewSlotData;

    // Push the single slot change to the character's LoadoutComponent
    // if they currently have a character in the world
    if (ULoadoutComponent* LoadoutComp = GetOwnedLoadoutComponent())
    {
        LoadoutComp->Server_SetSlot(Slot, NewSlotData);
    }

    // Persist the change to disk via SaveSubsystem (Step 5B wires this up)
    NotifySaveSubsystem();
}

void AShooterPlayerState::SetFullLoadout(const FLoadoutData& NewLoadout, bool bPushToCharacter)
{
    // Server-only — called by SaveSubsystem on login
    if (!HasAuthority()) return;

    SavedLoadout = NewLoadout;

    if (bPushToCharacter)
    {
        if (ULoadoutComponent* LoadoutComp = GetOwnedLoadoutComponent())
        {
            LoadoutComp->Server_SetFullLoadout(SavedLoadout);
        }
    }
}

void AShooterPlayerState::Server_UpdateAppearance_Implementation(
    const FCharacterAppearance& NewAppearance)
{
    SavedAppearance = NewAppearance;

    if (ULoadoutComponent* LoadoutComp = GetOwnedLoadoutComponent())
    {
        LoadoutComp->Server_SetAppearance(SavedAppearance);
    }

    NotifySaveSubsystem();
}

// ============================================================================
// Character Possession Bridge
// ============================================================================

void AShooterPlayerState::PushLoadoutToCharacter(ACharacter* NewCharacter)
{
    // Only the server should push authoritative loadout data
    if (!HasAuthority()) return;

    if (!NewCharacter)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("AShooterPlayerState::PushLoadoutToCharacter — called with null character."));
        return;
    }

    AShooterGameCharacter* ShooterChar = Cast<AShooterGameCharacter>(NewCharacter);
    if (!ShooterChar)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("AShooterPlayerState::PushLoadoutToCharacter — character is not AShooterGameCharacter."));
        return;
    }

    ULoadoutComponent* LoadoutComp = ShooterChar->GetLoadoutComponent();
    if (!LoadoutComp)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("AShooterPlayerState::PushLoadoutToCharacter — character has no LoadoutComponent."));
        return;
    }

    // Push both loadout and appearance in one go
    LoadoutComp->Server_SetFullLoadout(SavedLoadout);
    LoadoutComp->Server_SetAppearance(SavedAppearance);

    UE_LOG(LogTemp, Log,
        TEXT("AShooterPlayerState::PushLoadoutToCharacter — pushed loadout to %s."),
        *NewCharacter->GetName());
}

// ============================================================================
// Seamless Travel — preserve loadout across level transitions
// ============================================================================

void AShooterPlayerState::CopyProperties(APlayerState* PlayerState)
{
    Super::CopyProperties(PlayerState);

    // When UE5 re-creates PlayerState after seamless travel, copy our data
    // to the new instance so the player's loadout survives the transition
    if (AShooterPlayerState* NewPS = Cast<AShooterPlayerState>(PlayerState))
    {
        NewPS->SavedLoadout    = SavedLoadout;
        NewPS->SavedAppearance = SavedAppearance;
    }
}

// ============================================================================
// Rep Notifies
// ============================================================================

void AShooterPlayerState::OnRep_SavedLoadout()
{
    // On clients: if we own a character, push the updated loadout to it.
    // This handles the case where loadout changes while in-game
    // (e.g. future mid-mission loadout swap feature).
    if (ULoadoutComponent* LoadoutComp = GetOwnedLoadoutComponent())
    {
        // Clients can't call Server_SetFullLoadout — push directly to local
        // component for display purposes. Server state is authoritative.
        LoadoutComp->BroadcastCurrentLoadout();
    }
}

void AShooterPlayerState::OnRep_SavedAppearance()
{
    // Future: trigger appearance refresh on the local character mesh
    // when another player's appearance changes in lobby
}

// ============================================================================
// Internal Helpers
// ============================================================================

ULoadoutComponent* AShooterPlayerState::GetOwnedLoadoutComponent() const
{
    // GetPawn() returns the currently possessed pawn for this PlayerState
    APawn* OwnedPawn = GetPawn();
    if (!OwnedPawn) return nullptr;

    AShooterGameCharacter* ShooterChar = Cast<AShooterGameCharacter>(OwnedPawn);
    if (!ShooterChar) return nullptr;

    return ShooterChar->GetLoadoutComponent();
}

void AShooterPlayerState::NotifySaveSubsystem()
{
    UGameInstance* GI = GetGameInstance();
    if (!GI) return;

    UShooterSaveGameSubsystem* SaveSys = GI->GetSubsystem<UShooterSaveGameSubsystem>();
    if (SaveSys)
    {
        SaveSys->SaveLoadout(SavedLoadout, SavedAppearance);
    }
}