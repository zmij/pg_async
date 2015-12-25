// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#pragma once

#include "GameFramework/PlayerController.h"
#include "AwmPlayerController.generated.h"

class AAwmSpectatorPawn;

/**
 * Core class for player controller
 */
UCLASS()
class AAwmPlayerController : public APlayerController
{
	GENERATED_UCLASS_BODY()


	//////////////////////////////////////////////////////////////////////////
	// Initialization

protected:
	// Begin APlayerController interface
	virtual void InitInputSystem() override;
	virtual void SetupInputComponent() override;
	virtual void ProcessPlayerInput(const float DeltaTime, const bool bGamePaused) override;

	/** Show loading screen */
	virtual void PreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel) override;

	/** Restart player */
	virtual void UnFreeze() override;
	// End APlayerController interface

	//Begin AController interface
	virtual void FailedToSpawnPawn() override;
	//End AController interface


	//////////////////////////////////////////////////////////////////////////
	// Game Control

public:
	/** Notify player about started match */
	UFUNCTION(reliable, client)
	void ClientGameStarted();

	/** sets spectator location and rotation */
	UFUNCTION(reliable, client)
	void ClientSetSpectatorCamera(FVector CameraLocation, FRotator CameraRotation);

	/** Informs that player fragged someone */
	UFUNCTION(BlueprintImplementableEvent, Category = "Awm|Player|Notify")
	void OnKill();

	/** Notify local client about deaths */
	UFUNCTION(BlueprintImplementableEvent, Category = "Awm|Player|Notify")
	void OnDeathMessage(class AAwmPlayerState* KillerPlayerState, class AAwmPlayerState* KilledPlayerState, const UDamageType* KillerDamageType);

	/** Notify local client about locking target */
	UFUNCTION(BlueprintImplementableEvent, Category = "Awm|Player|Notify")
	void OnTargetLocked(class AAwmVehicle* NewTarget);

	/** Notify local client about unlocking target */
	UFUNCTION(BlueprintImplementableEvent, Category = "Awm|Player|Notify")
	void OnTargetUnlocked();

	/** Cleans up any resources necessary to return to main menu. Does not modify GameInstance state. */
	virtual void HandleReturnToMainMenu();


	//////////////////////////////////////////////////////////////////////////
	// Player Input

public:
	/** Helper function to toggle input detection */
	UFUNCTION(BlueprintCallable, Category = "Awm|Player")
	void SetIgnoreInput(bool bIgnore);

protected:
	/** If set, input and camera updates will be ignored */
	uint8 bIgnoreInput : 1;

	/** If set, gameplay related actions (movement, weapn usage, etc) are allowed */
	uint8 bAllowGameActions : 1;

	/** Custom input handler */
	UPROPERTY(BlueprintReadOnly)
	class UAwmInput* InputHandler;


	//////////////////////////////////////////////////////////////////////////
	// Touch Input

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Awm|Input|Player|Touch")
	void OnTapPressed(const FVector2D& ScreenPosition1, float DownTime);

	UFUNCTION(BlueprintNativeEvent, Category = "Awm|Input|Player|Touch")
	void OnHoldPressed(const FVector2D& ScreenPosition1, float DownTime);

	UFUNCTION(BlueprintNativeEvent, Category = "Awm|Input|Player|Touch")
	void OnHoldReleased(const FVector2D& ScreenPosition1, float DownTime);

	UFUNCTION(BlueprintNativeEvent, Category = "Awm|Input|Player|Touch")
	void OnSwipeStarted(const FVector2D& AnchorPosition1, float DownTime);

	UFUNCTION(BlueprintNativeEvent, Category = "Awm|Input|Player|Touch")
	void OnSwipeUpdate(const FVector2D& ScreenPosition1, float DownTime);

	UFUNCTION(BlueprintNativeEvent, Category = "Awm|Input|Player|Touch")
	void OnSwipeReleased(const FVector2D& ScreenPosition1, float DownTime);

	UFUNCTION(BlueprintNativeEvent, Category = "Awm|Input|Player|Touch")
	void OnSwipeTwoPointsStarted(const FVector2D& ScreenPosition1, const FVector2D& ScreenPosition2, float DownTime);

	UFUNCTION(BlueprintNativeEvent, Category = "Awm|Input|Player|Touch")
	void OnSwipeTwoPointsUpdate(const FVector2D& ScreenPosition1, const FVector2D& ScreenPosition2, float DownTime);

	UFUNCTION(BlueprintNativeEvent, Category = "Awm|Input|Player|Touch")
	void OnSwipeTwoPointsReleased(const FVector2D& ScreenPosition1, const FVector2D& ScreenPosition2, float DownTime);

	UFUNCTION(BlueprintNativeEvent, Category = "Awm|Input|Player|Touch")
	void OnPinchStarted(const FVector2D& AnchorPosition1, const FVector2D& AnchorPosition2, float DownTime);

	UFUNCTION(BlueprintNativeEvent, Category = "Awm|Input|Player|Touch")
	void OnPinchUpdate(const FVector2D& ScreenPosition1, const FVector2D& ScreenPosition2, float DownTime);

	UFUNCTION(BlueprintNativeEvent, Category = "Awm|Input|Player|Touch")
	void OnPinchReleased(const FVector2D& ScreenPosition1, const FVector2D& ScreenPosition2, float DownTime);


	//////////////////////////////////////////////////////////////////////////
	// Helpers

