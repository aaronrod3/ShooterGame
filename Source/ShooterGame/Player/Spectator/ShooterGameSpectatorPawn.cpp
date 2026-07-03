// ShooterGameSpectatorPawn.cpp

#include "ShooterGameSpectatorPawn.h"
#include "EnhancedInputComponent.h"
#include "ShooterGame/Player/Character/ShooterGameCharacter.h"

AShooterGameSpectatorPawn::AShooterGameSpectatorPawn()
{
	PrimaryActorTick.bCanEverTick = false;

	// Inherit USpectatorPawn defaults — it already has a UCapsuleComponent
	// and basic movement. We override the view target behavior only.
	bReplicates = false; // purely local — no need to replicate spectator
}

void AShooterGameSpectatorPawn::BeginPlay()
{
	Super::BeginPlay();
}

// -----------------------------------------------------------------------
// Input
// -----------------------------------------------------------------------

void AShooterGameSpectatorPawn::SetupPlayerInputComponent(
	UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC =
		Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (SpectateNextAction)
		{
			EIC->BindAction(SpectateNextAction,
				ETriggerEvent::Started, this,
				&AShooterGameSpectatorPawn::OnSpectateNext);
		}

		if (SpectatePrevAction)
		{
			EIC->BindAction(SpectatePrevAction,
				ETriggerEvent::Started, this,
				&AShooterGameSpectatorPawn::OnSpectatePrev);
		}
	}
}

void AShooterGameSpectatorPawn::OnSpectateNext()
{
	CycleToNextTarget();
}

void AShooterGameSpectatorPawn::OnSpectatePrev()
{
	CycleToPreviousTarget();
}

// -----------------------------------------------------------------------
// Init — called by GameMode after possession
// -----------------------------------------------------------------------

void AShooterGameSpectatorPawn::InitSpectator(
	const TArray<AShooterGameCharacter*>& AlivePlayers)
{
	AliveTargets = AlivePlayers;
	CurrentTargetIndex = 0;

	if (AliveTargets.Num() > 0)
	{
		AttachToTarget(AliveTargets[0]);
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("AShooterGameSpectatorPawn::InitSpectator — no alive players to spectate"));
	}
}

// -----------------------------------------------------------------------
// Cycle targets
// -----------------------------------------------------------------------

void AShooterGameSpectatorPawn::CycleToNextTarget()
{
	if (AliveTargets.Num() == 0) return;

	CurrentTargetIndex = (CurrentTargetIndex + 1) % AliveTargets.Num();
	AttachToTarget(AliveTargets[CurrentTargetIndex]);
}

void AShooterGameSpectatorPawn::CycleToPreviousTarget()
{
	if (AliveTargets.Num() == 0) return;

	CurrentTargetIndex =
		(CurrentTargetIndex - 1 + AliveTargets.Num()) % AliveTargets.Num();
	AttachToTarget(AliveTargets[CurrentTargetIndex]);
}

// -----------------------------------------------------------------------
// Update alive list — called when someone dies or checkpoints respawn players
// -----------------------------------------------------------------------

void AShooterGameSpectatorPawn::UpdateAlivePlayerList(
	const TArray<AShooterGameCharacter*>& AlivePlayers)
{
	AliveTargets = AlivePlayers;

	// If current target is no longer in the list, move to next valid target
	if (!AliveTargets.Contains(CurrentTarget))
	{
		CurrentTargetIndex = 0;
		if (AliveTargets.Num() > 0)
		{
			AttachToTarget(AliveTargets[0]);
		}
		else
		{
			CurrentTarget = nullptr;
			UE_LOG(LogTemp, Warning,
				TEXT("AShooterGameSpectatorPawn — no alive players left to spectate"));
		}
	}
}

// -----------------------------------------------------------------------
// Attach camera to target — follow mode
// -----------------------------------------------------------------------

void AShooterGameSpectatorPawn::AttachToTarget(AShooterGameCharacter* Target)
{
	if (!Target) return;

	CurrentTarget = Target;

	// Use PlayerController::SetViewTarget to follow the target's camera
	// This gives the exact same view as the spectated player — their camera,
	// their spring arm, their rotation. No separate camera needed on spectator pawn.
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		FViewTargetTransitionParams TransitionParams;
		TransitionParams.BlendTime = 0.5f; // smooth 0.5s blend between targets
		TransitionParams.BlendFunction = EViewTargetBlendFunction::VTBlend_Cubic;

		PC->SetViewTarget(Target, TransitionParams);

		UE_LOG(LogTemp, Log,
			TEXT("AShooterGameSpectatorPawn::AttachToTarget — now spectating %s"),
			*Target->GetName());
	}
}