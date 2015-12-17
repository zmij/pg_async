// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#pragma once

#include "AwmGame_TeamDeathMatch.generated.h"

class AAwmPlayerState;
class AAwmAIController;

UCLASS()
class AAwmGame_TeamDeathMatch : public AAwmGameMode
{
	GENERATED_UCLASS_BODY()

	/** setup team changes at player login */
	void PostLogin(APlayerController* NewPlayer) override;

	/** initialize replicated game data */
	virtual void InitGameState() override;

	/** can players damage each other? */
	virtual bool CanDealDamage(AAwmPlayerState* DamageInstigator, AAwmPlayerState* DamagedPlayer) const override;

protected:

	/** number of teams */
	int32 NumTeams;

	/** best team */
	int32 WinnerTeam;

	/** pick team with least players in or random when it's equal */
	int32 ChooseTeam(AAwmPlayerState* ForPlayerState) const;

	/** check who won */
	virtual void DetermineMatchWinner() override;
    
    /** Check winner team in tick */
    virtual int32 CheckWinnerTeam() override;

	/** check if PlayerState is a winner */
	virtual bool IsWinner(AAwmPlayerState* PlayerState) const override;

	/** check team constraints */
	virtual bool IsSpawnpointAllowed(APlayerStart* SpawnPoint, AController* Player) const;

	/** initialization for bot after spawning */
	virtual void InitBot(AAwmAIController* AIC, int32 BotNum) override;	

};
