// Source/ShooterGame/HUD/InteractPromptWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InteractPromptWidget.generated.h"

/**
 * Minimal C++ parent for WBP_InteractPrompt.
 * Exposes SetPromptText so the character can push text
 * without casting to a Blueprint class.
 *
 * Add a TextBlock bound to PromptText in the UMG designer.
 * Override UpdatePromptDisplay in Blueprint to drive
 * any animation, icon, or key hint you want later.
 */
UCLASS()
class SHOOTERGAME_API UInteractPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	// Called by AShooterGameCharacter to push the current prompt text.
	// Blueprint can override UpdatePromptDisplay to animate or style the change.
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetPromptText(const FText& NewText);

	// Expose the current text so Blueprint bindings can read it
	UFUNCTION(BlueprintPure, Category = "Interaction")
	FText GetPromptText() const { return PromptText; }

protected:

	// The current prompt string — bind a TextBlock to this in UMG
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	FText PromptText;

	// Blueprint override hook — fires every time SetPromptText is called.
	// Use this to play a fade-in animation or update a key hint icon.
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void UpdatePromptDisplay();
};