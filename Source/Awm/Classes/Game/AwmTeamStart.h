// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#pragma once

#include "AwmTeamStart.generated.h"

UCLASS()
class AAwmTeamStart : public APlayerStart
{
	GENERATED_UCLASS_BODY()

	/** Which team can start at this point */
	UPROPERTY(EditInstanceOnly, Category=Team)
	int32 SpawnTeam;

	/** Whether players can start at this point */
	UPROPERTY(EditInstanceOnly, Category=Team)
	uint32 bNotForPlayers:1;

	/** Whether bots can start at this point */
	UPROPERTY(EditInstanceOnly, Category=Team)
	uint32 bNotForBots:1;
};
