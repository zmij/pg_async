// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#pragma once

#include "AwmHUD.generated.h"

class APawn;

UCLASS()
class AAwmHUD : public AHUD
{
	GENERATED_UCLASS_BODY()
	

	//////////////////////////////////////////////////////////////////////////
	// Loading screen

	/** Show the loading screen */
	UFUNCTION(BlueprintCallable, Category = "Awm|HUD")
	void LaunchGame(FString MapName);

	/** Show the loading screen */
	UFUNCTION(BlueprintNativeEvent, Category = "Awm|HUD")
	void ShowLoadingScreen(const FString& PendingURL);

	/** Show the loading screen */
	UFUNCTION(BlueprintImplementableEvent, Category = "Awm|HUD")
	void NotifyClientGameStarted();

	/** Show the loading screen and return to default map */
	UFUNCTION(BlueprintCallable, Category = "Awm|HUD")
	void HandleReturnToMainMenu();


	//////////////////////////////////////////////////////////////////////////
	// Battle

	/** Called when our vehicle takes damage */
	UFUNCTION(BlueprintImplementableEvent, Category = "Awm|HUD")
	void NotifyWeaponHit(float DamageTaken, struct FDamageEvent const& DamageEvent, APawn* PawnInstigator);

	/** Called when enemy vehicle takes damage */
	UFUNCTION(BlueprintImplementableEvent, Category = "Awm|HUD")
	void NotifyEnemyHit();

	/** Called when player's weapon is out of ammo */
	UFUNCTION(BlueprintImplementableEvent, Category = "Awm|HUD")
	void NotifyOutOfAmmo();


	//////////////////////////////////////////////////////////////////////////
	// Touch input handler

	/** Input system gives chance to override the input */
	UFUNCTION(BlueprintImplementableEvent, Category = "Awm|HUD")
	void ProcessTouchEvents(const TArray<FFingerTouch>& TouchCache);

};
