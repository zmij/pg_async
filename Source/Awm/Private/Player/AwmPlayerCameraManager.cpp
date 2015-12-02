// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#include "Awm.h"
#include "Player/AwmPlayerCameraManager.h"

AAwmPlayerCameraManager::AAwmPlayerCameraManager(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NormalFOV = 90.0f;
	TargetingFOV = 60.0f;
	ViewPitchMin = -87.0f;
	ViewPitchMax = 87.0f;
	bAlwaysApplyModifiers = true;
}

void AAwmPlayerCameraManager::UpdateCamera(float DeltaTime)
{
	Super::UpdateCamera(DeltaTime);
}
