// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "AwmWidgetFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class AWM_API UAwmWidgetFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:

	UFUNCTION(BlueprintCallable, Category = "Awm|WidgetUtils")
	void GetWidgetAbsolutePosition(UUserWidget* Widget, FVector2D& AbsolutePosition);
	
	
};
