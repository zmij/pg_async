// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#include "Awm.h"

AAwmGameMode::AAwmGameMode(const FObjectInitializer& ObjectInitializer) 
	: Super(ObjectInitializer)
{
	DefaultPawnClass = AAwmVehicle::StaticClass();

	HUDClass = AAwmHUD::StaticClass();
	PlayerControllerClass = AAwmPlayerController::StaticClass();
	PlayerStateClass = AAwmPlayerState::StaticClass();
	SpectatorClass = AAwmSpectatorPawn::StaticClass();
	GameStateClass = AAwmGameState::StaticClass();
	ReplaySpectatorPlayerControllerClass = AAwmDemoSpectator::StaticClass();

	MinRespawnDelay = 5.0f;

	bAllowBots = true;
	bNeedsBotCreation = true;
	bUseSeamlessTravel = true;
}

FString AAwmGameMode::GetBotsCountOptionName()
{
	return FString(TEXT("Bots"));
}

void AAwmGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	const int32 BotsCountOptionValue = UGameplayStatics::GetIntOption(Options, GetBotsCountOptionName(), 0);
	SetAllowBots(BotsCountOptionValue > 0 ? true : false, BotsCountOptionValue);

	Super::InitGame(MapName, Options, ErrorMessage);
}

void AAwmGameMode::SetAllowBots(bool bInAllowBots, int32 InMaxBots)
{
	bAllowBots = bInAllowBots;
	MaxBots = InMaxBots;
}

void AAwmGameMode::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	GetWorldTimerManager().SetTimer(TimerHandle_DefaultTimer, this, &AAwmGameMode::DefaultTimer, GetWorldSettings()->GetEffectiveTimeDilation(), true);
}

void AAwmGameMode::DefaultTimer()
{                                                                                                                                                                           
	// don't update timers for Play In Editor mode, it's not real match
	if (GetWorld()->IsPlayInEditor())
	{
		// start match if necessary.
		if (GetMatchState() == MatchState::WaitingToStart)
		{
			StartMatch();
		}
		return;
	}

	AAwmGameState* const MyGameState = Cast<AAwmGameState>(GameState);
	if (MyGameState && MyGameState->RemainingTime > 0 && !MyGameState->bTimerPaused)
	{
		MyGameState->RemainingTime--;
		
		if (MyGameState->RemainingTime <= 0)
		{
			if (GetMatchState() == MatchState::WaitingPostMatch)
			{
				RestartGame();
			}
			else if (GetMatchState() == MatchState::InProgress)
			{
				FinishMatch();
			}
			else if (GetMatchState() == MatchState::WaitingToStart)
			{
				StartMatch();
			}
		}
	}
}

void AAwmGameMode::HandleMatchIsWaitingToStart()
{
	if (bNeedsBotCreation)
	{
		CreateBotControllers();
		bNeedsBotCreation = false;
	}

	if (bDelayedStart)
	{
		// start warmup if needed
		AAwmGameState* const MyGameState = Cast<AAwmGameState>(GameState);
		if (MyGameState && MyGameState->RemainingTime == 0)
		{
			const bool bWantsMatchWarmup = !GetWorld()->IsPlayInEditor();
			if (bWantsMatchWarmup && WarmupTime > 0)
			{
				MyGameState->RemainingTime = WarmupTime;
			}
			else
			{
				MyGameState->RemainingTime = 0.0f;
			}
		}
	}
}

void AAwmGameMode::HandleMatchHasStarted()
{
	bNeedsBotCreation = true;
	Super::HandleMatchHasStarted();

	AAwmGameState* const MyGameState = Cast<AAwmGameState>(GameState);
	MyGameState->RemainingTime = RoundTime;	
	StartBots();	

	// notify players
	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		AAwmPlayerController* PC = Cast<AAwmPlayerController>(*It);
		if (PC)
		{
			PC->ClientGameStarted();
		}
	}
}

void AAwmGameMode::FinishMatch()
{
	AAwmGameState* const MyGameState = Cast<AAwmGameState>(GameState);
	if (IsMatchInProgress())
	{
		EndMatch();
		DetermineMatchWinner();		

		// notify players
		for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
		{
			AAwmPlayerState* PlayerState = Cast<AAwmPlayerState>((*It)->PlayerState);
			const bool bIsWinner = IsWinner(PlayerState);

			(*It)->GameHasEnded(NULL, bIsWinner);
		}

		// lock all pawns
		// pawns are not marked as keep for seamless travel, so we will create new pawns on the next match rather than
		// turning these back on.
		for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
		{
			(*It)->TurnOff();
		}

		// set up to restart the match
		MyGameState->RemainingTime = TimeBetweenMatches;
	}
}

