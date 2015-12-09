// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#include "Awm.h"
#include "AwmLoadingScreen.h"

#include "DisplayDebugHelpers.h"

AAwmHUD::AAwmHUD(const class FObjectInitializer& PCIP)
	: Super(PCIP)
{

}


//////////////////////////////////////////////////////////////////////////
// Loading screen

void AAwmHUD::LaunchGame(FString MapName)
{
	FString StartStr = TEXT("/Game/Maps/") + MapName;
	GetWorld()->ServerTravel(StartStr);
	ShowLoadingScreen("AwmLoadingScreen");
}

void AAwmHUD::ShowLoadingScreen_Implementation(const FString& PendingURL)
{
	IAwmLoadingScreenModule* LoadingScreenModule = FModuleManager::LoadModulePtr<IAwmLoadingScreenModule>("AwmLoadingScreen");
	if (LoadingScreenModule != nullptr)
	{
		LoadingScreenModule->StartInGameLoadingScreen();
	}
}
