// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#pragma once

#include "AwmPersistentUser.h"
#include "AwmLocalPlayer.generated.h"

UCLASS(config=Engine, transient)
class UAwmLocalPlayer : public ULocalPlayer
{
	GENERATED_UCLASS_BODY()

public:

	virtual void SetControllerId(int32 NewControllerId) override;

	virtual FString GetNickname() const;

	class UAwmPersistentUser* GetPersistentUser() const;
	
	/** Initializes the PersistentUser */
	void LoadPersistentUser();

private:
	/** Persistent user data stored between sessions (i.e. the user's savegame) */
	UPROPERTY()
	class UAwmPersistentUser* PersistentUser;
};



