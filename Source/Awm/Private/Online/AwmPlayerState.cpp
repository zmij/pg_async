// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#include "Awm.h"

AAwmPlayerState::AAwmPlayerState(const class FObjectInitializer& PCIP)
	: Super(PCIP)
{
	//TeamNumber = 0;
}

void AAwmPlayerState::Reset()
{
	Super::Reset();

	//SetTeamNum(0);
}

void AAwmPlayerState::ClientInitialize(class AController* InController)
{
	Super::ClientInitialize(InController);

	//UpdateTeamColors();
}


//////////////////////////////////////////////////////////////////////////
// Replication

/**void AAwmPlayerState::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAwmPlayerState, TeamNumber);
}*/
