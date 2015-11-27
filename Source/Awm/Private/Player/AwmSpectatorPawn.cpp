// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#include "Awm.h"

AAwmSpectatorPawn::AAwmSpectatorPawn(const class FObjectInitializer& PCIP)
	: Super(PCIP)
{

}


//////////////////////////////////////////////////////////////////////////
// Player Input

void AAwmSpectatorPawn::SetupPlayerInputComponent(UInputComponent* InputComponent)
{
	check(InputComponent);
	
	InputComponent->BindAxis("MoveForward", this, &ADefaultPawn::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &ADefaultPawn::MoveRight);
	InputComponent->BindAxis("MoveUp", this, &ADefaultPawn::MoveUp_World);
	InputComponent->BindAxis("Turn", this, &ADefaultPawn::AddControllerYawInput);
	InputComponent->BindAxis("TurnRate", this, &ADefaultPawn::TurnAtRate);
	InputComponent->BindAxis("LookUp", this, &ADefaultPawn::AddControllerPitchInput);
	InputComponent->BindAxis("LookUpRate", this, &AAwmSpectatorPawn::LookUpAtRate);
}

void AAwmSpectatorPawn::LookUpAtRate(float Val)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Val * BaseLookUpRate * GetWorld()->GetDeltaSeconds() * CustomTimeDilation);
}
