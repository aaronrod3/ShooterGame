// Copyright Epic Games, Inc. All Rights Reserved.


#include "ShooterGamePlayerController.h"
#include "Items/Components/ItemComponent.h"
#include "ShooterGame/HUD/Widgets/HUDWidget.h"
#include "Inventory/InventoryComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Interaction/Highlightable.h"
#include "Kismet/GameplayStatics.h"
#include "InputMappingContext.h"
#include "ShooterGame/Components/CombatComponent.h"
#include "Blueprint/UserWidget.h"
#include "Interaction/HighlightableStaticMesh.h"
#include "ShooterGame/Framework/GameMode/ShooterGameGameMode.h"
#include "Player/Character/ShooterGameCharacter.h"


AShooterGamePlayerController::AShooterGamePlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	TraceLength = 500.f;
	ItemTraceChannel = ECC_GameTraceChannel1;
	
	
}

void AShooterGamePlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// Do not run gameplay tick logic during match over / end screen
	if (bMatchOverHandled) return;
	
	TraceForItem();
	UpdateCursorVisibility();
}


void AShooterGamePlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	// Start with cursor visible — unequipped default
	bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	//bAutoManageActiveCameraTarget = false;

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		for (UInputMappingContext* CurrentContext : DefaultIMCs)
		{
			Subsystem->AddMappingContext(CurrentContext, 0);
		}
	}
	
	InventoryComponent = FindComponentByClass<UInventoryComponent>();
	
	CreateHUDWidget();
}

void AShooterGamePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent);
	
}



void AShooterGamePlayerController::CreateHUDWidget()
{
	if (!IsLocalPlayerController()) return;
	HUDWidget = CreateWidget<UHUDWidget>(this, HUDWidgetClass);
	if (IsValid(HUDWidget))
	{
		HUDWidget->AddToViewport();
	}
}

void AShooterGamePlayerController::UpdateCursorVisibility()
{
	AShooterGameCharacter* ShooterCharacter = Cast<AShooterGameCharacter>(GetPawn());
	if (!ShooterCharacter) return;

	const bool bIsEquipped = ShooterCharacter->IsWeaponEquipped();
	if (bIsEquipped == bWasEquipped) return;
	bWasEquipped = bIsEquipped;

	// Keep the same input mode in both cases so the viewport never enters
	// mouse-capture camera-look behavior.
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);

	// Your project may still want the OS cursor hidden while equipped,
	// but do not switch to GameOnly.
	bShowMouseCursor = !bIsEquipped ? true : false;
}

void AShooterGamePlayerController::TraceForItem()
{
	if (!IsValid(GEngine) || !IsValid(GEngine->GameViewport)) return;
	FVector2D ViewportSize;
	GEngine->GameViewport->GetViewportSize(ViewportSize);
	const FVector2D ViewportCenter = ViewportSize / 2.f;
	FVector TraceStart;
	FVector Forward;
	if (!UGameplayStatics::DeprojectScreenToWorld(this, ViewportCenter, TraceStart, Forward)) return;

	const FVector TraceEnd = TraceStart + Forward * TraceLength;
	FHitResult HitResult;
	GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ItemTraceChannel);

	LastActor = ThisActor;
	ThisActor = HitResult.GetActor();
	
	if (!ThisActor.IsValid())
	{
		if (IsValid(HUDWidget)) HUDWidget->HidePickupMessage();
	}

	if (ThisActor == LastActor) return;

	if (ThisActor.IsValid())
	{
		if (UActorComponent* Highlightable = ThisActor->FindComponentByInterface(UHighlightable::StaticClass()); IsValid(Highlightable))
		{
			IHighlightable::Execute_Highlight(Highlightable);
		}
		
		
		UItemComponent* ItemComponent = ThisActor->FindComponentByClass<UItemComponent>();
		if (!IsValid(ItemComponent)) return;

		if (IsValid(HUDWidget)) HUDWidget->ShowPickupMessage(ItemComponent->GetPickupMessage());
	}

	if (LastActor.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Stopped tracing last actor."))
		if (UActorComponent* Highlightable = LastActor->FindComponentByInterface(UHighlightable::StaticClass()); IsValid(Highlightable))
		{
			IHighlightable::Execute_UnHighlight(Highlightable);
		}
	}
}

