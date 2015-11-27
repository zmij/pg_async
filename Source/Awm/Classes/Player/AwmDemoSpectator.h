// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#pragma once

#include "AwmDemoSpectator.generated.h"

class SAwmDemoHUD;

UCLASS(config=Game)
class AAwmDemoSpectator : public APlayerController
{
	GENERATED_UCLASS_BODY()

public:
	virtual void SetupInputComponent() override;
	virtual void SetPlayer( UPlayer* Player ) override;

	void OnIncreasePlaybackSpeed();
	void OnDecreasePlaybackSpeed();

	int32 PlaybackSpeed;

};
