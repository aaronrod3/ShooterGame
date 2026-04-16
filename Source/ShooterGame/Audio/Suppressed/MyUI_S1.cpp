// Copyright SOUNDFX STUDIO © 2023

#include "MyUI_S1.h"

void UMyUI_S1::ErrorMessages(int32 n)
{
	if (!txt_errors)
		return;

	switch (n)
	{
	case 0:
		txt_errors->SetText(FText::FromString("PLEASE ADD SFX COMPONENT\nTO THE WEAPON"));
		break;
	case 99:
	default:
		// DUMMY
		break;
	}

	PlayAnimation(anim_TxtErrors);
}

void UMyUI_S1::UI_Init()
{
	weapons.Add(weap_1);
	weapons.Add(weap_2);
	weapons.Add(weap_3);
	weapons.Add(weap_4);
	weapons.Add(weap_5);
	weapons.Add(weap_6);
	weapons.Add(weap_7);
	weapons.Add(weap_8);
	weapons.Add(weap_9);
	UI_SwitchWeapon(0);
	UI_Fire(false);
	UI_FireMode(true);
}

void UMyUI_S1::UI_SwitchWeapon(int wpnNumber)
{
	for (UBorder* wpn : weapons)
	{
		SetWpnOnOff(wpn, false);
	}
	if (weapons.Num() >= wpnNumber + 1)
		SetWpnOnOff(weapons[wpnNumber], true);
}

void UMyUI_S1::UI_Fire(bool bOn)
{
	if (bOn)
		PlayAnimation(anim_Fire_Pressed);
	SetWpnOnOff(action_1, bOn);
}

void UMyUI_S1::UI_FireMode(bool bAuto)
{
	SetWpnOnOff(action_2, bAuto);
}

void UMyUI_S1::UI_FireMode_Anim()
{
	PlayAnimation(anim_Firemode_Pressed);
}

// Switch UI Buttons On/Off
void UMyUI_S1::SetWpnOnOff(UBorder* wpn, bool on /*= true*/)
{
	if (!wpn)
		return;

	FLinearColor setColor = on ? txt_on_color : txt_off_color;
	wpn->SetContentColorAndOpacity(setColor);
	setColor = on ? txt_on_color : brush_off_color;
	wpn->SetBrushColor(setColor);
}