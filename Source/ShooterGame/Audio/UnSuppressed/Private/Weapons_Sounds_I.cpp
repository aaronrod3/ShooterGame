// Copyright SOUNDFX STUDIO © 2023

#include "Weapons_Sounds_I.h"

#define LOCTEXT_NAMESPACE "FWeapons_Sounds_IModule"

void FWeapons_Sounds_IModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FWeapons_Sounds_IModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FWeapons_Sounds_IModule, Weapons_Sounds_I)