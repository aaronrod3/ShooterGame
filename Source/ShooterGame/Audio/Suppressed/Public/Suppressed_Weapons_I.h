// Copyright SOUNDFX STUDIO © 2023

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FSuppressed_Weapons_IModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
