// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#include "Awm.h"

UAwmInput::UAwmInput(const FObjectInitializer& ObjectInitializer) 
	: Super(ObjectInitializer)
	, PrevTouchState(0)
{
	// Init default touch cache
	for (int i = 0; i < 4; i++) {
		TouchCache.Add(FFingerTouch());
	}
}

void UAwmInput::UpdateDetection(float DeltaTime)
{
	UpdateTouchCache(DeltaTime);
	UpdateGameKeys(DeltaTime);
	ProcessKeyStates(DeltaTime);
}

void UAwmInput::ProcessKeyStates(float DeltaTime)
{
	for (const FActionBinding1P& AB : ActionBindings1P)
	{
		const FSimpleKeyState* KeyState = KeyStateMap.Find(AB.Key);

		if (KeyState && KeyState->Events[AB.KeyEvent] > 0)
		{
			AB.ActionDelegate.ExecuteIfBound(KeyState->Position, KeyState->DownTime);
		}
	}

	for (const FActionBinding2P& AB : ActionBindings2P)
	{
		const FSimpleKeyState* KeyState = KeyStateMap.Find(AB.Key);

		if (KeyState && KeyState->Events[AB.KeyEvent] > 0)
		{
			AB.ActionDelegate.ExecuteIfBound(KeyState->Position, KeyState->Position2, KeyState->DownTime);
		}
	}

	// Update states
	for (TMap<EGameKey::Type,FSimpleKeyState>::TIterator It(KeyStateMap); It; ++It)
	{
		FSimpleKeyState* const KeyState = &It.Value();

		if (KeyState->Events[IE_Pressed])
		{
			KeyState->bDown = true;
		}
		else if (KeyState->Events[IE_Released])
		{
			KeyState->bDown = false;
		}

		FMemory::Memzero(KeyState->Events, sizeof(KeyState->Events));
	}
}

void UAwmInput::UpdateTouchCache(float DeltaTime)
{
	AAwmPlayerController* MyController = CastChecked<AAwmPlayerController>(GetOuter());

	// Cache touch input states
	int32 TouchCount = FMath::Min(TouchCache.Num(), (int32)ARRAY_COUNT(MyController->PlayerInput->Touches));
	for (int32 i = 0; i < TouchCount; i++)
	{
		// Update location
		TouchCache[i].TouchLocation = FVector2D(MyController->PlayerInput->Touches[i]);

		// Check pressed state
		if (MyController->PlayerInput->Touches[i].Z == 0)
		{
			TouchCache[i].bFingerDown = false;
		}
		else
		{
			TouchCache[i].bFingerDown = true;
		}

		// Check consumation flag
		if (TouchCache[i].bFingerDown == false)
		{
			TouchCache[i].bConsumed = false;
		}
	}

	// Let chance to HUD to process events by itself
	AAwmHUD* MyHUD = Cast<AAwmHUD>(MyController->GetHUD());
	if (MyHUD) {
		MyHUD->ProcessTouchEvents(TouchCache);
	}
}

void UAwmInput::UpdateGameKeys(float DeltaTime)
{
	AAwmPlayerController* MyController = CastChecked<AAwmPlayerController>(GetOuter());

	// Prepare unconsumed 
	TArray<FFingerTouch> UnconsumedInput;
	UnconsumedInput.SetNum(2);

	// Cache touch input states
	int32 TouchCount = FMath::Min(TouchCache.Num(), UnconsumedInput.Num());
	for (int32 i = 0; i < TouchCount; i++)
	{
		// Check unconsumed fingers
		if (!TouchCache[i].bConsumed)
		{
			UnconsumedInput[i] = TouchCache[i];
		}
	}

	// Gather current states
	uint32 CurrentTouchState = 0;
	for (int32 i = 0; i < UnconsumedInput.Num(); i++)
	{
		if (UnconsumedInput[i].bFingerDown) 
		{
			CurrentTouchState |= (1 << i);
		}
	}

	// Detection
	FVector2D LocalPosition1 = UnconsumedInput[0].TouchLocation;
	FVector2D LocalPosition2 = UnconsumedInput[1].TouchLocation;

	DetectOnePointActions(CurrentTouchState & 1, PrevTouchState & 1, DeltaTime, LocalPosition1, TouchAnchors[0], Touch0DownTime);
	DetectTwoPointsActions((CurrentTouchState & 1) && (CurrentTouchState & 2), (PrevTouchState & 1) && (PrevTouchState & 2), DeltaTime, LocalPosition1, LocalPosition2);

	// Save states
	PrevTouchState = CurrentTouchState;
}

