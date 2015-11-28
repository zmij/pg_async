// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#include "Awm.h"

UAwmViewportClient::UAwmViewportClient(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetSuppressTransitionMessage(true);
}

void UAwmViewportClient::NotifyPlayerAdded(int32 PlayerIndex, ULocalPlayer* AddedPlayer)
{
	Super::NotifyPlayerAdded(PlayerIndex, AddedPlayer);

 	UAwmLocalPlayer* const AwmLP = Cast<UAwmLocalPlayer>(AddedPlayer);
 	if (AwmLP)
 	{
 		AwmLP->LoadPersistentUser();
 	}
}

#if WITH_EDITOR
void UAwmViewportClient::DrawTransition(UCanvas* Canvas)
{
	if (GetOuterUEngine() != NULL)
	{
		TEnumAsByte<enum ETransitionType> Type = GetOuterUEngine()->TransitionType;
		switch (Type)
		{
		case TT_Connecting:
			DrawTransitionMessage(Canvas, NSLOCTEXT("GameViewportClient", "ConnectingMessage", "CONNECTING").ToString());
			break;
		case TT_WaitingToConnect:
			DrawTransitionMessage(Canvas, NSLOCTEXT("GameViewportClient", "Waitingtoconnect", "Waiting to connect...").ToString());
			break;	
		}
	}
}
#endif //WITH_EDITOR
