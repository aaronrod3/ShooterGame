// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShooterGameGameMode.h"
#include "EngineUtils.h"
#include "ShooterGame/Framework/Subsystems/ShooterSaveGameSubsystem.h"
#include "ShooterGame/Framework/PlayerState/ShooterPlayerState.h"
#include "ShooterGame/Player/Character/ShooterGameCharacter.h"
#include "ShooterGame/Player/Controller/ShooterGamePlayerController.h"

AShooterGameGameMode::AShooterGameGameMode()
{
	// stub
}


void AShooterGameGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// Push saved loadout data into the new player's PlayerState.
	// PostLogin is server-only and fires after PlayerState is fully initialized.
	if (AShooterPlayerState* PS = NewPlayer->GetPlayerState<AShooterPlayerState>())
	{
		if (UShooterSaveGameSubsystem* SaveSys =
			GetGameInstance()->GetSubsystem<UShooterSaveGameSubsystem>())
		{
			SaveSys->PushSaveDataToPlayerState(PS);
		}
	}
}



// -----------------------------------------------------------------------
// RestartPlayer — fires after pawn is possessed, safe to read GetPawn()
// -----------------------------------------------------------------------

void AShooterGameGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);

	if (AShooterGameCharacter* Character =
		Cast<AShooterGameCharacter>(NewPlayer->GetPawn()))
	{
		AlivePlayers.AddUnique(Character);
		bMatchInProgress = true;

		UE_LOG(LogTemp, Log,
			TEXT("AShooterGameGameMode::RestartPlayer — %s added to AlivePlayers (%d total)"),
			*Character->GetName(),
			AlivePlayers.Num());
	}
}

// -----------------------------------------------------------------------
// PlayerDowned — player entered downed state, bleedout timer started
// -----------------------------------------------------------------------

void AShooterGameGameMode::PlayerDowned(
	AShooterGameCharacter* DownedCharacter,
	AShooterGamePlayerController* VictimController)
{
	if (!DownedCharacter) return;

	UE_LOG(LogTemp, Warning,
		TEXT("AShooterGameGameMode::PlayerDowned — %s is downed"),
		*DownedCharacter->GetName());
}

// -----------------------------------------------------------------------
// PlayerDied — removes from alive list, triggers spectator possession
// -----------------------------------------------------------------------

void AShooterGameGameMode::PlayerDied(
	AShooterGameCharacter* DeadCharacter,
	AShooterGamePlayerController* VictimController)
{
	if (!DeadCharacter) return;
	
	// -----------------------------------------------------------------------
	// Guard — ignore late PlayerDied calls after match is already over
	// -----------------------------------------------------------------------
	if (!bMatchInProgress)
	{
		UE_LOG(LogTemp, Log,
			TEXT("AShooterGameGameMode::PlayerDied — match not in progress, ignoring (%s)"),
			*DeadCharacter->GetName());
		return;
	}
	// -----------------------------------------------------------------------

	AlivePlayers.Remove(DeadCharacter);

	UE_LOG(LogTemp, Warning,
		TEXT("AShooterGameGameMode::PlayerDied — %s died. %d players remaining."),
		*DeadCharacter->GetName(),
		AlivePlayers.Num());

	// Possess spectator pawn on the dead player's controller
	if (VictimController)
	{
		VictimController->PossessSpectatorPawn(AlivePlayers);
	}

	// Update any controllers already spectating
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AShooterGamePlayerController* OtherPC =
			Cast<AShooterGamePlayerController>(It->Get()))
		{
			if (OtherPC == VictimController) continue;

			if (OtherPC->GetSpectatorPawn())
			{
				OtherPC->UpdateSpectatorTargets(AlivePlayers);
			}
		}
	}

	// All players dead — game over hook (Step 9)
	if (AlivePlayers.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("AShooterGameGameMode::PlayerDied — all players dead, game over"));
		HandleMatchOver();
	}
}

// -----------------------------------------------------------------------
// PlayerRevived — restores player to alive list, updates all spectators
// -----------------------------------------------------------------------

