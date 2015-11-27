// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#include "Awm.h"
#include "Player/AwmLocalPlayer.h"
#include "OnlineSubsystemUtilsClasses.h"
#include "AwmInstance.h"

UAwmLocalPlayer::UAwmLocalPlayer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UAwmPersistentUser* UAwmLocalPlayer::GetPersistentUser() const
{
	// if persistent data isn't loaded yet, load it
	if (PersistentUser == nullptr)
	{
		UAwmLocalPlayer* const MutableThis = const_cast<UAwmLocalPlayer*>(this);
		// casting away constness to enable caching implementation behavior
		MutableThis->LoadPersistentUser();
	}
	return PersistentUser;
}

void UAwmLocalPlayer::LoadPersistentUser()
{
	// if we changed controllerid / user, then we need to load the appropriate persistent user.
	if (PersistentUser != nullptr && ( GetControllerId() != PersistentUser->GetUserIndex() || GetNickname() != PersistentUser->GetName() ) )
	{
		PersistentUser->SaveIfDirty();
		PersistentUser = nullptr;
	}

	if (PersistentUser == NULL)
	{
		// Use the platform id here to be resilient in the face of controller swapping and similar situations.
		FPlatformUserId PlatformId = GetControllerId();

		auto Identity = Online::GetIdentityInterface();
		if (Identity.IsValid() && GetPreferredUniqueNetId().IsValid())
		{
			PlatformId = Identity->GetPlatformUserIdFromUniqueNetId(*GetPreferredUniqueNetId());
		}

		PersistentUser = UAwmPersistentUser::LoadPersistentUser( GetNickname(), PlatformId );
	}
}

void UAwmLocalPlayer::SetControllerId(int32 NewControllerId)
{
	ULocalPlayer::SetControllerId(NewControllerId);

	// if we changed controllerid / user, then we need to load the appropriate persistent user.
	if (PersistentUser != nullptr && ( GetControllerId() != PersistentUser->GetUserIndex() || GetNickname() != PersistentUser->GetName() ) )
	{
		PersistentUser->SaveIfDirty();
		PersistentUser = nullptr;
	}

	if (!PersistentUser)
	{
		LoadPersistentUser();
	}
}

FString UAwmLocalPlayer::GetNickname() const
{
	return Super::GetNickname();
}
