// Source/ShooterGame/HUD/Inventory/SquadCacheWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SquadCacheWidget.generated.h"

class ASquadCacheActor;
class AShooterGameCharacter;
class USubContainerWidget;

UCLASS()
class SHOOTERGAME_API USquadCacheWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Squad Cache Widget")
	void InitializeSquadCacheWindow(ASquadCacheActor* InCacheActor, AShooterGameCharacter* InPlayerCharacter);

	UFUNCTION(BlueprintCallable, Category = "Squad Cache Widget")
	void CloseSquadCacheWindow();

	UFUNCTION(BlueprintPure, Category = "Squad Cache Widget")
	ASquadCacheActor* GetCacheActor() const { return CacheActor; }

protected:
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Squad Cache Widget")
	void BP_OnSquadCacheWindowInitialized(ASquadCacheActor* InCacheActor, AShooterGameCharacter* InPlayerCharacter);

	UFUNCTION(BlueprintImplementableEvent, Category = "Squad Cache Widget")
	void BP_OnSquadCacheWindowClosed();

	// Left panel — shared squad cache inventory
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Squad Cache Widget")
	TObjectPtr<USubContainerWidget> CacheContainerPanel;

	// Right panel — compressed player inventory (no paper doll)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Squad Cache Widget")
	TObjectPtr<USubContainerWidget> PlayerInventoryPanel;

	UPROPERTY(BlueprintReadOnly, Category = "Squad Cache Widget", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ASquadCacheActor> CacheActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Squad Cache Widget", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AShooterGameCharacter> OwningCharacter = nullptr;
};