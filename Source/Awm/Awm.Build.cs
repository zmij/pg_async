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
                "AIModule"
           }
       );

        PrivateDependencyModuleNames.AddRange(new string[] { "AwmLoadingScreen" });

        // Additional plugins
        PrivateDependencyModuleNames.AddRange(new string[] { "VaRestPlugin" });

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");
        // if ((Target.Platform == UnrealTargetPlatform.Win32) || (Target.Platform == UnrealTargetPlatform.Win64))
        // {
        //		if (UEBuildConfiguration.bCompileSteamOSS == true)
        //		{
        //			DynamicallyLoadedModuleNames.Add("OnlineSubsystemSteam");
        //		}
        // }
    }
}
