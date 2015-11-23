// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#pragma once

#include "AIController.h"
#include "AwmAIController.generated.h"

/**
 * Brains for each unit of squad
 */
UCLASS()
class AAwmAIController : public AAIController
{
	GENERATED_UCLASS_BODY()


public:
	/** Master switch: allows disabling all interactions */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Awm|AI")
	void EnableLogic(bool bEnable);

	/** Returns information if we have logic enabled or disabled */
	UFUNCTION(BlueprintCallable, Category = "Awm|AI")
	bool IsLogicEnabled() const;

protected:
	/** master switch state */
	uint8 bLogicEnabled : 1;

};
