// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#pragma once

#include "AwmCheatManager.generated.h"

UCLASS(Within=AwmPlayerController)
class UAwmCheatManager : public UCheatManager
{
	GENERATED_UCLASS_BODY()

	UFUNCTION(exec)
	void ToggleInfiniteAmmo();

	UFUNCTION(exec)
	void ToggleInfiniteClip();

	UFUNCTION(exec)
	void ToggleMatchTimer();

	UFUNCTION(exec)
	void ForceMatchStart();

	UFUNCTION(exec)
	void ChangeTeam(int32 NewTeamNumber);

	UFUNCTION(exec)
	void Cheat(const FString& Msg);

	UFUNCTION(exec)
	void SpawnBot();
};