void AShooterGameGameMode::PlayerRevived(AShooterGameCharacter* RevivedPlayer)
{
	if (!RevivedPlayer) return;

	AlivePlayers.AddUnique(RevivedPlayer);

	UE_LOG(LogTemp, Warning,
		TEXT("AShooterGameGameMode::PlayerRevived — %s revived. %d players alive."),
		*RevivedPlayer->GetName(),
		AlivePlayers.Num());

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AShooterGamePlayerController* OtherPC =
			Cast<AShooterGamePlayerController>(It->Get()))
		{
			if (OtherPC->GetSpectatorPawn())
			{
				OtherPC->UpdateSpectatorTargets(AlivePlayers);
			}
		}
	}
}

// -----------------------------------------------------------------------
// Friendly Fire
// -----------------------------------------------------------------------

void AShooterGameGameMode::SetFriendlyFireEnabled(bool bEnabled)
{
	bFriendlyFireEnabled = bEnabled;
	UE_LOG(LogTemp, Warning,
		TEXT("AShooterGameGameMode::SetFriendlyFireEnabled — Friendly fire: %s"),
		bEnabled ? TEXT("ENABLED") : TEXT("DISABLED"));
}

void AShooterGameGameMode::ToggleFriendlyFire()
{
	SetFriendlyFireEnabled(!bFriendlyFireEnabled);
}


// -----------------------------------------------------------------------
// HandleMatchOver — fires when last player dies
// -----------------------------------------------------------------------


void AShooterGameGameMode::HandleMatchOver()
{
	if (!bMatchInProgress) return; // Guard against double-fire
	bMatchInProgress = false;

	UE_LOG(LogTemp, Warning,
		TEXT("AShooterGameGameMode::HandleMatchOver — match over, restarting in %.1fs"),
		PostMatchDelay);

	// Notify all player controllers — they'll show the end screen
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AShooterGamePlayerController* PC =
			Cast<AShooterGamePlayerController>(It->Get()))
		{
			PC->HandleMatchOver();
		}
	}

	// Auto-restart after PostMatchDelay as fallback (if no button press)
	GetWorld()->GetTimerManager().SetTimer(
		PostMatchTimerHandle,
		this,
		&AShooterGameGameMode::RestartMatch,
		PostMatchDelay,
		false
	);
}

// -----------------------------------------------------------------------
// RestartMatch — simulated lobby: reset all players in-place, same map
// -----------------------------------------------------------------------

void AShooterGameGameMode::RestartMatch()
{
	UE_LOG(LogTemp, Warning,
		TEXT("AShooterGameGameMode::RestartMatch — called. Timer active: %s"),
		GetWorld()->GetTimerManager().IsTimerActive(PostMatchTimerHandle) ? TEXT("YES") : TEXT("NO"));
	
	// Guard against being called multiple times or before match over settles
	if (bMatchInProgress)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("AShooterGameGameMode::RestartMatch — match still in progress, ignoring"));
		return;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("AShooterGameGameMode::RestartMatch — restarting match")); 

	GetWorld()->GetTimerManager().ClearTimer(PostMatchTimerHandle);
	AlivePlayers.Empty();

	// -----------------------------------------------------------------------
	// Destroy ALL existing ShooterGameCharacter actors in the world
	// Catches unpossessed characters left over from spectator possession
	// -----------------------------------------------------------------------
	for (TActorIterator<AShooterGameCharacter> It(GetWorld()); It; ++It)
	{
		It->Destroy();
	}

	// Restart every connected player controller
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			// Destroy spectator pawn if still possessed
			if (APawn* OldPawn = PC->GetPawn())
			{
				OldPawn->Destroy();
			}

			RestartPlayer(PC);

			if (AShooterGamePlayerController* SPC =
				Cast<AShooterGamePlayerController>(PC))
			{
				SPC->HandleMatchRestart();
			}
		}
	}

	bMatchInProgress = true;

	UE_LOG(LogTemp, Warning,
		TEXT("AShooterGameGameMode::RestartMatch — match restarted, %d players alive"),
		AlivePlayers.Num());
}