void AShooterGamePlayerController::PossessSpectatorPawn(
	const TArray<AShooterGameCharacter*>& AlivePlayers)
{
	if (!SpectatorPawnClass)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("AShooterGamePlayerController::PossessSpectatorPawn — SpectatorPawnClass not set"));
		return;
	}

	// Spawn spectator pawn at dead player's last location
	FVector SpectatorSpawnLocation = FVector::ZeroVector;
	if (GetPawn())
	{
		SpectatorSpawnLocation = GetPawn()->GetActorLocation();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;

	ActiveSpectatorPawn = GetWorld()->SpawnActor<AShooterGameSpectatorPawn>(
		SpectatorPawnClass,
		SpectatorSpawnLocation,
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (ActiveSpectatorPawn)
	{
		// Unpossess dead character, possess spectator pawn
		UnPossess();
		Possess(ActiveSpectatorPawn);

		// Initialize with current alive players
		ActiveSpectatorPawn->InitSpectator(AlivePlayers);

		UE_LOG(LogTemp, Warning,
			TEXT("AShooterGamePlayerController — now spectating, %d players alive"),
			AlivePlayers.Num());
	}
}

void AShooterGamePlayerController::UpdateSpectatorTargets(
	const TArray<AShooterGameCharacter*>& AlivePlayers)
{
	if (ActiveSpectatorPawn)
	{
		ActiveSpectatorPawn->UpdateAlivePlayerList(AlivePlayers);
	}
}

// -----------------------------------------------------------------------
// HandleMatchOver — disables gameplay input, shows end screen
// -----------------------------------------------------------------------

void AShooterGamePlayerController::HandleMatchOver()
{
	UE_LOG(LogTemp, Warning,
		TEXT("AShooterGamePlayerController::HandleMatchOver — disabling input, showing end screen"));

	bMatchOverHandled = true;
	
	// Disable all gameplay input — player can still click UI
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);

	// Show cursor for end screen interaction
	bShowMouseCursor = true;
	SetInputMode(FInputModeUIOnly());

	// Destroy spectator pawn if still active — no longer needed
	if (ActiveSpectatorPawn)
	{
		ActiveSpectatorPawn->Destroy();
		ActiveSpectatorPawn = nullptr;
	}

	// Spawn and display end screen widget — local only
	if (IsLocalController() && EndScreenWidgetClass)
	{
		EndScreenWidget = CreateWidget<UShooterEndScreenWidget>(this, EndScreenWidgetClass);
		if (EndScreenWidget)
		{
			EndScreenWidget->AddToViewport(10); // ZOrder 10 — above HUD
			UE_LOG(LogTemp, Log,
				TEXT("AShooterGamePlayerController::HandleMatchOver — end screen added to viewport"));
		}
	}
	else if (IsLocalController() && !EndScreenWidgetClass.Get())
		
	{
		// 9c stub — EndScreenWidgetClass not set yet, log and continue
		UE_LOG(LogTemp, Warning,
			TEXT("AShooterGamePlayerController::HandleMatchOver — EndScreenWidgetClass not set, skipping widget (set in BP after 9c)"));
	}
}

// -----------------------------------------------------------------------
// HandleMatchRestart — re-enables input, removes end screen
// -----------------------------------------------------------------------

void AShooterGamePlayerController::HandleMatchRestart()
{
	if (!bMatchOverHandled) return;
	bMatchOverHandled = false;

	UE_LOG(LogTemp, Warning,
		TEXT("AShooterGamePlayerController::HandleMatchRestart — restoring input, hiding end screen"));

	// Restore to project default input mode — matches BeginPlay
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);

	// Cursor visibility handled by UpdateCursorVisibility() in Tick once bMatchOverHandled = false
	bShowMouseCursor = true;

	// Remove end screen widget
	if (EndScreenWidget)
	{
		EndScreenWidget->RemoveFromParent();
		EndScreenWidget = nullptr;
	}
}

// -----------------------------------------------------------------------
// RequestMatchRestart — called by end screen button before auto-timer fires
// -----------------------------------------------------------------------

void AShooterGamePlayerController::RequestMatchRestart()
{
	UE_LOG(LogTemp, Warning,
		TEXT("AShooterGamePlayerController::RequestMatchRestart — player requested early restart"));

	// Only the server can restart — RPC if client
	if (HasAuthority())
	{
		if (AShooterGameGameMode* GM = GetWorld()->GetAuthGameMode<AShooterGameGameMode>())
		{
			GM->RestartMatch();
		}
	}
	else
	{
		ServerRequestMatchRestart();
	}
}

void AShooterGamePlayerController::ServerRequestMatchRestart_Implementation()
{
	if (AShooterGameGameMode* GM = GetWorld()->GetAuthGameMode<AShooterGameGameMode>())
	{
		GM->RestartMatch();
	}
}


