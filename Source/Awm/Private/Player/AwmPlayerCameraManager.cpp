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

	// @todo

	/*AAwmCharacter* MyPawn = PCOwner ? Cast<AAwmCharacter>(PCOwner->GetPawn()) : NULL;
	if (MyPawn && MyPawn->IsFirstPerson())
	{
		const float TargetFOV = MyPawn->IsTargeting() ? TargetingFOV : NormalFOV;
		DefaultFOV = FMath::FInterpTo(DefaultFOV, TargetFOV, DeltaTime, 20.0f);
	}

	Super::UpdateCamera(DeltaTime);

	if (MyPawn && MyPawn->IsFirstPerson())
	{
		MyPawn->OnCameraUpdate(GetCameraLocation(), GetCameraRotation());
	}*/
}
