// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#pragma once

#include "AwmPlayerState.generated.h"

UCLASS()
class AAwmPlayerState : public APlayerState
{
	GENERATED_UCLASS_BODY()

	// Begin APlayerState interface
	/** Clear scores */
	virtual void Reset() override;

	/** Set the team */
	virtual void ClientInitialize(class AController* InController) override;
	// End APlayerState interface

	
};
