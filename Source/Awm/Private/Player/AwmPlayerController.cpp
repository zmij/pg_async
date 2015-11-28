// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#include "Awm.h"

AAwmPlayerController::AAwmPlayerController(const class FObjectInitializer& PCIP)
	: Super(PCIP)
{
	PrimaryActorTick.bCanEverTick = true;
	bShowMouseCursor = true;
	bIgnoreInput = false;
}

//////////////////////////////////////////////////////////////////////////
// Initialization

void AAwmPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputHandler = NewObject<UAwmInput>(this);

	BIND_1P_ACTION(InputHandler, EGameKey::Tap, IE_Pressed, &AAwmPlayerController::OnTapPressed);
	BIND_1P_ACTION(InputHandler, EGameKey::Hold, IE_Pressed, &AAwmPlayerController::OnHoldPressed);
	BIND_1P_ACTION(InputHandler, EGameKey::Hold, IE_Released, &AAwmPlayerController::OnHoldReleased);
	BIND_1P_ACTION(InputHandler, EGameKey::Swipe, IE_Pressed, &AAwmPlayerController::OnSwipeStarted);
	BIND_1P_ACTION(InputHandler, EGameKey::Swipe, IE_Repeat, &AAwmPlayerController::OnSwipeUpdate);
	BIND_1P_ACTION(InputHandler, EGameKey::Swipe, IE_Released, &AAwmPlayerController::OnSwipeReleased);
	BIND_2P_ACTION(InputHandler, EGameKey::SwipeTwoPoints, IE_Pressed, &AAwmPlayerController::OnSwipeTwoPointsStarted);
	BIND_2P_ACTION(InputHandler, EGameKey::SwipeTwoPoints, IE_Repeat, &AAwmPlayerController::OnSwipeTwoPointsUpdate);
	BIND_2P_ACTION(InputHandler, EGameKey::SwipeTwoPoints, IE_Released, &AAwmPlayerController::OnSwipeTwoPointsReleased);
	BIND_2P_ACTION(InputHandler, EGameKey::Pinch, IE_Pressed, &AAwmPlayerController::OnPinchStarted);
	BIND_2P_ACTION(InputHandler, EGameKey::Pinch, IE_Repeat, &AAwmPlayerController::OnPinchUpdate);
	BIND_2P_ACTION(InputHandler, EGameKey::Pinch, IE_Released, &AAwmPlayerController::OnPinchReleased);
}

void AAwmPlayerController::ProcessPlayerInput(const float DeltaTime, const bool bGamePaused)
{
	// Process input with our custom input handler
	if (!bGamePaused && PlayerInput && InputHandler && !bIgnoreInput) {
		InputHandler->UpdateDetection(DeltaTime);
	}

	Super::ProcessPlayerInput(DeltaTime, bGamePaused);
}


//////////////////////////////////////////////////////////////////////////
// Helpers

void AAwmPlayerController::SetIgnoreInput(bool bIgnore)
{
	bIgnoreInput = bIgnore;
}

AAwmSpectatorPawn* AAwmPlayerController::GetAwmSpectatorPawn() const
{
	return Cast<AAwmSpectatorPawn>(GetPawnOrSpectator());
}

bool AAwmPlayerController::GetHitResultAtScreenPositionByChannel(const FVector2D ScreenPosition, ETraceTypeQuery TraceChannel, bool bTraceComplex, FHitResult& HitResult) const
{
	return GetHitResultAtScreenPosition(ScreenPosition, TraceChannel, bTraceComplex, HitResult);
}

bool AAwmPlayerController::GetHitResultAtScreenPositionForObjects(const FVector2D ScreenPosition, 
	const TArray<TEnumAsByte<EObjectTypeQuery> > & ObjectTypes, bool bTraceComplex, FHitResult& HitResult) const
{
	return GetHitResultAtScreenPosition(ScreenPosition, ObjectTypes, bTraceComplex, HitResult);
}
