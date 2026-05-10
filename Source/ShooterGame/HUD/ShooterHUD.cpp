// Source/ShooterGame/HUD/ShooterHUD.cpp
#include "HUD/ShooterHUD.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "HUD/Inventory/EquipmentPanelWidget.h"
#include "HUD/Inventory/LootContainerWidget.h"
#include "HUD/Inventory/PostExtractionWidget.h"
#include "HUD/Inventory/QuickSlotBarWidget.h"
#include "HUD/Inventory/SquadCacheWidget.h"
#include "HUD/Inventory/StashWindowWidget.h"
#include "HUD/Inventory/VendorWidget.h"
#include "Inventory/QuestbookWidget.h"
#include "ShooterGame/Interaction/VendorNPCActor.h"
#include "Interaction/LootContainerActor.h"
#include "Interaction/SquadCacheActor.h"
#include "Player/Character/ShooterGameCharacter.h"
#include "EngineUtils.h"   // for TActorIterator
#include "Player/Controller/ShooterGamePlayerController.h"

void AShooterHUD::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}

	// Create the persistent HUD widget (ammo, crosshair, etc.)
	if (HUDWidgetClass)
	{
		HUDWidget = CreateWidget<UHUDWidget>(PC, HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToViewport(0);
		}
	}

	// Create the persistent quick slot bar
	if (QuickSlotBarWidgetClass)
	{
		QuickSlotBarWidget = CreateWidget<UQuickSlotBarWidget>(PC, QuickSlotBarWidgetClass);
		if (QuickSlotBarWidget)
		{
			QuickSlotBarWidget->AddToViewport(1);

			// Bind to the character's quick slot component if available
			if (AShooterGameCharacter* ShooterChar = GetOwningShooterCharacter())
			{
				QuickSlotBarWidget->InitializeQuickSlotBar(ShooterChar->GetQuickSlotComponent());
			}
		}
	}
	
	// Bind to all vendor actors already in the world at BeginPlay.
	// Vendors placed in the lobby level are available immediately.
	// Dynamically spawned vendors should call OpenVendor() directly
	// or bind OnVendorInteracted after spawning.
	for (TActorIterator<AVendorNPCActor> It(GetWorld()); It; ++It)
	{
		It->OnVendorInteracted.AddDynamic(this, &AShooterHUD::HandleVendorInteracted);
	}
	
}

// ── Existing weapon/ammo binding ────────────────────────────────────────────

void AShooterHUD::BindToWeapon(AWeapon* NewWeapon)
{
	if (BoundWeapon)
	{
		BoundWeapon->OnAmmoChanged.RemoveDynamic(this, &AShooterHUD::OnAmmoChanged);
	}

	BoundWeapon = NewWeapon;

	if (BoundWeapon)
	{
		BoundWeapon->OnAmmoChanged.AddDynamic(this, &AShooterHUD::OnAmmoChanged);
	}
}

void AShooterHUD::OnAmmoChanged(int32 MagRounds, int32 MagCapacity)
{
	if (HUDWidget)
	{
		HUDWidget->UpdateAmmoDisplay(MagRounds, MagCapacity);
	}
}

// ── Main Inventory ───────────────────────────────────────────────────────────

void AShooterHUD::ToggleInventory()
{
	if (bMainInventoryOpen)
	{
		CloseInventory();
	}
	else
	{
		OpenInventory();
	}
}

void AShooterHUD::OpenInventory()
{
	if (bMainInventoryOpen)
	{
		return;
	}

	APlayerController* PC = GetOwningPlayerController();
	AShooterGameCharacter* ShooterChar = GetOwningShooterCharacter();
	if (!PC || !ShooterChar)
	{
		return;
	}

	// Stash window
	if (StashWindowWidgetClass && !StashWindowWidget)
	{
		StashWindowWidget = CreateWidget<UStashWindowWidget>(PC, StashWindowWidgetClass);
	}

	if (StashWindowWidget)
	{
		StashWindowWidget->InitializeStashWindow(ShooterChar->GetStash());
		StashWindowWidget->AddToViewport(10);
	}

	// Equipment panel
	if (EquipmentPanelWidgetClass && !EquipmentPanelWidget)
	{
		EquipmentPanelWidget = CreateWidget<UEquipmentPanelWidget>(PC, EquipmentPanelWidgetClass);
	}

	if (EquipmentPanelWidget)
	{
		EquipmentPanelWidget->InitializeEquipmentPanel(ShooterChar->GetEquippedState());
		EquipmentPanelWidget->AddToViewport(10);
	}

	bMainInventoryOpen = true;
	ApplyInventoryInputMode();
}

void AShooterHUD::CloseInventory()
{
	if (!bMainInventoryOpen)
	{
		return;
	}

	if (StashWindowWidget)
	{
		StashWindowWidget->RemoveFromParent();
	}

	if (EquipmentPanelWidget)
	{
		EquipmentPanelWidget->RemoveFromParent();
	}

	bMainInventoryOpen = false;

	if (!IsAnyInventoryOpen())
	{
		ApplyGameplayInputMode();
	}
}

