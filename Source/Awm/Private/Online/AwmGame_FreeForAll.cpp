// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#include "Awm.h"

AAwmGame_FreeForAll::AAwmGame_FreeForAll(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bDelayedStart = true;
}

void AAwmGame_FreeForAll::DetermineMatchWinner()
{
	AAwmGameState const* const MyGameState = CastChecked<AAwmGameState>(GameState);
	float BestScore = MAX_FLT;
	int32 BestPlayer = -1;
	int32 NumBestPlayers = 0;

	for (int32 i = 0; i < MyGameState->PlayerArray.Num(); i++)
	{
		const float PlayerScore = MyGameState->PlayerArray[i]->Score;
		if (BestScore < PlayerScore)
		{
			BestScore = PlayerScore;
			BestPlayer = i;
			NumBestPlayers = 1;
		}
		else if (BestScore == PlayerScore)
		{
			NumBestPlayers++;
		}
	}

	WinnerPlayerState = (NumBestPlayers == 1) ? Cast<AAwmPlayerState>(MyGameState->PlayerArray[BestPlayer]) : NULL;
}

bool AAwmGame_FreeForAll::IsWinner(AAwmPlayerState* PlayerState) const
{
	return PlayerState && !PlayerState->IsQuitter() && PlayerState == WinnerPlayerState;
}
