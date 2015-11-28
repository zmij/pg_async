// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#include "Awm.h"

AAwmGameState::AAwmGameState(const class FObjectInitializer& PCIP)
	: Super(PCIP)
{
	NumTeams = 0;
	RemainingTime = 0;
	bTimerPaused = false;
}

void AAwmGameState::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAwmGameState, NumTeams);
	DOREPLIFETIME(AAwmGameState, RemainingTime);
	DOREPLIFETIME(AAwmGameState, bTimerPaused);
	DOREPLIFETIME(AAwmGameState, TeamScores);
}

void AAwmGameState::RequestFinishAndExitToMainMenu()
{
	if (AuthorityGameMode)
	{
		// we are server, tell the gamemode
		AAwmGameMode* const GameMode = Cast<AAwmGameMode>(AuthorityGameMode);
		if (GameMode)
		{
			GameMode->RequestFinishAndExitToMainMenu();
		}
	}
	else
	{
		// we are client, handle our own business
		AAwmPlayerController* const PrimaryPC = Cast<AAwmPlayerController>(GetGameInstance()->GetFirstLocalPlayerController());
		if (PrimaryPC)
		{
			check(PrimaryPC->GetNetMode() == ENetMode::NM_Client);
			// @todo
			// PrimaryPC->HandleReturnToMainMenu();
		}
	}

}
