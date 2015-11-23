// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#include "Awm.h"

AAwmSpectatorPawn::AAwmSpectatorPawn(const class FObjectInitializer& PCIP)
	: Super(PCIP)
{

}

bool AAwmSpectatorPawn::OnTapPressed_Implementation(const FVector2D& ScreenPosition, float DownTime)
{
	return false;
}

void AAwmSpectatorPawn::OnHoldPressed_Implementation(const FVector2D& ScreenPosition, float DownTime)
{

}

void AAwmSpectatorPawn::OnHoldReleased_Implementation(const FVector2D& ScreenPosition, float DownTime)
{

}

bool AAwmSpectatorPawn::OnSwipeStarted_Implementation(const FVector2D& SwipePosition, float DownTime)
{
	return false;
}

bool AAwmSpectatorPawn::OnSwipeUpdate_Implementation(const FVector2D& SwipePosition, float DownTime)
{
	return false;
}

bool AAwmSpectatorPawn::OnSwipeReleased_Implementation(const FVector2D& SwipePosition, float DownTime)
{
	return false;
}

void AAwmSpectatorPawn::OnSwipeTwoPointsStarted_Implementation(const FVector2D& ScreenPosition1, const FVector2D& ScreenPosition2, float DownTime)
{

}

void AAwmSpectatorPawn::OnSwipeTwoPointsUpdate_Implementation(const FVector2D& ScreenPosition1, const FVector2D& ScreenPosition2, float DownTime)
{

}

void AAwmSpectatorPawn::OnSwipeTwoPointsReleased_Implementation(const FVector2D& ScreenPosition1, const FVector2D& ScreenPosition2, float DownTime)
{

}

void AAwmSpectatorPawn::OnPinchStarted_Implementation(const FVector2D& AnchorPosition1, const FVector2D& AnchorPosition2, float DownTime)
{
	
}

void AAwmSpectatorPawn::OnPinchUpdate_Implementation(UAwmInput* InputHandler, const FVector2D& ScreenPosition1, const FVector2D& ScreenPosition2, float DownTime)
{

}

void AAwmSpectatorPawn::OnPinchReleased_Implementation(UAwmInput* InputHandler, const FVector2D& ScreenPosition1, const FVector2D& ScreenPosition2, float DownTime)
{

}
