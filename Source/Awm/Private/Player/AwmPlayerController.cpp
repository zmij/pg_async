// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#include "Awm.h"

AAwmPlayerController::AAwmPlayerController(const class FObjectInitializer& PCIP)
	: Super(PCIP)
{
	PrimaryActorTick.bCanEverTick = true;
	bShowMouseCursor = true;
	bIgnoreInput = false;

	//PlayerCameraManagerClass = AAwmPlayerCameraManager::StaticClass();
	CheatClass = UAwmCheatManager::StaticClass();
	bAllowGameActions = true;
}


//////////////////////////////////////////////////////////////////////////
// Initialization

void AAwmPlayerController::InitInputSystem()
{
	Super::InitInputSystem();

	UAwmPersistentUser* PersistentUser = GetPersistentUser();
	if (PersistentUser)
	{
		PersistentUser->TellInputAboutKeybindings();
	}
}

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

void AAwmPlayerController::PreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel)
{
	Super::PreClientTravel(PendingURL, TravelType, bIsSeamlessTravel);

	if (GetWorld() != NULL)
	{
		AAwmHUD* MyAwmHUD = Cast<AAwmHUD>(GetHUD());
		if (MyAwmHUD)
		{
			MyAwmHUD->ShowLoadingScreen(PendingURL);
		}
	}
}

void AAwmPlayerController::UnFreeze()
{
    AGameMode* GameMode = GetWorld()->GetAuthGameMode();
    if (GameMode == NULL) return;
    
    AAwmGameMode* AwmGameMode = Cast<AAwmGameMode>(GameMode);
    if (AwmGameMode == NULL || (AwmGameMode != NULL && AwmGameMode->bRespawn))
    {
        ServerRestartPlayer();
    }
}

void AAwmPlayerController::FailedToSpawnPawn()
{
	if (StateName == NAME_Inactive)
	{
		BeginInactiveState();
	}

	Super::FailedToSpawnPawn();
}


//////////////////////////////////////////////////////////////////////////
// Game Control

void AAwmPlayerController::ClientGameStarted_Implementation()
{
	bAllowGameActions = true;

	// Enable controls mode now the game has started
	SetIgnoreMoveInput(false);
	AAwmHUD* MyAwmHUD = Cast<AAwmHUD>(GetHUD());
	if (MyAwmHUD)
	{
		MyAwmHUD->NotifyClientGameStarted();
	}
}

void AAwmPlayerController::ClientSetSpectatorCamera_Implementation(FVector CameraLocation, FRotator CameraRotation)
{
	SetInitialLocationAndRotation(CameraLocation, CameraRotation);
	SetViewTarget(this);
}

void AAwmPlayerController::HandleReturnToMainMenu()
{
	AAwmHUD* MyAwmHUD = Cast<AAwmHUD>(GetHUD());
	if (MyAwmHUD)
	{
		MyAwmHUD->HandleReturnToMainMenu();
	}
}


//////////////////////////////////////////////////////////////////////////
// Player Input

void AAwmPlayerController::SetIgnoreInput(bool bIgnore)
{
	bIgnoreInput = bIgnore;
}


//////////////////////////////////////////////////////////////////////////
// Helpers

UAwmPersistentUser* AAwmPlayerController::GetPersistentUser() const
{
	UAwmLocalPlayer* const AwmLocalPlayer = Cast<UAwmLocalPlayer>(Player);
	return AwmLocalPlayer ? AwmLocalPlayer->GetPersistentUser() : nullptr;
}

AAwmSpectatorPawn* AAwmPlayerController::GetAwmSpectatorPawn() const
{
	return Cast<AAwmSpectatorPawn>(GetPawnOrSpectator());
}

AAwmVehicle* AAwmPlayerController::GetAwmVehiclePawn() const
{
	return Cast<AAwmVehicle>(GetPawn());
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


//////////////////////////////////////////////////////////////////////////
// Cheats

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


//////////////////////////////////////////////////////////////////////////
// Replication

void AAwmPlayerController::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AAwmPlayerController, bInfiniteAmmo, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AAwmPlayerController, bInfiniteClip, COND_OwnerOnly);
}


//////////////////////////////////////////////////////////////////////////
// Player Input

void AAwmPlayerController::OnTapPressed_Implementation(const FVector2D& ScreenPosition1, float DownTime)
{
	// Pass the tap through to the spectator pawn
	if (GetAwmVehiclePawn() != nullptr) {
		GetAwmVehiclePawn()->OnTapPressed(ScreenPosition1, DownTime);
	}
}

void AAwmPlayerController::OnHoldPressed_Implementation(const FVector2D& ScreenPosition1, float DownTime)
{
	// Pass the hold through to the spectator pawn
	if (GetAwmVehiclePawn() != nullptr) {
		GetAwmVehiclePawn()->OnHoldPressed(ScreenPosition1, DownTime);
	}
}

