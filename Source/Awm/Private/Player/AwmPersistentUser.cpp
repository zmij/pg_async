// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#include "Awm.h"
#include "Player/AwmPersistentUser.h"

UAwmPersistentUser::UAwmPersistentUser(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetToDefaults();
}

void UAwmPersistentUser::SetToDefaults()
{
	bIsDirty = false;

	bInvertedYAxis = false;
	AimSensitivity = 1.0f;
}

bool UAwmPersistentUser::IsAimSensitivityDirty() const
{
	bool bIsAimSensitivityDirty = false;

	// Fixme: UAwmPersistentUser is not setup to work with multiple worlds.
	// For now, user settings are global to all world instances.
	if (GEngine)
	{
		TArray<APlayerController*> PlayerList;
		GEngine->GetAllLocalPlayerControllers(PlayerList);

		for (auto It = PlayerList.CreateIterator(); It; ++It)
		{
			APlayerController* PC = *It;
			if (!PC || !PC->Player || !PC->PlayerInput)
			{
				continue;
			}

			// Update key bindings for the current user only
			UAwmLocalPlayer* LocalPlayer = Cast<UAwmLocalPlayer>(PC->Player);
			if(!LocalPlayer || LocalPlayer->GetPersistentUser() != this)
			{
				continue;
			}

			// check if the aim sensitivity is off anywhere
			for (int32 Idx = 0; Idx < PC->PlayerInput->AxisMappings.Num(); Idx++)
			{
				FInputAxisKeyMapping &AxisMapping = PC->PlayerInput->AxisMappings[Idx];
				if (AxisMapping.AxisName == "Lookup" || AxisMapping.AxisName == "LookupRate" || AxisMapping.AxisName == "Turn" || AxisMapping.AxisName == "TurnRate")
				{
					if (FMath::Abs(AxisMapping.Scale) != GetAimSensitivity())
					{
						bIsAimSensitivityDirty = true;
						break;
					}
				}
			}
		}
	}

	return bIsAimSensitivityDirty;
}

bool UAwmPersistentUser::IsInvertedYAxisDirty() const
{
	bool bIsInvertedYAxisDirty = false;
	if (GEngine)
	{
		TArray<APlayerController*> PlayerList;
		GEngine->GetAllLocalPlayerControllers(PlayerList);

		for (auto It = PlayerList.CreateIterator(); It; ++It)
		{
			APlayerController* PC = *It;
			if (!PC || !PC->Player || !PC->PlayerInput)
			{
				continue;
			}

			// Update key bindings for the current user only
			UAwmLocalPlayer* LocalPlayer = Cast<UAwmLocalPlayer>(PC->Player);
			if(!LocalPlayer || LocalPlayer->GetPersistentUser() != this)
			{
				continue;
			}

			bIsInvertedYAxisDirty |= PC->PlayerInput->GetInvertAxis("Lookup") != GetInvertedYAxis();
			bIsInvertedYAxisDirty |= PC->PlayerInput->GetInvertAxis("LookupRate") != GetInvertedYAxis();
		}
	}

	return bIsInvertedYAxisDirty;
}

void UAwmPersistentUser::SavePersistentUser()
{
	UGameplayStatics::SaveGameToSlot(this, SlotName, UserIndex);
	bIsDirty = false;
}

UAwmPersistentUser* UAwmPersistentUser::LoadPersistentUser(FString SlotName, const int32 UserIndex)
{
	UAwmPersistentUser* Result = nullptr;
	
	// first set of player signins can happen before the UWorld exists, which means no OSS, which means no user names, which means no slotnames.
	// Persistent users aren't valid in this state.
	if (SlotName.Len() > 0)
	{	
		Result = Cast<UAwmPersistentUser>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
		if (Result == NULL)
		{
			// if failed to load, create a new one
			Result = Cast<UAwmPersistentUser>( UGameplayStatics::CreateSaveGameObject(UAwmPersistentUser::StaticClass()) );
		}
		check(Result != NULL);
	
		Result->SlotName = SlotName;
		Result->UserIndex = UserIndex;
	}

	return Result;
}

void UAwmPersistentUser::SaveIfDirty()
{
	if (bIsDirty || IsInvertedYAxisDirty() || IsAimSensitivityDirty())
	{
		SavePersistentUser();
	}
}

void UAwmPersistentUser::TellInputAboutKeybindings()
{
	TArray<APlayerController*> PlayerList;
	GEngine->GetAllLocalPlayerControllers(PlayerList);

	for (auto It = PlayerList.CreateIterator(); It; ++It)
	{
		APlayerController* PC = *It;
		if (!PC || !PC->Player || !PC->PlayerInput)
		{
			continue;
		}

		// Update key bindings for the current user only
		UAwmLocalPlayer* LocalPlayer = Cast<UAwmLocalPlayer>(PC->Player);
		if(!LocalPlayer || LocalPlayer->GetPersistentUser() != this)
		{
			continue;
		}

		//set the aim sensitivity
		for (int32 Idx = 0; Idx < PC->PlayerInput->AxisMappings.Num(); Idx++)
		{
			FInputAxisKeyMapping &AxisMapping = PC->PlayerInput->AxisMappings[Idx];
			if (AxisMapping.AxisName == "Lookup" || AxisMapping.AxisName == "LookupRate" || AxisMapping.AxisName == "Turn" || AxisMapping.AxisName == "TurnRate")
			{
				AxisMapping.Scale = (AxisMapping.Scale < 0.0f) ? -GetAimSensitivity() : +GetAimSensitivity();
			}
		}
		PC->PlayerInput->ForceRebuildingKeyMaps();

		//invert it, and if does not equal our bool, invert it again
		if (PC->PlayerInput->GetInvertAxis("LookupRate") != GetInvertedYAxis())
		{
			PC->PlayerInput->InvertAxis("LookupRate");
		}

		if (PC->PlayerInput->GetInvertAxis("Lookup") != GetInvertedYAxis())
		{
			PC->PlayerInput->InvertAxis("Lookup");
		}
	}
}

int32 UAwmPersistentUser::GetUserIndex() const
{
	return UserIndex;
}

void UAwmPersistentUser::SetInvertedYAxis(bool bInvert)
{
	bIsDirty |= bInvertedYAxis != bInvert;

	bInvertedYAxis = bInvert;
}

void UAwmPersistentUser::SetAimSensitivity(float InSensitivity)
{
	bIsDirty |= AimSensitivity != InSensitivity;

	AimSensitivity = InSensitivity;
}
