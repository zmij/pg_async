// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#pragma once

#include "AwmTypes.h"
#include "AwmViewportClient.generated.h"

UCLASS(Within=Engine, transient, config=Engine)
class UAwmViewportClient : public UGameViewportClient
{
	GENERATED_UCLASS_BODY()

public:

 	// Begin UGameViewportClient interface
 	void NotifyPlayerAdded( int32 PlayerIndex, ULocalPlayer* AddedPlayer ) override;

#if WITH_EDITOR
	virtual void DrawTransition(class UCanvas* Canvas) override;
#endif //WITH_EDITOR
	// End UGameViewportClient interface
};