void AAwmGameMode::RequestFinishAndExitToMainMenu()
{
	FinishMatch();

	AAwmPlayerController* LocalPrimaryController = nullptr;
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		AAwmPlayerController* Controller = Cast<AAwmPlayerController>(*Iterator);

		if (Controller == NULL)
		{
			continue;
		}

		if (!Controller->IsLocalController())
		{
			const FString RemoteReturnReason = NSLOCTEXT("NetworkErrors", "HostHasLeft", "Host has left the game.").ToString();
			Controller->ClientReturnToMainMenu(RemoteReturnReason);
		}
		else
		{
			LocalPrimaryController = Controller;
		}
	}

	// GameInstance should be calling this from an EndState.  So call the PC function that performs cleanup, not the one that sets GI state.
	if (LocalPrimaryController != NULL)
	{
		LocalPrimaryController->HandleReturnToMainMenu();
	}
}

void AAwmGameMode::DetermineMatchWinner()
{
	// nothing to do here
}

bool AAwmGameMode::IsWinner(class AAwmPlayerState* PlayerState) const
{
	return false;
}

void AAwmGameMode::PreLogin(const FString& Options, const FString& Address, const TSharedPtr<const FUniqueNetId>& UniqueId, FString& ErrorMessage)
{
	AAwmGameState* const MyGameState = Cast<AAwmGameState>(GameState);
	const bool bMatchIsOver = MyGameState && MyGameState->HasMatchEnded();
	if( bMatchIsOver )
	{
		ErrorMessage = TEXT("Match is over!");
	}
	else
	{
		// GameSession can be NULL if the match is over
		Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	}
}


void AAwmGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// update spectator location for client
	AAwmPlayerController* NewPC = Cast<AAwmPlayerController>(NewPlayer);
	if (NewPC && NewPC->GetPawn() == NULL)
	{
		NewPC->ClientSetSpectatorCamera(NewPC->GetSpawnLocation(), NewPC->GetControlRotation());
	}

	// notify new player if match is already in progress
	if (NewPC && IsMatchInProgress())
	{
		NewPC->ClientGameStarted();
		//NewPC->ClientStartOnlineGame();
	}
}

void AAwmGameMode::Killed(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, const UDamageType* DamageType)
{
	AAwmPlayerState* KillerPlayerState = Killer ? Cast<AAwmPlayerState>(Killer->PlayerState) : NULL;
	AAwmPlayerState* VictimPlayerState = KilledPlayer ? Cast<AAwmPlayerState>(KilledPlayer->PlayerState) : NULL;

	if (KillerPlayerState && KillerPlayerState != VictimPlayerState)
	{
		KillerPlayerState->ScoreKill(VictimPlayerState, KillScore);
		KillerPlayerState->InformAboutKill(KillerPlayerState, DamageType, VictimPlayerState);
	}

	if (VictimPlayerState)
	{
		VictimPlayerState->ScoreDeath(KillerPlayerState, DeathScore);
		VictimPlayerState->BroadcastDeath(KillerPlayerState, DamageType, VictimPlayerState);
	}
}

float AAwmGameMode::ModifyDamage(float Damage, AActor* DamagedActor, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) const
{
	float ActualDamage = Damage;

	AAwmVehicle* DamagedPawn = Cast<AAwmVehicle>(DamagedActor);
	if (DamagedPawn && EventInstigator)
	{
		AAwmPlayerState* DamagedPlayerState = Cast<AAwmPlayerState>(DamagedPawn->PlayerState);
		AAwmPlayerState* InstigatorPlayerState = Cast<AAwmPlayerState>(EventInstigator->PlayerState);

		// disable friendly fire
		if (!CanDealDamage(InstigatorPlayerState, DamagedPlayerState))
		{
			ActualDamage = 0.0f;
		}

		// scale self instigated damage
		if (InstigatorPlayerState == DamagedPlayerState)
		{
			ActualDamage *= DamageSelfScale;
		}
	}

	return ActualDamage;
}

bool AAwmGameMode::CanDealDamage(class AAwmPlayerState* DamageInstigator, class AAwmPlayerState* DamagedPlayer) const
{
	return true;
}

bool AAwmGameMode::AllowCheats(APlayerController* P)
{
	return true;
}

bool AAwmGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
	return false;
}

UClass* AAwmGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if (InController->IsA<AAwmAIController>())
	{
		return BotPawnClass;
	}

	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

