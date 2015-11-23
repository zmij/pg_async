// Copyright 2015 Mail.Ru Group. All Rights Reserved.

using UnrealBuildTool;

// This module must be loaded "PreLoadingScreen" in the .uproject file, otherwise it will not hook in time!

public class AwmLoadingScreen : ModuleRules
{
    public AwmLoadingScreen(TargetInfo Target)
	{
		PrivateIncludePaths.Add("../../Awm/Source/AwmLoadingScreen/Private");

        PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine"
			}
		);

        PrivateDependencyModuleNames.AddRange(
			new string[] {
				"MoviePlayer",
				"Slate",
				"SlateCore",
				"InputCore"
			}
		);
	}
}
