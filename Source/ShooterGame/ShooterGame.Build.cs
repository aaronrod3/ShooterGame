// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ShooterGame : ModuleRules
{
	public ShooterGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"ShooterGame",
			"ShooterGame/Variant_Platforming",
			"ShooterGame/Variant_Platforming/Animation",
			"ShooterGame/Variant_Combat",
			"ShooterGame/Variant_Combat/AI",
			"ShooterGame/Variant_Combat/Animation",
			"ShooterGame/Variant_Combat/Gameplay",
			"ShooterGame/Variant_Combat/Interfaces",
			"ShooterGame/Variant_Combat/UI",
			"ShooterGame/Variant_SideScrolling",
			"ShooterGame/Variant_SideScrolling/AI",
			"ShooterGame/Variant_SideScrolling/Gameplay",
			"ShooterGame/Variant_SideScrolling/Interfaces",
			"ShooterGame/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
