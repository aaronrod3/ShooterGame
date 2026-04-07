#include "ShooterEndScreenWidget.h"
#include "ShooterGame/Player/Controller/ShooterGamePlayerController.h"

void UShooterEndScreenWidget::RequestMatchRestart()
{
	if (AShooterGamePlayerController* PC = GetShooterController())
	{
		PC->RequestMatchRestart();
	}
}

AShooterGamePlayerController* UShooterEndScreenWidget::GetShooterController() const
{
	return Cast<AShooterGamePlayerController>(GetOwningPlayer());
}