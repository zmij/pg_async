// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#pragma once

#include "AwmSpectatorPawn.generated.h"

UCLASS(config = Game, Blueprintable, BlueprintType)
class AAwmSpectatorPawn : public ASpectatorPawn
{
	GENERATED_UCLASS_BODY()


	//////////////////////////////////////////////////////////////////////////
	// Player Input

	// Begin ASpectatorPawn overrides
	/** Overridden to implement Key Bindings the match the player controls */
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;
	// End Pawn overrides
	
	// Frame rate linked look
	void LookUpAtRate(float Val);


};
