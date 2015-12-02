// Copyright 2015 Mail.Ru Group. All Rights Reserved.

using UnrealBuildTool;

public class Awm : ModuleRules
{
	public Awm(TargetInfo Target)
	{
        PublicDependencyModuleNames.AddRange(
           new string[] {
                "Core",
                "CoreUObject",
                "Engine",
                "InputCore",
                "AIModule",
                "PhysX",
                "APEX"
           }
       );

        PrivateDependencyModuleNames.AddRange(new string[] { "AwmLoadingScreen" });

        // Additional plugins
        PrivateDependencyModuleNames.AddRange(new string[] { "VaRestPlugin" });

        // iOS requires special build process
        if (Target.Platform == UnrealTargetPlatform.IOS)
        {
            PublicDependencyModuleNames.Remove("APEX");
            PublicDependencyModuleNames.Remove("PhysX");

            SetupModulePhysXAPEXSupport(Target);
        }
    }
}
