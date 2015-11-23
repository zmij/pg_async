// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#pragma once

#include "AwmHUD.generated.h"

UCLASS()
class AAwmHUD : public AHUD
{
	GENERATED_UCLASS_BODY()
	

	//////////////////////////////////////////////////////////////////////////
	// Loading screen

	/** Show the loading screen */
	UFUNCTION(BlueprintCallable, Category = "Awm|HUD")
	void LaunchGame(FString MapName);

	/** Show the loading screen */
	UFUNCTION(BlueprintCallable, Category = "Awm|HUD")
	void ShowLoadingScreen();


};
