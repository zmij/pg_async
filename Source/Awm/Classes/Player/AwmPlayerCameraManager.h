// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#pragma once

#include "AwmPlayerCameraManager.generated.h"

UCLASS()
class AAwmPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_UCLASS_BODY()

public:
	/** Normal FOV */
	float NormalFOV;

	/** Targeting FOV */
	float TargetingFOV;

	/** After updating camera, inform pawn to update 1p mesh to match camera's location&rotation */
	virtual void UpdateCamera(float DeltaTime) override;

};