public:
	/** Returns the persistent user record associated with this player, or null if there is't one. */
	class UAwmPersistentUser* GetPersistentUser() const;

    /** Helper to return cast version of controlled Vehicle */
    AAwmVehicle* GetAwmVehiclePawn() const;
    
private:
	/** Helper to return cast version of Spectator pawn */
	AAwmSpectatorPawn* GetAwmSpectatorPawn() const;

public:
	UFUNCTION(BlueprintCallable, Category = "Awm|Player", meta = (bTraceComplex = true))
	bool GetHitResultAtScreenPositionByChannel(const FVector2D ScreenPosition, ETraceTypeQuery TraceChannel, bool bTraceComplex, FHitResult& HitResult) const;

	UFUNCTION(BlueprintCallable, Category = "Awm|Player", meta = (bTraceComplex = true))
	bool GetHitResultAtScreenPositionForObjects(const FVector2D ScreenPosition, const TArray<TEnumAsByte<EObjectTypeQuery> > & ObjectTypes, bool bTraceComplex, FHitResult& HitResult) const;


	//////////////////////////////////////////////////////////////////////////
	// Cheats

public:
	/** sends cheat message */
	UFUNCTION(reliable, server, WithValidation)
	void ServerCheat(const FString& Msg);

	/** Set infinite ammo cheat */
	void SetInfiniteAmmo(bool bEnable);

	/** Set infinite clip cheat */
	void SetInfiniteClip(bool bEnable);

	/** Set health regen cheat */
	void SetHealthRegen(bool bEnable);

	/** Set god mode cheat */
	UFUNCTION(exec)
	void SetGodMode(bool bEnable);

	/** Get infinite ammo cheat */
	bool HasInfiniteAmmo() const;

	/** Get infinite clip cheat */
	bool HasInfiniteClip() const;

	/** Get health regen cheat */
	bool HasHealthRegen() const;

	/** Get gode mode cheat */
	bool HasGodMode() const;

	/** Check if gameplay related actions (movement, weapon usage, etc) are allowed right now */
	bool IsGameInputAllowed() const;

protected:
	/** Infinite ammo cheat */
	UPROPERTY(Transient, Replicated)
	uint8 bInfiniteAmmo : 1;

	/** Infinite clip cheat */
	UPROPERTY(Transient, Replicated)
	uint8 bInfiniteClip : 1;

	/** Health regen cheat */
	UPROPERTY(Transient, Replicated)
	uint8 bHealthRegen : 1;

	/** God mode cheat */
	UPROPERTY(Transient, Replicated)
	uint8 bGodMode : 1;


	//////////////////////////////////////////////////////////////////////////
	// Client connect

protected:
	/** saved options for player authority check */
	FString ConnectionOptions;

public:
	/** get saved player options */
	UFUNCTION(BlueprintCallable, Category = "Awm|Player")
	FString GetConnectionOptions() const;

	/** get saved player options */
	void SetConnectionOptions(const FString& ConnectionOptions);
};
