// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#pragma once

#include "AwmGameInstance.generated.h"


UCLASS()
class UAwmGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	/** Calculate the salt using Base64 and SHA256 algorythms */
	UFUNCTION(BlueprintCallable, Category = "Awm|Utilities")
	FString EncodeSalt(FString UserID, FString Token);

	/** [server] Called when map loaded */
	UFUNCTION(BlueprintImplementableEvent, Category = "Awm|Game Instance", meta = (DisplayName = "Notify Start Play"))
	void NotifyStartPlay();

	/** [server] Called when match starts */
	UFUNCTION(BlueprintImplementableEvent, Category = "Awm|Game Instance", meta = (DisplayName = "Notify Start Match"))
	void NotifyStartMatch();

	/** [server] Called when match ends */
	UFUNCTION(BlueprintImplementableEvent, Category = "Awm|Game Instance", meta = (DisplayName = "Notify End Match"))
	void NotifyEndMatch();

	/** returns default class for controller using information about tanks, returning nullptr means usage DefaultPawnClass set in GameMode */
	UFUNCTION(BlueprintNativeEvent, Category = "Awm|Game Instance")
	UClass* GetDefaultClassFor(AController* Controller);

	/** [server] checks if player shall be accepted to this server or not. DontKnow means GameInstance need more information for assured answer */
	UFUNCTION(BlueprintNativeEvent, Category = "Awm|Game Instance")
	EClientAuthority::Type CheckPlayerAuthority(const FString& Options);
};
