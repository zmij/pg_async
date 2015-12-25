// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#pragma once

#include "AwmGameState.generated.h"

UCLASS()
class AAwmGameState : public AGameState
{
	GENERATED_UCLASS_BODY()

public:
	/** number of teams in current game (doesn't deprecate when no players are left in a team) */
	UPROPERTY(Transient, Replicated)
	int32 NumTeams;

    /** accumulated death score per team */
    UPROPERTY(Transient, Replicated)
    TArray<int32> TeamDeathScores;
    
    /** accumulated score per team */
    UPROPERTY(Transient, Replicated)
    TArray<int32> TeamScores;

	/** time left for warmup / match */
	UPROPERTY(Transient, Replicated)
	float RemainingTime;

	/** is timer paused? */
	UPROPERTY(Transient, Replicated)
	bool bTimerPaused;

	void RequestFinishAndExitToMainMenu();
    
    /** Add team scores from game mode */
    void AddTeamScores(int32 Team, int32 Scores);
};
