// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#include "Awm.h"

FString UAwmGameInstance::EncodeSalt(FString UserID, FString Token)
{
	FString SignatureString = UserID + Token;

	return FMD5::HashAnsiString(*SignatureString);
}

UClass* UAwmGameInstance::GetDefaultClassFor_Implementation(AController* Controller)
{
	return nullptr;
}

EClientAuthority::Type UAwmGameInstance::CheckPlayerAuthority_Implementation(const FString& Options)
{
	// Default implementation accepts everyone
	return EClientAuthority::Accept;
}