// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#pragma once

#include "GameFramework/OnlineSession.h"
#include "AwmOnlineSession.generated.h"

/**
 * 
 */
UCLASS()
class AWM_API UAwmOnlineSession : public UOnlineSession
{
	GENERATED_BODY()
	
	
	/** Called to tear down any online sessions and return to main menu	 */
	virtual void HandleDisconnect(UWorld *World, class UNetDriver *NetDriver);
	
};
