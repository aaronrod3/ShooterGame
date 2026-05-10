// Source/ShooterGame/HUD/ShooterHUD.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ShooterGame/Items/Weapon/Weapon.h"
#include "ShooterGame/Components/CombatComponent.h"
#include "ShooterGame/HUD/Widgets/HUDWidget.h"
#include "ShooterGame/HUD/Inventory/VendorWidget.h"
#include "ShooterGame/HUD/Inventory/QuestbookWidget.h"
#include "ShooterGame/Interaction/VendorNPCActor.h"
#include "ShooterHUD.generated.h"

class AShooterGamePlayerController;
class ALootContainerActor;
class ASquadCacheActor;
class AShooterGameCharacter;
class UStashWindowWidget;
class UEquipmentPanelWidget;
class ULootContainerWidget;
class USquadCacheWidget;
class UPostExtractionWidget;
class UQuickSlotBarWidget;
class AVendorNPCActor;
class UVendorWidget;
class UQuestbookWidget;

UCLASS()
class SHOOTERGAME_API AShooterHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void DrawHUD() override;

	// ── Weapon binding ──────────────────────────────────────────────────────
	void BindToWeapon(AWeapon* NewWeapon);

	// ── Inventory Window Orchestration ──────────────────────────────────────

	// Toggles the main inventory screen (stash + equipment panel) open/closed
	UFUNCTION(BlueprintCallable, Category = "HUD|Inventory")
	void ToggleInventory();

	UFUNCTION(BlueprintCallable, Category = "HUD|Inventory")
	void OpenInventory();

	UFUNCTION(BlueprintCallable, Category = "HUD|Inventory")
	void CloseInventory();

	// Opens the two-panel loot window for a world loot container actor
	UFUNCTION(BlueprintCallable, Category = "HUD|Inventory")
	void OpenLootContainer(ALootContainerActor* LootActor);

	UFUNCTION(BlueprintCallable, Category = "HUD|Inventory")
	void CloseLootContainer();

	// Opens the two-panel squad cache window
	UFUNCTION(BlueprintCallable, Category = "HUD|Inventory")
	void OpenSquadCache(ASquadCacheActor* CacheActor);

	UFUNCTION(BlueprintCallable, Category = "HUD|Inventory")
	void CloseSquadCache();

	// Opens the post-extraction review screen
	UFUNCTION(BlueprintCallable, Category = "HUD|Inventory")
	void OpenPostExtractionScreen();

	UFUNCTION(BlueprintCallable, Category = "HUD|Inventory")
	void ClosePostExtractionScreen();
	
	// Opens the vendor shop + quest widget for the given vendor actor.
	// Called by the OnVendorInteracted delegate bound in BeginPlay.
	UFUNCTION(BlueprintCallable, Category = "HUD|Inventory")
	void OpenVendor(AVendorNPCActor* VendorActor);

	UFUNCTION(BlueprintCallable, Category = "HUD|Inventory")
	void CloseVendor();

	// Opens the full questbook overlay.
	UFUNCTION(BlueprintCallable, Category = "HUD|Inventory")
	void OpenQuestbook();

	UFUNCTION(BlueprintCallable, Category = "HUD|Inventory")
	void CloseQuestbook();

	UFUNCTION(BlueprintPure, Category = "HUD|Inventory")
	bool IsVendorOpen() const { return bVendorOpen; }

	UFUNCTION(BlueprintPure, Category = "HUD|Inventory")
	bool IsQuestbookOpen() const { return bQuestbookOpen; }

	// Returns true if any inventory-type window is currently open
	UFUNCTION(BlueprintPure, Category = "HUD|Inventory")
	bool IsAnyInventoryOpen() const;

	UFUNCTION(BlueprintPure, Category = "HUD|Inventory")
	bool IsMainInventoryOpen() const { return bMainInventoryOpen; }

	UFUNCTION(BlueprintPure, Category = "HUD|Inventory")
	bool IsLootWindowOpen() const { return bLootWindowOpen; }

	UFUNCTION(BlueprintPure, Category = "HUD|Inventory")
	bool IsSquadCacheWindowOpen() const { return bSquadCacheWindowOpen; }

	UFUNCTION(BlueprintPure, Category = "HUD|Inventory")
	bool IsPostExtractionOpen() const { return bPostExtractionOpen; }

private:
	// ── Crosshair / reticle ─────────────────────────────────────────────────
	void DrawCrosshairReticle(const FVector2D& Center, const FReticleState& State, const FReticleConfig& Config);

	UPROPERTY(EditAnywhere, Category = "HUD|Reticle")
	FReticleConfig UnequippedReticle;

	// ── HUD widget ──────────────────────────────────────────────────────────
	UPROPERTY()
	UHUDWidget* HUDWidget = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Widgets")
	TSubclassOf<UHUDWidget> HUDWidgetClass;

	// ── Ammo binding ────────────────────────────────────────────────────────
	UPROPERTY()
	AWeapon* BoundWeapon = nullptr;

	UFUNCTION()
	void OnAmmoChanged(int32 MagRounds, int32 MagCapacity);

	// ── Inventory widget classes (set in HUD Blueprint defaults) ────────────
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Inventory")
	TSubclassOf<UStashWindowWidget> StashWindowWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Inventory")
	TSubclassOf<UEquipmentPanelWidget> EquipmentPanelWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Inventory")
	TSubclassOf<ULootContainerWidget> LootContainerWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Inventory")
	TSubclassOf<USquadCacheWidget> SquadCacheWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Inventory")
	TSubclassOf<UPostExtractionWidget> PostExtractionWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Inventory")
	TSubclassOf<UQuickSlotBarWidget> QuickSlotBarWidgetClass;
	
	// ── Quest widget classes ────────────
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Inventory")
	TSubclassOf<UVendorWidget> VendorWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Inventory")
	TSubclassOf<UQuestbookWidget> QuestbookWidgetClass;
	
	// Bound to AVendorNPCActor::OnVendorInteracted in BeginPlay.
	// Fired when the local player interacts with any vendor in the world.
	UFUNCTION()
	void HandleVendorInteracted(APlayerController* InteractingPlayer, AVendorNPCActor* VendorActor);
	

	// ── Live widget instances ────────────────────────────────────────────────
	UPROPERTY()
	TObjectPtr<UStashWindowWidget> StashWindowWidget;

	UPROPERTY()
	TObjectPtr<UEquipmentPanelWidget> EquipmentPanelWidget;

	UPROPERTY()
	TObjectPtr<ULootContainerWidget> LootContainerWidget;

	UPROPERTY()
	TObjectPtr<USquadCacheWidget> SquadCacheWidgetInstance;

	UPROPERTY()
	TObjectPtr<UPostExtractionWidget> PostExtractionWidget;

	UPROPERTY()
	TObjectPtr<UQuickSlotBarWidget> QuickSlotBarWidget;
	
	UPROPERTY()
	TObjectPtr<UVendorWidget> VendorWidgetInstance;

	UPROPERTY()
	TObjectPtr<UQuestbookWidget> QuestbookWidgetInstance;

	// ── Open state flags ────────────────────────────────────────────────────
	bool bMainInventoryOpen = false;
	bool bLootWindowOpen = false;
	bool bSquadCacheWindowOpen = false;
	bool bPostExtractionOpen = false;
	bool bVendorOpen = false;
	bool bQuestbookOpen = false;

	// ── Input mode helpers ──────────────────────────────────────────────────
	void ApplyInventoryInputMode();
	void ApplyGameplayInputMode();

	AShooterGameCharacter* GetOwningShooterCharacter() const;
};