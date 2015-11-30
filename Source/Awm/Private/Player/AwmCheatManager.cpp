// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#include "Awm.h"

UAwmCheatManager::UAwmCheatManager(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UAwmCheatManager::ToggleInfiniteAmmo()
{
	AAwmPlayerController* MyPC = GetOuterAAwmPlayerController();

	MyPC->SetInfiniteAmmo(!MyPC->HasInfiniteAmmo());
	MyPC->ClientMessage(FString::Printf(TEXT("Infinite ammo: %s"), MyPC->HasInfiniteAmmo() ? TEXT("ENABLED") : TEXT("off")));
}

void UAwmCheatManager::ToggleInfiniteClip()
{
	AAwmPlayerController* MyPC = GetOuterAAwmPlayerController();

	MyPC->SetInfiniteClip(!MyPC->HasInfiniteClip());
	MyPC->ClientMessage(FString::Printf(TEXT("Infinite clip: %s"), MyPC->HasInfiniteClip() ? TEXT("ENABLED") : TEXT("off")));
}

void UAwmCheatManager::ToggleMatchTimer()
{
	AAwmPlayerController* MyPC = GetOuterAAwmPlayerController();

	AAwmGameState* const MyGameState = Cast<AAwmGameState>(MyPC->GetWorld()->GameState);
	if (MyGameState && MyGameState->Role == ROLE_Authority)
	{
		MyGameState->bTimerPaused = !MyGameState->bTimerPaused;
		MyPC->ClientMessage(FString::Printf(TEXT("Match timer: %s"), MyGameState->bTimerPaused ? TEXT("PAUSED") : TEXT("running")));
	}
}

void UAwmCheatManager::ForceMatchStart()
{
	AAwmPlayerController* const MyPC = GetOuterAAwmPlayerController();

	AAwmGameMode* const MyGame = MyPC->GetWorld()->GetAuthGameMode<AAwmGameMode>();
	if (MyGame && MyGame->GetMatchState() == MatchState::WaitingToStart)
	{
		MyGame->StartMatch();
	}
}

void UAwmCheatManager::ChangeTeam(int32 NewTeamNumber)
{
	AAwmPlayerController* MyPC = GetOuterAAwmPlayerController();

	AAwmPlayerState* MyPlayerState = Cast<AAwmPlayerState>(MyPC->PlayerState);
	if (MyPlayerState && MyPlayerState->Role == ROLE_Authority)
	{
		MyPlayerState->SetTeamNum(NewTeamNumber);
		MyPC->ClientMessage(FString::Printf(TEXT("Team changed to: %d"), MyPlayerState->GetTeamNum()));
	}
}

void UAwmCheatManager::Cheat(const FString& Msg)
{
	GetOuterAAwmPlayerController()->ServerCheat(Msg.Left(128));
}

void UAwmCheatManager::SpawnBot()
{
	AAwmPlayerController* const MyPC = GetOuterAAwmPlayerController();
	APawn* const MyPawn = MyPC->GetPawn();
	AAwmGameMode* const MyGame = MyPC->GetWorld()->GetAuthGameMode<AAwmGameMode>();
	UWorld* World = MyPC->GetWorld();
	if (MyPawn && MyGame && World)
	{
		static int32 CheatBotNum = 50;
		AAwmAIController* AwmAIController = MyGame->CreateBot(CheatBotNum++);
		MyGame->RestartPlayer(AwmAIController);		
	}
}
