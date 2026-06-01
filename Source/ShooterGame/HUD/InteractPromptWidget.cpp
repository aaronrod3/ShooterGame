// Source/ShooterGame/HUD/InteractPromptWidget.cpp
#include "HUD/InteractPromptWidget.h"

void UInteractPromptWidget::SetPromptText(const FText& NewText)
{
	PromptText = NewText;
	UpdatePromptDisplay(); // fires Blueprint event if overridden
}