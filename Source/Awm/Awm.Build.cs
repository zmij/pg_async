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
                "EngineSettings",
                "InputCore",
                "AIModule",
                "PhysX",
                "APEX",
                "UMG"
           }
       );

        PrivateDependencyModuleNames.AddRange(new string[] { "AwmLoadingScreen" });

        // Uncomment if you are using Slate UI
        //PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

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
