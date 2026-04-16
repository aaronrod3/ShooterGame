// Copyright SOUNDFX STUDIO © 2023

#pragma once

#include "CoreMinimal.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Animation/WidgetAnimation.h"
#include "MyUI_S1.generated.h"

UCLASS()
class SUPPRESSED_WEAPONS_I_API UMyUI_S1 : public UUserWidget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "MyUI_S1 Functions")
		void UI_Init();
	UFUNCTION(BlueprintCallable, Category = "MyUI_S1 Functions")
		void UI_SwitchWeapon(int wpnNumber);
	UFUNCTION(BlueprintCallable, Category = "MyUI_S1 Functions")
		void UI_Fire(bool bOn);
	UFUNCTION(BlueprintCallable, Category = "MyUI_S1 Functions")
		void UI_FireMode(bool bAuto);
	UFUNCTION(BlueprintCallable, Category = "MyUI_S1 Functions")
		void UI_FireMode_Anim();

	// Error Messages on the screen
	UFUNCTION(BlueprintCallable, Category = "MyUI_S1 Functions")
		void ErrorMessages(int32 n);

protected:

	// ref to Animation for UI Error Text
	//UPROPERTY(BlueprintReadWrite, Category = "MyUI_S1 Properties")
		//UWidgetAnimation* animTxtErrors;

private:

	UPROPERTY(meta = (BindWidget))
		UTextBlock* txt_errors;
	UPROPERTY(Transient, meta = (BindWidgetAnim))
		UWidgetAnimation* anim_TxtErrors;
	UPROPERTY(Transient, meta = (BindWidgetAnim))
		UWidgetAnimation* anim_Fire_Pressed;
	UPROPERTY(Transient, meta = (BindWidgetAnim))
		UWidgetAnimation* anim_Firemode_Pressed;


	UPROPERTY(meta = (BindWidget))
		UBorder* weap_1;
	UPROPERTY(meta = (BindWidget))
		UBorder* weap_2;
	UPROPERTY(meta = (BindWidget))
		UBorder* weap_3;
	UPROPERTY(meta = (BindWidget))
		UBorder* weap_4;
	UPROPERTY(meta = (BindWidget))
		UBorder* weap_5;
	UPROPERTY(meta = (BindWidget))
		UBorder* weap_6;
	UPROPERTY(meta = (BindWidget))
		UBorder* weap_7;
	UPROPERTY(meta = (BindWidget))
		UBorder* weap_8;
	UPROPERTY(meta = (BindWidget))
		UBorder* weap_9;


	UPROPERTY(meta = (BindWidget))
		UBorder* action_1;
	UPROPERTY(meta = (BindWidget))
		UBorder* action_2;


	// Switch UI Buttons On/Off
	void SetWpnOnOff(UBorder* wpn, bool on = true);

	// Colors for button's borders
	FLinearColor txt_on_color = FLinearColor(1.f, 1.f, 1.f, 1.f);
	FLinearColor txt_off_color = FLinearColor(1.f, 1.f, 1.f, 0.4f);
	FLinearColor brush_off_color = FLinearColor(1.f, 1.f, 1.f, 0.f);

	TArray<UBorder*> weapons;

	// Is Cockpit Snd On
	bool bIsCockpitSnd = false;
};