// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#include "Awm.h"

AAwmPlayerController::AAwmPlayerController(const class FObjectInitializer& PCIP)
	: Super(PCIP)
{
	PrimaryActorTick.bCanEverTick = true;
	bShowMouseCursor = true;
	bIgnoreInput = false;

	PlayerCameraManagerClass = AAwmPlayerCameraManager::StaticClass();
	CheatClass = UAwmCheatManager::StaticClass();
	bAllowGameActions = true;
}

//////////////////////////////////////////////////////////////////////////
// Initialization

void AAwmPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputHandler = NewObject<UAwmInput>(this);

	BIND_1P_ACTION(InputHandler, EGameKey::Tap, IE_Pressed, &AAwmPlayerController::OnTapPressed);
	BIND_1P_ACTION(InputHandler, EGameKey::Hold, IE_Pressed, &AAwmPlayerController::OnHoldPressed);
	BIND_1P_ACTION(InputHandler, EGameKey::Hold, IE_Released, &AAwmPlayerController::OnHoldReleased);
	BIND_1P_ACTION(InputHandler, EGameKey::Swipe, IE_Pressed, &AAwmPlayerController::OnSwipeStarted);
	BIND_1P_ACTION(InputHandler, EGameKey::Swipe, IE_Repeat, &AAwmPlayerController::OnSwipeUpdate);
	BIND_1P_ACTION(InputHandler, EGameKey::Swipe, IE_Released, &AAwmPlayerController::OnSwipeReleased);
	BIND_2P_ACTION(InputHandler, EGameKey::SwipeTwoPoints, IE_Pressed, &AAwmPlayerController::OnSwipeTwoPointsStarted);
	BIND_2P_ACTION(InputHandler, EGameKey::SwipeTwoPoints, IE_Repeat, &AAwmPlayerController::OnSwipeTwoPointsUpdate);
	BIND_2P_ACTION(InputHandler, EGameKey::SwipeTwoPoints, IE_Released, &AAwmPlayerController::OnSwipeTwoPointsReleased);
	BIND_2P_ACTION(InputHandler, EGameKey::Pinch, IE_Pressed, &AAwmPlayerController::OnPinchStarted);
	BIND_2P_ACTION(InputHandler, EGameKey::Pinch, IE_Repeat, &AAwmPlayerController::OnPinchUpdate);
	BIND_2P_ACTION(InputHandler, EGameKey::Pinch, IE_Released, &AAwmPlayerController::OnPinchReleased);
}

void AAwmPlayerController::ProcessPlayerInput(const float DeltaTime, const bool bGamePaused)
{
	// Process input with our custom input handler
	if (!bGamePaused && PlayerInput && InputHandler && !bIgnoreInput) {
		InputHandler->UpdateDetection(DeltaTime);
	}

	Super::ProcessPlayerInput(DeltaTime, bGamePaused);
}




void AAwmPlayerController::UnFreeze()
{
	ServerRestartPlayer();
}

void AAwmPlayerController::FailedToSpawnPawn()
{
	if (StateName == NAME_Inactive)
	{
		BeginInactiveState();
	}

	Super::FailedToSpawnPawn();
}




void AAwmPlayerController::InitInputSystem()
{
	Super::InitInputSystem();

	UAwmPersistentUser* PersistentUser = GetPersistentUser();
	if (PersistentUser)
	{
		PersistentUser->TellInputAboutKeybindings();
	}
}


void AAwmPlayerController::PreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel)
{
	Super::PreClientTravel(PendingURL, TravelType, bIsSeamlessTravel);

	if (GetWorld() != NULL)
	{
		// @todo Show loading screen
	}
}


UAwmPersistentUser* AAwmPlayerController::GetPersistentUser() const
{
	UAwmLocalPlayer* const AwmLocalPlayer = Cast<UAwmLocalPlayer>(Player);
	return AwmLocalPlayer ? AwmLocalPlayer->GetPersistentUser() : nullptr;
}




void AAwmPlayerController::ClientGameStarted_Implementation()
{
	bAllowGameActions = true;

	// Enable controls mode now the game has started
	SetIgnoreMoveInput(false);

	// @todo notify HUD (bp)
}

void AAwmPlayerController::HandleReturnToMainMenu()
{
	
}

void AAwmPlayerController::ClientSetSpectatorCamera_Implementation(FVector CameraLocation, FRotator CameraRotation)
{
	SetInitialLocationAndRotation(CameraLocation, CameraRotation);
	SetViewTarget(this);
}

bool AAwmPlayerController::ServerCheat_Validate(const FString& Msg)
{
	return true;
}

void AAwmPlayerController::ServerCheat_Implementation(const FString& Msg)
{
	if (CheatManager)
	{
		ClientMessage(ConsoleCommand(Msg));
	}
}




void AAwmPlayerController::SetInfiniteAmmo(bool bEnable)
{
	bInfiniteAmmo = bEnable;
}

void AAwmPlayerController::SetInfiniteClip(bool bEnable)
{
	bInfiniteClip = bEnable;
}

void AAwmPlayerController::SetHealthRegen(bool bEnable)
{
	bHealthRegen = bEnable;
}

void AAwmPlayerController::SetGodMode(bool bEnable)
{
	bGodMode = bEnable;
}

bool AAwmPlayerController::HasInfiniteAmmo() const
{
	return bInfiniteAmmo;
}

bool AAwmPlayerController::HasInfiniteClip() const
{
	return bInfiniteClip;
}

bool AAwmPlayerController::HasHealthRegen() const
{
	return bHealthRegen;
}

bool AAwmPlayerController::HasGodMode() const
{
	return bGodMode;
}

bool AAwmPlayerController::IsGameInputAllowed() const
{
	return bAllowGameActions && !bCinematicMode;
}


void AAwmPlayerController::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AAwmPlayerController, bInfiniteAmmo, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AAwmPlayerController, bInfiniteClip, COND_OwnerOnly);
}


//////////////////////////////////////////////////////////////////////////
// Helpers

void AAwmPlayerController::SetIgnoreInput(bool bIgnore)
{
	bIgnoreInput = bIgnore;
}

AAwmSpectatorPawn* AAwmPlayerController::GetAwmSpectatorPawn() const
{
	return Cast<AAwmSpectatorPawn>(GetPawnOrSpectator());
}

bool AAwmPlayerController::GetHitResultAtScreenPositionByChannel(const FVector2D ScreenPosition, ETraceTypeQuery TraceChannel, bool bTraceComplex, FHitResult& HitResult) const
{
	return GetHitResultAtScreenPosition(ScreenPosition, TraceChannel, bTraceComplex, HitResult);
}

bool AAwmPlayerController::GetHitResultAtScreenPositionForObjects(const FVector2D ScreenPosition, 
	const TArray<TEnumAsByte<EObjectTypeQuery> > & ObjectTypes, bool bTraceComplex, FHitResult& HitResult) const
{
	return GetHitResultAtScreenPosition(ScreenPosition, ObjectTypes, bTraceComplex, HitResult);
}
