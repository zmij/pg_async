// Copyright 2015 Mail.Ru Group. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class AwmTarget : TargetRules
{
	public AwmTarget(TargetInfo Target)
	{
		Type = TargetType.Game;
	}

	//
	// TargetRules interface.
	//

	public override void SetupBinaries(
		TargetInfo Target,
		ref List<UEBuildBinaryConfiguration> OutBuildBinaryConfigurations,
		ref List<string> OutExtraModuleNames
		)
	{
		OutExtraModuleNames.AddRange( new string[] { "Awm" } );
	}
}
