// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#include "Awm.h"

void UAwmOnlineSession::HandleDisconnect(UWorld *World, UNetDriver *NetDriver)
{
	UAwmGameInstance* AwmGameInstance = Cast<UAwmGameInstance>(World->GetAuthGameMode());
	if (AwmGameInstance != nullptr)
	{
		FString ReturnReason = NSLOCTEXT("NetworkErrors", "Unexpected Error", "Unexpected connection error occured").ToString();
		AwmGameInstance->NotifyConnectionError(ReturnReason);
	}

	TArray<AActor*> AwmHUDs;
	UGameplayStatics::GetAllActorsOfClass(World, AAwmHUD::StaticClass(), AwmHUDs);
	check(AwmHUDs.Num() <= 1);
	if (AwmHUDs.Num() == 1)
	{
		Cast<AAwmHUD>(AwmHUDs[0])->HandleReturnToMainMenu();
	}
}


