// Copyright 2015 Mail.Ru Group. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class AwmEditorTarget : TargetRules
{
	public AwmEditorTarget(TargetInfo Target)
	{
		Type = TargetType.Editor;
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
