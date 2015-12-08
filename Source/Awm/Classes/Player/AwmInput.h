// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#pragma once

#include "Game/AwmTypes.h"
#include "AwmInput.generated.h"

DECLARE_DELEGATE_TwoParams(FOnePointActionSignature, const FVector2D&, float);
DECLARE_DELEGATE_ThreeParams(FTwoPointsActionSignature, const FVector2D&, const FVector2D&, float);

#define BIND_1P_ACTION(Handler, ActionKey, ActionEvent, Delegate)	\
{ \
	int32 Idx = Handler->ActionBindings1P.AddZeroed(); \
	Handler->ActionBindings1P[Idx].Key = ActionKey; \
	Handler->ActionBindings1P[Idx].KeyEvent = ActionEvent; \
	Handler->ActionBindings1P[Idx].ActionDelegate.BindUObject(this, Delegate); \
}

#define BIND_2P_ACTION(Handler, ActionKey, ActionEvent, Delegate)	\
{ \
	int32 Idx = Handler->ActionBindings2P.AddZeroed(); \
	Handler->ActionBindings2P[Idx].Key = ActionKey; \
	Handler->ActionBindings2P[Idx].KeyEvent = ActionEvent; \
	Handler->ActionBindings2P[Idx].ActionDelegate.BindUObject(this, Delegate); \
}

struct FActionBinding1P
{
	/** Key to bind it to */
	EGameKey::Type Key;

	/** Key event to bind it to, e.g. pressed, released, dblclick */
	TEnumAsByte<EInputEvent> KeyEvent;

	/** Action */
	FOnePointActionSignature ActionDelegate;
};

struct FActionBinding2P
{
	/** Key to bind it to */
	EGameKey::Type Key;

	/** Key event to bind it to, e.g. pressed, released, dblclick */
	TEnumAsByte<EInputEvent> KeyEvent;

	/** Action */
	FTwoPointsActionSignature ActionDelegate;
};

struct FSimpleKeyState
{
	/** Current events indexed with: IE_Pressed, IE_Released, IE_Repeat */
	uint8 Events[3];

	/** Is it pressed? (unused in tap & hold) */
	uint8 bDown : 1;

	/** Position associated with event */
	FVector2D Position;

	/** Position associated with event */
	FVector2D Position2;

	/** Accumulated down time */
	float DownTime;

	FSimpleKeyState()
	{
		FMemory::Memzero(this, sizeof(FSimpleKeyState));
	}
};

UCLASS()
class UAwmInput : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	/** Bindings for custom game events */
	TArray<FActionBinding1P> ActionBindings1P;
	TArray<FActionBinding2P> ActionBindings2P;

	/** Update detection */
	void UpdateDetection(float DeltaTime);

	/** Get touch anchor position */
	UFUNCTION(BlueprintCallable, Category = "Awm|Input")
	FVector GetTouchLocation(int32 i) const;

	/** Get touch anchor position */
	UFUNCTION(BlueprintCallable, Category = "Awm|Input")
	FVector2D GetTouchAnchor(int32 i) const;

	/** Force update touch anchor position */
	UFUNCTION(BlueprintCallable, Category = "Awm|Input")
	bool SetTouchAnchor(int32 i, FVector2D NewPosition);

protected:
	/** Game key states */
	TMap<EGameKey::Type, FSimpleKeyState> KeyStateMap;

	/** Touch anchors */
	FVector2D TouchAnchors[2];

	/** How long was touch 0 pressed? */
	float Touch0DownTime;

	/** How long was two points pressed? */
	float TwoPointsDownTime;

	/** Max distance delta for current pinch */
	float MaxPinchDistanceSq;

	/** Prev touch states for recognition */
	uint32 PrevTouchState;

	/** Is two points touch active? */
	bool bTwoPointsTouch;

	/** Update game key recognition */
	void UpdateGameKeys(float DeltaTime);

	/** Process input state and call handlers */
	void ProcessKeyStates(float DeltaTime);

	/** Detect one point actions (touch and mouse) */
	void DetectOnePointActions(bool bCurrentState, bool bPrevState, float DeltaTime, const FVector2D& CurrentPosition, FVector2D& AnchorPosition, float& DownTime);

	/** Detect two points actions (touch only) */
	void DetectTwoPointsActions(bool bCurrentState, bool bPrevState, float DeltaTime, const FVector2D& CurrentPosition1, const FVector2D& CurrentPosition2);

};