void UAwmInput::DetectOnePointActions(bool bCurrentState, bool bPrevState, float DeltaTime, const FVector2D& CurrentPosition, FVector2D& AnchorPosition, float& DownTime)
{
	const float HoldTime = 0.3f;

	if (bCurrentState && !bTwoPointsTouch)
	{
		// Just pressed? set anchor and zero time
		if (!bPrevState)
		{
			DownTime = 0;
			AnchorPosition = CurrentPosition;
		}

		// Swipe detection & upkeep
		FSimpleKeyState& SwipeState = KeyStateMap.FindOrAdd(EGameKey::Swipe);
		if (SwipeState.bDown)
		{
			SwipeState.Events[IE_Repeat]++;
			SwipeState.Position = CurrentPosition;
			SwipeState.DownTime = DownTime;
		}
		else if ((AnchorPosition - CurrentPosition).SizeSquared() > 0)
			{
				SwipeState.Events[IE_Pressed]++;
				SwipeState.Position = AnchorPosition;
				SwipeState.DownTime = DownTime;
			}


		// Hold detection
		if (DownTime + DeltaTime > HoldTime && DownTime <= HoldTime && !SwipeState.bDown)
		{
			FSimpleKeyState& HoldState = KeyStateMap.FindOrAdd(EGameKey::Hold);
			HoldState.Events[IE_Pressed]++;
			HoldState.Position = AnchorPosition;
			HoldState.DownTime = DownTime;
		}

		DownTime += DeltaTime;
	}
	else
	{
		// Just released? 
		if (bPrevState)
		{
			// Tap detection
			if (DownTime < HoldTime)
			{
				FSimpleKeyState& TapState = KeyStateMap.FindOrAdd(EGameKey::Tap);
				TapState.Events[IE_Pressed]++;
				TapState.Position = AnchorPosition;
				TapState.DownTime = DownTime;
			}
			else
			{
				FSimpleKeyState& HoldState = KeyStateMap.FindOrAdd(EGameKey::Hold);
				if (HoldState.bDown)
				{
					HoldState.Events[IE_Released]++;
					HoldState.Position = AnchorPosition;
					HoldState.DownTime = DownTime;
				}
			}

			// Swipe finish
			FSimpleKeyState& SwipeState = KeyStateMap.FindOrAdd(EGameKey::Swipe);
			if (SwipeState.bDown)
			{
				SwipeState.Events[IE_Released]++;
				SwipeState.Position = CurrentPosition;
				SwipeState.DownTime = DownTime;
			}
		}
	}
}

