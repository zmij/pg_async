// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#include "Awm.h"
#include "AwmUserSettings.h"

UAwmUserSettings::UAwmUserSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetToDefaults();
}

void UAwmUserSettings::SetToDefaults()
{
	Super::SetToDefaults();

	GraphicsQuality = 1;
}

void UAwmUserSettings::ApplySettings(bool bCheckForCommandLineOverrides)
{
	if (GraphicsQuality == 0)
	{
		ScalabilityQuality.SetFromSingleQualityLevel(1);
	}
	else
	{
		ScalabilityQuality.SetFromSingleQualityLevel(3);
	}

	Super::ApplySettings(bCheckForCommandLineOverrides);

	if (!GEngine)
	{
		return;
	}
}