void AAwmPlayerController::OnHoldReleased_Implementation(const FVector2D& ScreenPosition1, float DownTime)
{
	// Pass the hold through to the spectator pawn
	if (GetAwmVehiclePawn() != nullptr) {
		GetAwmVehiclePawn()->OnHoldReleased(ScreenPosition1, DownTime);
	}
}

void AAwmPlayerController::OnSwipeStarted_Implementation(const FVector2D& AnchorPosition1, float DownTime)
{
	// Pass the swipe through to the spectator pawn
	if (GetAwmVehiclePawn() != nullptr) {
		GetAwmVehiclePawn()->OnSwipeStarted(AnchorPosition1, DownTime);
	}
}

void AAwmPlayerController::OnSwipeUpdate_Implementation(const FVector2D& ScreenPosition1, float DownTime)
{
	// Pass the swipe through to the spectator pawn
	if (GetAwmVehiclePawn() != nullptr) {
		GetAwmVehiclePawn()->OnSwipeUpdate(ScreenPosition1, DownTime);
	}
}

void AAwmPlayerController::OnSwipeReleased_Implementation(const FVector2D& ScreenPosition1, float DownTime)
{
	// Pass the swipe through to the spectator pawn
	if (GetAwmVehiclePawn() != nullptr) {
		GetAwmVehiclePawn()->OnSwipeReleased(ScreenPosition1, DownTime);
	}
}

void AAwmPlayerController::OnSwipeTwoPointsStarted_Implementation(const FVector2D& ScreenPosition1, const FVector2D& ScreenPosition2, float DownTime)
{
	// Pass the double swipe through to the spectator pawn
	if (GetAwmVehiclePawn() != nullptr) {
		GetAwmVehiclePawn()->OnSwipeTwoPointsStarted(ScreenPosition1, ScreenPosition2, DownTime);
	}
}

void AAwmPlayerController::OnSwipeTwoPointsUpdate_Implementation(const FVector2D& ScreenPosition1, const FVector2D& ScreenPosition2, float DownTime)
{
	// Pass the double swipe through to the spectator pawn
	if (GetAwmVehiclePawn() != nullptr) {
		GetAwmVehiclePawn()->OnSwipeTwoPointsUpdate(ScreenPosition1, ScreenPosition2, DownTime);
	}
}

void AAwmPlayerController::OnSwipeTwoPointsReleased_Implementation(const FVector2D& ScreenPosition1, const FVector2D& ScreenPosition2, float DownTime)
{
	// Pass the double swipe through to the spectator pawn
	if (GetAwmVehiclePawn() != nullptr) {
		GetAwmVehiclePawn()->OnSwipeTwoPointsReleased(ScreenPosition1, ScreenPosition2, DownTime);
	}
}

void AAwmPlayerController::OnPinchStarted_Implementation(const FVector2D& AnchorPosition1, const FVector2D& AnchorPosition2, float DownTime)
{
	// Pass the pinch through to the spectator pawn
	if (GetAwmVehiclePawn() != nullptr) {
		GetAwmVehiclePawn()->OnPinchStarted(AnchorPosition1, AnchorPosition2, DownTime);
	}
}

void AAwmPlayerController::OnPinchUpdate_Implementation(const FVector2D& ScreenPosition1, const FVector2D& ScreenPosition2, float DownTime)
{
	// Pass the pinch through to the spectator pawn
	if (GetAwmVehiclePawn() != nullptr) {
		GetAwmVehiclePawn()->OnPinchUpdate(InputHandler, ScreenPosition1, ScreenPosition2, DownTime);
	}
}

void AAwmPlayerController::OnPinchReleased_Implementation(const FVector2D& ScreenPosition1, const FVector2D& ScreenPosition2, float DownTime)
{
	// Pass the pinch through to the spectator pawn
	if (GetAwmVehiclePawn() != nullptr) {
		GetAwmVehiclePawn()->OnPinchReleased(InputHandler, ScreenPosition1, ScreenPosition2, DownTime);
	}
}


//////////////////////////////////////////////////////////////////////////
// Client connect

FString AAwmPlayerController::GetConnectionOptions() const
{
	return ConnectionOptions;
}

void AAwmPlayerController::SetConnectionOptions(const FString& ConnectionOptions)
{
	this->ConnectionOptions = ConnectionOptions;
}

void AAwmPlayerController::ClientReturnToMainMenu_Implementation(const FString& ReturnReason)
{
	UAwmGameInstance* AwmGameInstance = Cast<UAwmGameInstance>(GetWorld()->GetAuthGameMode());
	// Don't notify when ReturnReason is empty
	if (AwmGameInstance != nullptr && ReturnReason.Len() > 0)
	{
		AwmGameInstance->NotifyConnectionError(ReturnReason);
	}

	HandleReturnToMainMenu();
}
