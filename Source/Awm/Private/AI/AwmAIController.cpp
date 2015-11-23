// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#include "Awm.h"

#include "VisualLogger/VisualLogger.h"

AAwmAIController::AAwmAIController(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer) //.SetDefaultSubobjectClass<UPathFollowingComponent>(TEXT("PathFollowingComponent")))
	, bLogicEnabled(false)	// Disable logic by default because it will be turned on after squad placement
{

}

void AAwmAIController::EnableLogic_Implementation(bool bEnable)
{
	bLogicEnabled = bEnable;
}

bool AAwmAIController::IsLogicEnabled() const
{
	return bLogicEnabled;
}