void UAwmInput::DetectTwoPointsActions(bool bCurrentState, bool bPrevState, float DeltaTime, const FVector2D& CurrentPosition1, const FVector2D& CurrentPosition2)
{
	const float MaxSwipeDistance = 150.0f;			// Swipe only if initial distance is lower
	const float PinchDistanceThreshold = 150.0f;	// Don't break pinch if distance exceeded threshold
	// @fixme Don't break pinch ever (50.0f by default)
	const float PinchMoveThreshold = 5000.0f;		// Break pinch if midpoint moved further from initial spot 

	bTwoPointsTouch = bCurrentState;
	if (bCurrentState)
	{
		// Just pressed? set anchors, time and pinch/swipe distinction
		if (!bPrevState)
		{
			TouchAnchors[0] = CurrentPosition1;
			TouchAnchors[1] = CurrentPosition2;
			TwoPointsDownTime = 0.0f;
			MaxPinchDistanceSq = 0.0f;

			const float DistanceSq = (CurrentPosition1 - CurrentPosition2).SizeSquared();
			if (DistanceSq < FMath::Square(MaxSwipeDistance))
			{
				FSimpleKeyState& SwipeState = KeyStateMap.FindOrAdd(EGameKey::SwipeTwoPoints);
				SwipeState.Events[IE_Pressed]++;
				SwipeState.Position = CurrentPosition1;
				SwipeState.Position2 = CurrentPosition2;
				SwipeState.DownTime = TwoPointsDownTime;
			}

			FSimpleKeyState& PinchState = KeyStateMap.FindOrAdd(EGameKey::Pinch);
			PinchState.Events[IE_Pressed]++;
			PinchState.Position = CurrentPosition1;
			PinchState.Position2 = CurrentPosition2;
			PinchState.DownTime = TwoPointsDownTime;
		}

		FVector2D AnchorMidPoint = (TouchAnchors[0] + TouchAnchors[1]) * 0.5f;
		FVector2D CurrentMidPoint = (CurrentPosition1 + CurrentPosition2) * 0.5f;
		float MovementDistanceSq = (CurrentMidPoint - AnchorMidPoint).SizeSquared();
		float PinchDistanceSq = FMath::Abs((CurrentPosition2 - CurrentPosition1).SizeSquared() - (TouchAnchors[1] - TouchAnchors[0]).SizeSquared());
		MaxPinchDistanceSq = FMath::Max(PinchDistanceSq, MaxPinchDistanceSq);

		// Finish swipe if distance changed before midpoint moved away from anchors
		FSimpleKeyState& SwipeState = KeyStateMap.FindOrAdd(EGameKey::SwipeTwoPoints);
		if (SwipeState.bDown)
		{
			bool bFinishSwipe = false;
			if (MovementDistanceSq < FMath::Square(PinchMoveThreshold) &&
				MaxPinchDistanceSq > FMath::Square(PinchDistanceThreshold))
			{
				bFinishSwipe = true;
			}

			SwipeState.Events[bFinishSwipe ? IE_Released : IE_Repeat]++;
			SwipeState.Position = CurrentPosition1;
			SwipeState.Position2 = CurrentPosition2;
			SwipeState.DownTime = TwoPointsDownTime;
		}

		// Finish pinch if midpoint moved away from anchors before any distance changed
		FSimpleKeyState& PinchState = KeyStateMap.FindOrAdd(EGameKey::Pinch);
		if (PinchState.bDown)
		{
			bool bFinishPinch = false;
			if (MovementDistanceSq > FMath::Square(PinchMoveThreshold) &&
				MaxPinchDistanceSq < FMath::Square(PinchDistanceThreshold))
			{
				bFinishPinch = true;
			}

			PinchState.Events[bFinishPinch ? IE_Released : IE_Repeat]++;
			PinchState.Position = CurrentPosition1;
			PinchState.Position2 = CurrentPosition2;
			PinchState.DownTime = TwoPointsDownTime;
		}

		TwoPointsDownTime += DeltaTime;
	} 
	else 
	{
		// Just released?
		if (bPrevState)
		{
			// Swipe finish
			FSimpleKeyState& SwipeState = KeyStateMap.FindOrAdd(EGameKey::SwipeTwoPoints);
			if (SwipeState.bDown)
			{
				SwipeState.Events[IE_Released]++;
				SwipeState.Position = CurrentPosition1;
				SwipeState.Position2 = CurrentPosition2;
				SwipeState.DownTime = TwoPointsDownTime;
			}

			// Pinch finish
			FSimpleKeyState& PinchState = KeyStateMap.FindOrAdd(EGameKey::Pinch);
			if (PinchState.bDown)
			{
				PinchState.Events[IE_Released]++;
				PinchState.Position = CurrentPosition1;
				PinchState.Position2 = CurrentPosition2;
				PinchState.DownTime = TwoPointsDownTime;
			}
		}
	}
}

FVector UAwmInput::GetTouchLocation(int32 i) const
{
	AAwmPlayerController* MyController = CastChecked<AAwmPlayerController>(GetOuter());

	return (i >= 0 && i < EKeys::NUM_TOUCH_KEYS) ? MyController->PlayerInput->Touches[i] : FVector::ZeroVector;
}

FFingerTouch UAwmInput::GetTouchCache(int32 i) const
{
	return (i >= 0 && i < TouchCache.Num()) ? TouchCache[i] : FFingerTouch();
}

bool UAwmInput::ConsumeTouch(int32 i)
{
	if (i >= 0 && i < TouchCache.Num()) 
	{
		TouchCache[i].bConsumed = true;
		return true;
	}

	return false;
}

FVector2D UAwmInput::GetTouchAnchor(int32 i) const
{
	return (i >= 0 && i < ARRAY_COUNT(TouchAnchors)) ? TouchAnchors[i] : FVector2D::ZeroVector;
}

bool UAwmInput::SetTouchAnchor(int32 i, FVector2D NewPosition)
{
	// Update desired anchor with new value
	if (i >= 0 && i < ARRAY_COUNT(TouchAnchors)) 
	{
		TouchAnchors[i] = NewPosition;
		return true;
	}

	return false;
}
