// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#pragma once
#include "AwmPersistentUser.generated.h"

UCLASS()
class UAwmPersistentUser : public USaveGame
{
	GENERATED_UCLASS_BODY()

public:
	/** Loads user persistence data if it exists, creates an empty record otherwise. */
	static UAwmPersistentUser* LoadPersistentUser(FString SlotName, const int32 UserIndex);

	/** Saves data if anything has changed. */
	void SaveIfDirty();

	/** needed because we can recreate the subsystem that stores it */
	void TellInputAboutKeybindings();

	int32 GetUserIndex() const;

	/** Is the y axis inverted? */
	FORCEINLINE bool GetInvertedYAxis() const
	{
		return bInvertedYAxis;
	}

	/** Setter for inverted y axis */
	void SetInvertedYAxis(bool bInvert);

	/** Getter for the aim sensitivity */
	FORCEINLINE float GetAimSensitivity() const
	{
		return AimSensitivity;
	}

	void SetAimSensitivity(float InSensitivity);

	FORCEINLINE FString GetName() const
	{
		return SlotName;
	}

protected:
	void SetToDefaults();

	/** Checks if the Mouse Sensitivity user setting is different from current */
	bool IsAimSensitivityDirty() const;

	/** Checks if the Inverted Mouse user setting is different from current */
	bool IsInvertedYAxisDirty() const;

	/** Triggers a save of this data. */
	void SavePersistentUser();

	/** Holds the mouse sensitivity */
	UPROPERTY()
	float AimSensitivity;

	/** Is the y axis inverted or not? */
	UPROPERTY()
	bool bInvertedYAxis;

private:
	/** Internal.  True if data is changed but hasn't been saved. */
	bool bIsDirty;

	/** The string identifier used to save/load this persistent user. */
	FString SlotName;
	int32 UserIndex;


};