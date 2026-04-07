#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShooterEndScreenWidget.generated.h"

class AShooterGamePlayerController;

UCLASS()
class SHOOTERGAME_API UShooterEndScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Called by the "Play Again" button in WBP_EndScreen
	UFUNCTION(BlueprintCallable, Category = "Match")
	void RequestMatchRestart();

protected:
	// Convenience getter — casts GetOwningPlayer to our controller type
	AShooterGamePlayerController* GetShooterController() const;
};