AActor* AAwmGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	TArray<APlayerStart*> PreferredSpawns;
	TArray<APlayerStart*> FallbackSpawns;

	APlayerStart* BestStart = NULL;
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* TestSpawn = *It;
		if (TestSpawn->IsA<APlayerStartPIE>())
		{
			// Always prefer the first "Play from Here" PlayerStart, if we find one while in PIE mode
			BestStart = TestSpawn;
			break;
		}
		else
		{
			if (IsSpawnpointAllowed(TestSpawn, Player))
			{
				if (IsSpawnpointPreferred(TestSpawn, Player))
				{
					PreferredSpawns.Add(TestSpawn);
				}
				else
				{
					FallbackSpawns.Add(TestSpawn);
				}
			}
		}
	}

	
	if (BestStart == NULL)
	{
		if (PreferredSpawns.Num() > 0)
		{
			BestStart = PreferredSpawns[FMath::RandHelper(PreferredSpawns.Num())];
		}
		else if (FallbackSpawns.Num() > 0)
		{
			BestStart = FallbackSpawns[FMath::RandHelper(FallbackSpawns.Num())];
		}
	}

	return BestStart ? BestStart : Super::ChoosePlayerStart_Implementation(Player);
}

bool AAwmGameMode::IsSpawnpointAllowed(APlayerStart* SpawnPoint, AController* Player) const
{
	AAwmTeamStart* AwmSpawnPoint = Cast<AAwmTeamStart>(SpawnPoint);
	if (AwmSpawnPoint)
	{
		AAwmAIController* AIController = Cast<AAwmAIController>(Player);
		if (AwmSpawnPoint->bNotForBots && AIController)
		{
			return false;
		}

		if (AwmSpawnPoint->bNotForPlayers && AIController == NULL)
		{
			return false;
		}
		return true;
	}

	return false;
}

bool AAwmGameMode::IsSpawnpointPreferred(APlayerStart* SpawnPoint, AController* Player) const
{
	ACharacter* MyPawn = Cast<ACharacter>((*DefaultPawnClass)->GetDefaultObject<ACharacter>());	
	AAwmAIController* AIController = Cast<AAwmAIController>(Player);
	if( AIController != nullptr )
	{
		MyPawn = Cast<ACharacter>(BotPawnClass->GetDefaultObject<ACharacter>());
	}
	
	if (MyPawn)
	{
		const FVector SpawnLocation = SpawnPoint->GetActorLocation();
		for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
		{
			ACharacter* OtherPawn = Cast<ACharacter>(*It);
			if (OtherPawn && OtherPawn != MyPawn)
			{
				const float CombinedHeight = (MyPawn->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + OtherPawn->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()) * 2.0f;
				const float CombinedRadius = MyPawn->GetCapsuleComponent()->GetScaledCapsuleRadius() + OtherPawn->GetCapsuleComponent()->GetScaledCapsuleRadius();
				const FVector OtherLocation = OtherPawn->GetActorLocation();

				// check if player start overlaps this pawn
				if (FMath::Abs(SpawnLocation.Z - OtherLocation.Z) < CombinedHeight && (SpawnLocation - OtherLocation).Size2D() < CombinedRadius)
				{
					return false;
				}
			}
		}
	}
	else
	{
		return false;
	}
	
	return true;
}

void AAwmGameMode::CreateBotControllers()
{
	UWorld* World = GetWorld();
	int32 ExistingBots = 0;
	for (FConstControllerIterator It = World->GetControllerIterator(); It; ++It)
	{		
		AAwmAIController* AIC = Cast<AAwmAIController>(*It);
		if (AIC)
		{
			++ExistingBots;
		}
	}

	// Create any necessary AIControllers.  Hold off on Pawn creation until pawns are actually necessary or need recreating.	
	int32 BotNum = ExistingBots;
	for (int32 i = 0; i < MaxBots - ExistingBots; ++i)
	{
		CreateBot(BotNum + i);
	}
}

AAwmAIController* AAwmGameMode::CreateBot(int32 BotNum)
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = nullptr;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnInfo.OverrideLevel = nullptr;

	UWorld* World = GetWorld();
	AAwmAIController* AIC = World->SpawnActor<AAwmAIController>(SpawnInfo);
	InitBot(AIC, BotNum);

	return AIC;
}

void AAwmGameMode::StartBots()
{
	// checking number of existing human player.
	int32 NumPlayers = 0;
	int32 NumBots = 0;
	UWorld* World = GetWorld();
	for (FConstControllerIterator It = World->GetControllerIterator(); It; ++It)
	{		
		AAwmAIController* AIC = Cast<AAwmAIController>(*It);
		if (AIC)
		{
			RestartPlayer(AIC);
		}
	}	
}

void AAwmGameMode::InitBot(AAwmAIController* AIController, int32 BotNum)
{	
	if (AIController)
	{
		if (AIController->PlayerState)
		{
			FString BotName = FString::Printf(TEXT("Bot %d"), BotNum);
			AIController->PlayerState->PlayerName = BotName;
		}		
	}
}
