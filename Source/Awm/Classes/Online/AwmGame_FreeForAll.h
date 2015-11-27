// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#pragma once

#include "AwmGame_FreeForAll.generated.h"

class AAwmPlayerState;

UCLASS()
class AAwmGame_FreeForAll : public AAwmGameMode
{
	GENERATED_UCLASS_BODY()

protected:

	/** best player */
	UPROPERTY(transient)
	AAwmPlayerState* WinnerPlayerState;

	/** check who won */
	virtual void DetermineMatchWinner() override;

	/** check if PlayerState is a winner */
	virtual bool IsWinner(AAwmPlayerState* PlayerState) const override;
};