// ── Loot Container ───────────────────────────────────────────────────────────

void AShooterHUD::OpenLootContainer(ALootContainerActor* LootActor)
{
	if (bLootWindowOpen || !LootActor)
	{
		return;
	}

	APlayerController* PC = GetOwningPlayerController();
	AShooterGameCharacter* ShooterChar = GetOwningShooterCharacter();
	if (!PC || !ShooterChar)
	{
		return;
	}

	if (LootContainerWidgetClass && !LootContainerWidget)
	{
		LootContainerWidget = CreateWidget<ULootContainerWidget>(PC, LootContainerWidgetClass);
	}

	if (LootContainerWidget)
	{
		LootContainerWidget->InitializeLootWindow(LootActor, ShooterChar);
		LootContainerWidget->AddToViewport(10);
	}

	bLootWindowOpen = true;
	ApplyInventoryInputMode();
}

void AShooterHUD::CloseLootContainer()
{
	if (!bLootWindowOpen)
	{
		return;
	}

	if (LootContainerWidget)
	{
		LootContainerWidget->RemoveFromParent();
	}

	bLootWindowOpen = false;

	if (!IsAnyInventoryOpen())
	{
		ApplyGameplayInputMode();
	}
}

// ── Squad Cache ──────────────────────────────────────────────────────────────

void AShooterHUD::OpenSquadCache(ASquadCacheActor* CacheActor)
{
	if (bSquadCacheWindowOpen || !CacheActor)
	{
		return;
	}

	APlayerController* PC = GetOwningPlayerController();
	AShooterGameCharacter* ShooterChar = GetOwningShooterCharacter();
	if (!PC || !ShooterChar)
	{
		return;
	}

	if (SquadCacheWidgetClass && !SquadCacheWidgetInstance)
	{
		SquadCacheWidgetInstance = CreateWidget<USquadCacheWidget>(PC, SquadCacheWidgetClass);
	}

	if (SquadCacheWidgetInstance)
	{
		SquadCacheWidgetInstance->InitializeSquadCacheWindow(CacheActor, ShooterChar);
		SquadCacheWidgetInstance->AddToViewport(10);
	}

	bSquadCacheWindowOpen = true;
	ApplyInventoryInputMode();
}

void AShooterHUD::CloseSquadCache()
{
	if (!bSquadCacheWindowOpen)
	{
		return;
	}

	if (SquadCacheWidgetInstance)
	{
		SquadCacheWidgetInstance->RemoveFromParent();
	}

	bSquadCacheWindowOpen = false;

	if (!IsAnyInventoryOpen())
	{
		ApplyGameplayInputMode();
	}
}

// ── Post Extraction ──────────────────────────────────────────────────────────

void AShooterHUD::OpenPostExtractionScreen()
{
	if (bPostExtractionOpen)
	{
		return;
	}

	APlayerController* PC = GetOwningPlayerController();
	AShooterGameCharacter* ShooterChar = GetOwningShooterCharacter();
	if (!PC || !ShooterChar)
	{
		return;
	}

	if (PostExtractionWidgetClass && !PostExtractionWidget)
	{
		PostExtractionWidget = CreateWidget<UPostExtractionWidget>(PC, PostExtractionWidgetClass);
	}

	if (PostExtractionWidget)
	{
		PostExtractionWidget->InitializePostExtractionScreen(ShooterChar);
		PostExtractionWidget->AddToViewport(20);
	}

	bPostExtractionOpen = true;
	ApplyInventoryInputMode();
}

void AShooterHUD::ClosePostExtractionScreen()
{
	if (!bPostExtractionOpen)
	{
		return;
	}

	if (PostExtractionWidget)
	{
		PostExtractionWidget->RemoveFromParent();
	}

	bPostExtractionOpen = false;

	if (!IsAnyInventoryOpen())
	{
		ApplyGameplayInputMode();
	}
}

// ── State queries ────────────────────────────────────────────────────────────

bool AShooterHUD::IsAnyInventoryOpen() const
{
	return bMainInventoryOpen || bLootWindowOpen || bSquadCacheWindowOpen || bPostExtractionOpen || bVendorOpen || bQuestbookOpen;
}

// ── Input mode ───────────────────────────────────────────────────────────────

void AShooterHUD::ApplyInventoryInputMode()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}

	// Game + UI mode: player can still move but mouse is visible for drag/drop
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	PC->SetInputMode(InputMode);
	PC->SetShowMouseCursor(true);
}

void AShooterHUD::ApplyGameplayInputMode()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}

	// Full game mode: cursor hidden, all input goes to character
	FInputModeGameOnly InputMode;
	PC->SetInputMode(InputMode);
	PC->SetShowMouseCursor(false);
}

// ── Helpers ──────────────────────────────────────────────────────────────────

AShooterGameCharacter* AShooterHUD::GetOwningShooterCharacter() const
{
	if (APlayerController* PC = GetOwningPlayerController())
	{
		return Cast<AShooterGameCharacter>(PC->GetPawn());
	}

	return nullptr;
}


