// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#pragma once

#include "AwmDamageType.generated.h"

// DamageType class that specifies an icon to display
UCLASS(const, Blueprintable, BlueprintType)
class UAwmDamageType : public UDamageType
{
	GENERATED_UCLASS_BODY()

	/** icon displayed in death messages log when killed with this weapon */
	UPROPERTY(EditDefaultsOnly, Category=HUD)
	FCanvasIcon KillIcon;

	/** force feedback effect to play on a player hit by this damage type */
	UPROPERTY(EditDefaultsOnly, Category=Effects)
	UForceFeedbackEffect *HitForceFeedback;

	/** force feedback effect to play on a player killed by this damage type */
	UPROPERTY(EditDefaultsOnly, Category=Effects)
	UForceFeedbackEffect *KilledForceFeedback;
};



