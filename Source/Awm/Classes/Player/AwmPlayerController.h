// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#pragma once

#include "GameFramework/PlayerController.h"
#include "AwmPlayerController.generated.h"

class AAwmSpectatorPawn;

/**
 * Core class for player controller
 */
UCLASS()
class AAwmPlayerController : public APlayerController
{
	GENERATED_UCLASS_BODY()


	//////////////////////////////////////////////////////////////////////////
	// Initialization

protected:
	// Begin PlayerController interface
	virtual void SetupInputComponent() override;
	virtual void ProcessPlayerInput(const float DeltaTime, const bool bGamePaused) override;
	// End PlayerController interface


	//////////////////////////////////////////////////////////////////////////
	// Player Input

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "Awm|Input|Player|Touch")
	void OnTapPressed(const FVector2D& ScreenPosition, float DownTime);

	UFUNCTION(BlueprintImplementableEvent, Category = "Awm|Input|Player|Touch")
	void OnHoldPressed(const FVector2D& ScreenPosition, float DownTime);

	UFUNCTION(BlueprintImplementableEvent, Category = "Awm|Input|Player|Touch")
	void OnHoldReleased(const FVector2D& ScreenPosition, float DownTime);

	UFUNCTION(BlueprintImplementableEvent, Category = "Awm|Input|Player|Touch")
	void OnSwipeStarted(const FVector2D& AnchorPosition, float DownTime);

	UFUNCTION(BlueprintImplementableEvent, Category = "Awm|Input|Player|Touch")
	void OnSwipeUpdate(const FVector2D& ScreenPosition, float DownTime);

	UFUNCTION(BlueprintImplementableEvent, Category = "Awm|Input|Player|Touch")
	void OnSwipeReleased(const FVector2D& ScreenPosition, float DownTime);

	UFUNCTION(BlueprintImplementableEvent, Category = "Awm|Input|Player|Touch")
	void OnSwipeTwoPointsStarted(const FVector2D& ScreenPosition1, const FVector2D& ScreenPosition2, float DownTime);

	UFUNCTION(BlueprintImplementableEvent, Category = "Awm|Input|Player|Touch")
	void OnSwipeTwoPointsUpdate(const FVector2D& ScreenPosition1, const FVector2D& ScreenPosition2, float DownTime);

	UFUNCTION(BlueprintImplementableEvent, Category = "Awm|Input|Player|Touch")
	void OnSwipeTwoPointsReleased(const FVector2D& ScreenPosition1, const FVector2D& ScreenPosition2, float DownTime);

	UFUNCTION(BlueprintImplementableEvent, Category = "Awm|Input|Player|Touch")
	void OnPinchStarted(const FVector2D& AnchorPosition1, const FVector2D& AnchorPosition2, float DownTime);

	UFUNCTION(BlueprintImplementableEvent, Category = "Awm|Input|Player|Touch")
	void OnPinchUpdate(const FVector2D& ScreenPosition1, const FVector2D& ScreenPosition2, float DownTime);

	UFUNCTION(BlueprintImplementableEvent, Category = "Awm|Input|Player|Touch")
	void OnPinchReleased(const FVector2D& ScreenPosition1, const FVector2D& ScreenPosition2, float DownTime);

protected:
	/** If set, input and camera updates will be ignored */
	uint8 bIgnoreInput : 1;

	/** Custom input handler */
	UPROPERTY()
	class UAwmInput* InputHandler;


	//////////////////////////////////////////////////////////////////////////
	// Helpers

public:
	/** Helper function to toggle input detection */
	UFUNCTION(BlueprintCallable, Category = "Awm|Player")
	void SetIgnoreInput(bool bIgnore);

private:
	/** Helper to return cast version of Spectator pawn */
	AAwmSpectatorPawn* GetAwmSpectatorPawn() const;

public:
	UFUNCTION(BlueprintCallable, Category = "Awm|Player", meta = (bTraceComplex = true))
	bool GetHitResultAtScreenPositionByChannel(const FVector2D ScreenPosition, ETraceTypeQuery TraceChannel, bool bTraceComplex, FHitResult& HitResult) const;

	UFUNCTION(BlueprintCallable, Category = "Awm|Player", meta = (bTraceComplex = true))
	bool GetHitResultAtScreenPositionForObjects(const FVector2D ScreenPosition, const TArray<TEnumAsByte<EObjectTypeQuery> > & ObjectTypes, bool bTraceComplex, FHitResult& HitResult) const;


};