void AShooterHUD::DrawHUD()
{
	Super::DrawHUD();

	AShooterGamePlayerController* ShooterGamePlayerController = Cast<AShooterGamePlayerController>(PlayerOwner);
	if (!ShooterGamePlayerController) return;
    
	AShooterGameCharacter* ShooterCharacter = Cast<AShooterGameCharacter>(ShooterGamePlayerController->GetPawn());
	if (!ShooterCharacter) return;
    
	UCombatComponent* Combat = ShooterCharacter->FindComponentByClass<UCombatComponent>();
	if (!Combat) return;
    
	const FReticleState& State = Combat->GetReticleState();

	if (State.bIsEquipped)
	{
		const AWeapon* Weapon = ShooterCharacter->GetEquippedWeapon();
		if (!Weapon) return;

		const FReticleConfig& Config = Weapon->GetReticleConfig();

		// TPS — reticle is always locked to screen center, never follows mouse
		int32 SizeX, SizeY;
		ShooterGamePlayerController->GetViewportSize(SizeX, SizeY);
		DrawCrosshairReticle(FVector2D(SizeX * 0.5f, SizeY * 0.5f), State, Config);
	}
}

void AShooterHUD::DrawCrosshairReticle(const FVector2D& Center, const FReticleState& State, const FReticleConfig& Config)
{
	if (!Canvas) return;

	float Radius = FMath::Lerp(Config.BaseRadius, Config.MaxRadius, State.SpreadAlpha);
	if (State.bIsCrouched) Radius *= Config.CrouchMultiplier;
	if (State.bIsAiming)   Radius *= Config.AimMultiplier;

	const float GapSize = Config.CrosshairGapSize;
	const FLinearColor LineColor = Config.LineColor;         // ← now uses Config.LineColor
	const float T = Config.Thickness;

	DrawLine(Center.X, Center.Y - GapSize, Center.X, Center.Y - Radius, LineColor, T);
	DrawLine(Center.X, Center.Y + GapSize, Center.X, Center.Y + Radius, LineColor, T);
	DrawLine(Center.X - GapSize, Center.Y, Center.X - Radius, Center.Y, LineColor, T);
	DrawLine(Center.X + GapSize, Center.Y, Center.X + Radius, Center.Y, LineColor, T);
	
}


void AShooterHUD::OpenVendor(AVendorNPCActor* VendorActor)
{
    if (!IsValid(VendorActor) || !VendorWidgetClass)
    {
        return;
    }

    // Close any other open inventory window first — matches existing pattern.
    if (bMainInventoryOpen)   { CloseInventory(); }
    if (bLootWindowOpen)      { CloseLootContainer(); }
    if (bSquadCacheWindowOpen){ CloseSquadCache(); }
    if (bQuestbookOpen)       { CloseQuestbook(); }

    APlayerController* PC = GetOwningPlayerController();
    if (!PC) { return; }

    if (!VendorWidgetInstance)
    {
        VendorWidgetInstance = CreateWidget<UVendorWidget>(PC, VendorWidgetClass);
    }

    if (VendorWidgetInstance)
    {
        VendorWidgetInstance->AddToViewport();
        VendorWidgetInstance->InitVendor(VendorActor);
        bVendorOpen = true;
        ApplyInventoryInputMode();
    }
}

void AShooterHUD::CloseVendor()
{
    if (VendorWidgetInstance)
    {
        VendorWidgetInstance->RemoveFromParent();
    }

    bVendorOpen = false;

    // Only restore gameplay input if nothing else is open.
    if (!IsAnyInventoryOpen())
    {
        ApplyGameplayInputMode();
    }
}

void AShooterHUD::OpenQuestbook()
{
    if (!QuestbookWidgetClass) { return; }

    if (bVendorOpen)           { CloseVendor(); }
    if (bMainInventoryOpen)    { CloseInventory(); }
    if (bLootWindowOpen)       { CloseLootContainer(); }
    if (bSquadCacheWindowOpen) { CloseSquadCache(); }

    APlayerController* PC = GetOwningPlayerController();
    if (!PC) { return; }

    if (!QuestbookWidgetInstance)
    {
        QuestbookWidgetInstance = CreateWidget<UQuestbookWidget>(PC, QuestbookWidgetClass);
    }

    if (QuestbookWidgetInstance)
    {
        QuestbookWidgetInstance->AddToViewport();
        bQuestbookOpen = true;
        ApplyInventoryInputMode();
    }
}

void AShooterHUD::CloseQuestbook()
{
    if (QuestbookWidgetInstance)
    {
        QuestbookWidgetInstance->RemoveFromParent();
    }

    bQuestbookOpen = false;

    if (!IsAnyInventoryOpen())
    {
        ApplyGameplayInputMode();
    }
}

void AShooterHUD::HandleVendorInteracted(APlayerController* InteractingPlayer, AVendorNPCActor* VendorActor)
{
    // Only open the vendor UI for the local player — in a listen server session
    // this delegate fires for all players, so guard against remote controllers.
    APlayerController* LocalPC = GetOwningPlayerController();
    if (InteractingPlayer != LocalPC)
    {
        return;
    }

    OpenVendor(VendorActor);
}