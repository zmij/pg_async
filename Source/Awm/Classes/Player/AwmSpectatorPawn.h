// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#pragma once

#include "AwmSpectatorPawn.generated.h"

UCLASS(config = Game, Blueprintable, BlueprintType)
class AAwmSpectatorPawn : public ASpectatorPawn
{
	GENERATED_UCLASS_BODY()


	//////////////////////////////////////////////////////////////////////////
	// Player Input (from Player Controller)

	UFUNCTION(BlueprintNativeEvent, Category = "Awm|Input|Spectator|Touch")
	bool OnTapPressed(const FVector2D& ScreenPosition, float DownTime);

	UFUNCTION(BlueprintNativeEvent, Category = "Awm|Input|Spectator|Touch")
	void OnHoldPressed(const FVector2D& ScreenPosition, float DownTime);

	UFUNCTION(BlueprintNativeEvent, Category = "Awm|Input|Spectator|Touch")
	void OnHoldReleased(const FVector2D& ScreenPosition, float DownTime);

	UFUNCTION(BlueprintNativeEvent, Category = "Awm|Input|Spectator|Touch")
	bool OnSwipeStarted(const FVector2D& SwipePosition, float DownTime);

	UFUNCTION(BlueprintNativeEvent, Category = "Awm|Input|Spectator|Touch")
	bool OnSwipeUpdate(const FVector2D& SwipePosition, float DownTime);

	UFUNCTION(BlueprintNativeEvent, Category = "Awm|Input|Spectator|Touch")
	bool OnSwipeReleased(const FVector2D& SwipePosition, float DownTime);

	UFUNCTION(BlueprintNativeEvent, Category = "Awm|Input|Spectator|Touch")
	void OnSwipeTwoPointsStarted(const FVector2D& ScreenPosition1, const FVector2D& ScreenPosition2, float DownTime);

	UFUNCTION(BlueprintNativeEvent, Category = "Awm|Input|Spectator|Touch")
	void OnSwipeTwoPointsUpdate(const FVector2D& ScreenPosition1, const FVector2D& ScreenPosition2, float DownTime);

	UFUNCTION(BlueprintNativeEvent, Category = "Awm|Input|Spectator|Touch")
	void OnSwipeTwoPointsReleased(const FVector2D& ScreenPosition1, const FVector2D& ScreenPosition2, float DownTime);

	UFUNCTION(BlueprintNativeEvent, Category = "Awm|Input|Spectator|Touch")
	void OnPinchStarted(const FVector2D& AnchorPosition1, const FVector2D& AnchorPosition2, float DownTime);
	
	UFUNCTION(BlueprintNativeEvent, Category = "Awm|Input|Spectator|Touch")
	void OnPinchUpdate(class UAwmInput* InputHandler, const FVector2D& ScreenPosition1, const FVector2D& ScreenPosition2, float DownTime);

	UFUNCTION(BlueprintNativeEvent, Category = "Awm|Input|Spectator|Touch")
	void OnPinchReleased(class UAwmInput* InputHandler, const FVector2D& ScreenPosition1, const FVector2D& ScreenPosition2, float DownTime);

};
