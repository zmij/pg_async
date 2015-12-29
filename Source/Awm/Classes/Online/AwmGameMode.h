// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#pragma once

#include "GameFramework/GameMode.h"
#include "AwmGameMode.generated.h"

class AAwmPlayerState;



/**
 * 
 */
UCLASS()
class AWM_API AAwmGameMode : public AGameMode
{
	GENERATED_UCLASS_BODY()
	

	//////////////////////////////////////////////////////////////////////////
	// Initialization

	// Begin AGameMode interface
	virtual void PreInitializeComponents() override;
    
    /** Event when play begins for this actor. */
    virtual void BeginPlay() override;
    
    /** Overridable function called whenever this actor is being removed from a level */
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Initialize the game. This is called before actors' PreInitializeComponents. */
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	/** Accept or reject a player attempting to join the server.  Fails login if you set the ErrorMessage to a non-empty string. */
	virtual void PreLogin(const FString& Options, const FString& Address, const TSharedPtr<const FUniqueNetId>& UniqueId, FString& ErrorMessage) override;

	/** If login is successful, returns a new PlayerController to associate with this player. Login fails if ErrorMessage string is set. */
	virtual APlayerController* Login(class UPlayer* NewPlayer, ENetRole InRemoteRole, const FString& Portal, const FString& Options, const TSharedPtr<const FUniqueNetId>& UniqueId, FString& ErrorMessage) override;

	/** starts match warmup */
	virtual void PostLogin(APlayerController* NewPlayer) override;

	/** kicks all players which arent allowed to this server, GameInstance must have enough information for assured answer if player is allowed to be at server or not */
	UFUNCTION(BluePrintCallable, Category = "Awm|GameMode")
	void KickRejectedPlayers();

	/** select best spawn point for player */
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	/** always pick new random spawn */
	virtual bool ShouldSpawnAtStartSpot(AController* Player) override;

	/** returns default pawn class for given controller */
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

	/** prevents friendly fire */
	virtual float ModifyDamage(float Damage, AActor* DamagedActor, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) const;

	/** notify about kills */
	virtual void Killed(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, const UDamageType* DamageType);

	/** can players damage each other? */
	virtual bool CanDealDamage(AAwmPlayerState* DamageInstigator, AAwmPlayerState* DamagedPlayer) const;

	/** always create cheat manager */
	virtual bool AllowCheats(APlayerController* P) override;

	/** update remaining time */
	virtual void DefaultTimer();

	/** called before startmatch */
	virtual void HandleMatchIsWaitingToStart() override;

	/** starts new match */
	virtual void HandleMatchHasStarted() override;
	// End AGameMode interface


	//////////////////////////////////////////////////////////////////////////
	// Bots (AI)

	/** The bot pawn class */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=GameMode)
	TSubclassOf<APawn> BotPawnClass;

	UFUNCTION(exec)
	void SetAllowBots(bool bInAllowBots, int32 InMaxBots = 8);

	/** Creates AIControllers for all bots */
	void CreateBotControllers();

	/** Create a bot */
	AAwmAIController* CreateBot(int32 BotNum);	

public:
    
    bool UseRespawn() const;
    
protected:
    
    /** Respawn */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(DisplayName="Respawn"))
    bool bRespawn;
    
    /** Got all capture areas for victory  */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(DisplayName="Got all CA for victory"))
    bool bGotAllCaptureAreas;
    
    /** If true, RestartGame return players to main menu  */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(DisplayName="One round"))
    bool bOneRound;

    /** How many scores need for victory */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    int32 NeedScoresForVictory;
    
    /** delay between first player login and starting match */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    int32 WarmupTime;

	/** match duration */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 RoundTime;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 TimeBetweenMatches;

	/** score for kill */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 KillScore;

	/** score for death */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 DeathScore;

	/** scale for self instigated damage */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float DamageSelfScale;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 MaxBots;

    UPROPERTY()
    TArray<AAwmAIController*> BotControllers;
    
    UPROPERTY()
    TArray<AAwmCaptureArea*> CaptureAreas;
	
	/** Handle for efficient management of DefaultTimer timer */
	FTimerHandle TimerHandle_DefaultTimer;

	bool bNeedsBotCreation;

	bool bAllowBots;		

	/** spawning all bots for this game */
	void StartBots();

	/** initialization for bot after creation */
	virtual void InitBot(AAwmAIController* AIC, int32 BotNum);

    /** check who won, return -1 if there are no winners */
	virtual void DetermineMatchWinner();
    
    /** Check winner team in tick */
    virtual int32 CheckWinnerTeam();

	/** check if PlayerState is a winner */
	virtual bool IsWinner(AAwmPlayerState* PlayerState) const;

	/** check if player can use spawnpoint */
	virtual bool IsSpawnpointAllowed(APlayerStart* SpawnPoint, AController* Player) const;

	/** check if player should use spawnpoint */
	virtual bool IsSpawnpointPreferred(APlayerStart* SpawnPoint, AController* Player) const;
    
    //////////////////////////////////////////////////////////////////////////
    // Events
    
    virtual void OnCaptureAreaBonus(AAwmCaptureArea* CaptureArea);
    
    //////////////////////////////////////////////////////////////////////////
    // Helpers
    
    /** Get team who has more live players */
    virtual int32 GetMoreLiveTeam() const;
    
    /** Only one team is alive */
    virtual bool OnlyOneTeamIsAlive() const;
    
    /** Get team that has more capturea areas */
    virtual int32 GetTeamWithMaxNumCaptureArea() const;
    
    /** Get num capture area by team */
    virtual int32 GetAmountCaptureAreaByTeam(int32 Team) const;
    
    /** Only one team got all capture areas */
    virtual bool OnlyOneTeamGotAllCaptureAreas() const;
    
    /** Get team with max scores */
    virtual int32 GetTeamWithMaxScores() const;
    
    /** Has scores for victory */
    virtual bool HasScoresForVictory() const;
    
public:	

	/** overrided StartPlay, runs awm logic implemented in BP (AwmGameInstance) */
	void StartPlay() override;

	/** overrided StartMatch, runs awm logic implemented in BP (AwmGameInstance) */
	void StartMatch() override;

	/** overrided EndMatch, runs awm logic implemented in BP (AwmGameInstance) */
	void EndMatch() override;

	/** finish current match and lock players */
	UFUNCTION(exec)
	void FinishMatch();
    
    /** Restart game or return to main menu */
    virtual void RestartGame() override;

	/*Finishes the match and bumps everyone to main menu.*/
	/*Only GameInstance should call this function */
	void RequestFinishAndExitToMainMenu();

	/** get the name of the bots count option used in server travel URL */
	static FString GetBotsCountOptionName();

	/** spawns awm specific pawn using AwmGameInstance */
	APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, class AActor* StartSpot) override;
    
private:
    
    /** Time the last call DefaultTimer */
    float LastCallDefaultTimer;
	
};
