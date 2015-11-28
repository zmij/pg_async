// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#pragma once

#include "AwmEngine.generated.h"

UCLASS()
class AWM_API UAwmEngine : public UGameEngine
{
	GENERATED_UCLASS_BODY()

	/* Hook up specific callbacks */
	virtual void Init(IEngineLoop* InEngineLoop);

public:

	/**
	 * 	All regular engine handling, plus update AwmKing state appropriately.
	 */
	virtual void HandleNetworkFailure(UWorld *World, UNetDriver *NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString) override;
};

