// Copyright SOUNDFX STUDIO © 2023

#include "Suppressed_Weapons_I.h"

#define LOCTEXT_NAMESPACE "FSuppressed_Weapons_IModule"

void FSuppressed_Weapons_IModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FSuppressed_Weapons_IModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSuppressed_Weapons_IModule, Suppressed_Weapons_I)