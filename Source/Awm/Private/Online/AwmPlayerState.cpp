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

	//PlayerStates persist across seamless travel.  Keep the same teams as previous match.
	//SetTeamNum(0);
	NumKills = 0;
	NumDeaths = 0;
	NumBulletsFired = 0;
	NumRocketsFired = 0;
	bQuitter = false;
}

void AAwmPlayerState::UnregisterPlayerWithSession()
{
	if (!bFromPreviousLevel)
	{
		Super::UnregisterPlayerWithSession();
	}
}

void AAwmPlayerState::ClientInitialize(AController* InController)
{
	Super::ClientInitialize(InController);

	UpdateTeamColors();
}

void AAwmPlayerState::SetTeamNum(int32 NewTeamNumber)
{
	TeamNumber = NewTeamNumber;

	UpdateTeamColors();
}

void AAwmPlayerState::OnRep_TeamColor()
{
	UpdateTeamColors();
}

void AAwmPlayerState::AddBulletsFired(int32 NumBullets)
{
	NumBulletsFired += NumBullets;
}

void AAwmPlayerState::AddRocketsFired(int32 NumRockets)
{
	NumRocketsFired += NumRockets;
}

void AAwmPlayerState::SetQuitter(bool bInQuitter)
{
	bQuitter = bInQuitter;
}

void AAwmPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	AAwmPlayerState* AwmPlayer = Cast<AAwmPlayerState>(PlayerState);
	if (AwmPlayer)
	{
		AwmPlayer->TeamNumber = TeamNumber;
	}
}

void AAwmPlayerState::UpdateTeamColors()
{
	AController* OwnerController = Cast<AController>(GetOwner());
	if (OwnerController != NULL)
	{
		AAwmCharacter* AwmCharacter = Cast<AAwmCharacter>(OwnerController->GetCharacter());
		if (AwmCharacter != NULL)
		{
			AwmCharacter->UpdateTeamColorsAllMIDs();
		}
	}
}

int32 AAwmPlayerState::GetTeamNum() const
{
	return TeamNumber;
}

int32 AAwmPlayerState::GetKills() const
{
	return NumKills;
}

int32 AAwmPlayerState::GetDeaths() const
{
	return NumDeaths;
}

float AAwmPlayerState::GetScore() const
{
	return Score;
}

int32 AAwmPlayerState::GetNumBulletsFired() const
{
	return NumBulletsFired;
}

int32 AAwmPlayerState::GetNumRocketsFired() const
{
	return NumRocketsFired;
}

bool AAwmPlayerState::IsQuitter() const
{
	return bQuitter;
}

void AAwmPlayerState::ScoreKill(AAwmPlayerState* Victim, int32 Points)
{
	NumKills++;
	ScorePoints(Points);
}

void AAwmPlayerState::ScoreDeath(AAwmPlayerState* KilledBy, int32 Points)
{
	NumDeaths++;
	ScorePoints(Points);
}

void AAwmPlayerState::ScorePoints(int32 Points)
{
	AAwmGameState* const MyGameState = Cast<AAwmGameState>(GetWorld()->GameState);
	if (MyGameState && TeamNumber >= 0)
	{
		if (TeamNumber >= MyGameState->TeamScores.Num())
		{
			MyGameState->TeamScores.AddZeroed(TeamNumber - MyGameState->TeamScores.Num() + 1);
		}

		MyGameState->TeamScores[TeamNumber] += Points;
	}

	Score += Points;
}

void AAwmPlayerState::InformAboutKill_Implementation(class AAwmPlayerState* KillerPlayerState, const UDamageType* KillerDamageType, class AAwmPlayerState* KilledPlayerState)
{
	//id can be null for bots
	if (KillerPlayerState->UniqueId.IsValid())
	{
		//search for the actual killer before calling OnKill()	
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			AAwmPlayerController* TestPC = Cast<AAwmPlayerController>(*It);
			if (TestPC && TestPC->IsLocalController())
			{
				// a local player might not have an ID if it was created with CreateDebugPlayer.
				ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(TestPC->Player);
				TSharedPtr<const FUniqueNetId> LocalID = LocalPlayer->GetCachedUniqueNetId();
				if (LocalID.IsValid() && *LocalPlayer->GetCachedUniqueNetId() == *KillerPlayerState->UniqueId)
				{
					TestPC->OnKill();
				}
			}
		}
	}
}

void AAwmPlayerState::BroadcastDeath_Implementation(class AAwmPlayerState* KillerPlayerState, const UDamageType* KillerDamageType, class AAwmPlayerState* KilledPlayerState)
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		// all local players get death messages so they can update their huds.
		AAwmPlayerController* TestPC = Cast<AAwmPlayerController>(*It);
		if (TestPC && TestPC->IsLocalController())
		{
			TestPC->OnDeathMessage(KillerPlayerState, this, KillerDamageType);
		}
	}
}

FString AAwmPlayerState::GetShortPlayerName() const
{
	if (PlayerName.Len() > MAX_PLAYER_NAME_LENGTH)
	{
		return PlayerName.Left(MAX_PLAYER_NAME_LENGTH) + "...";
	}
	return PlayerName;
}


//////////////////////////////////////////////////////////////////////////
// Replication

void AAwmPlayerState::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAwmPlayerState, TeamNumber);
	DOREPLIFETIME(AAwmPlayerState, NumKills);
	DOREPLIFETIME(AAwmPlayerState, NumDeaths);
}
