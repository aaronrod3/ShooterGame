// Source/ShooterGame/HUD/Inventory/PostExtractionWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShooterGame/Types/ContainerTypes.h"
#include "PostExtractionWidget.generated.h"

class AShooterGameCharacter;
class UShooterSaveGameSubsystem;
class UEquipmentPanelWidget;
class UStashWindowWidget;

UCLASS()
class SHOOTERGAME_API UPostExtractionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Call this immediately after extraction success to populate both panels
	UFUNCTION(BlueprintCallable, Category = "Post Extraction")
	void InitializePostExtractionScreen(AShooterGameCharacter* InPlayerCharacter);

	// Called when the player is done reorganizing and ready to return to lobby
	// Commits current stash and equipped state to disk
	UFUNCTION(BlueprintCallable, Category = "Post Extraction")
	void CommitAndClose();

	UFUNCTION(BlueprintPure, Category = "Post Extraction")
	bool HasUnreviewedExtraction() const;

	UFUNCTION(BlueprintPure, Category = "Post Extraction")
	FEquippedStateSnapshot GetExtractionSnapshot() const { return ExtractionSnapshot; }

protected:
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Post Extraction")
	void BP_OnPostExtractionInitialized(const FEquippedStateSnapshot& Snapshot);

	UFUNCTION(BlueprintImplementableEvent, Category = "Post Extraction")
	void BP_OnCommitted();

	// Left panel — equipment state as-extracted (read-only until player reorganizes)
	// Uses UEquipmentPanelWidget so extracted gear is shown slot-by-slot
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Post Extraction")
	TObjectPtr<UEquipmentPanelWidget> ExtractionEquipmentPanel;

	// Right panel — full stash contents for drag-reorganize
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Post Extraction")
	TObjectPtr<UStashWindowWidget> StashPanel;

	UPROPERTY(BlueprintReadOnly, Category = "Post Extraction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AShooterGameCharacter> OwningCharacter = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Post Extraction", meta = (AllowPrivateAccess = "true"))
	FEquippedStateSnapshot ExtractionSnapshot;

private:
	UShooterSaveGameSubsystem* GetSaveSubsystem() const;
};