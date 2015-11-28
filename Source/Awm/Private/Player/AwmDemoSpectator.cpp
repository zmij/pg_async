// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#include "Awm.h"

AAwmDemoSpectator::AAwmDemoSpectator(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bShowMouseCursor = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
	bShouldPerformFullTickWhenPaused = true;
}

void AAwmDemoSpectator::SetupInputComponent()
{
	Super::SetupInputComponent();

	// UI input
	InputComponent->BindAction( "NextWeapon", IE_Pressed, this, &AAwmDemoSpectator::OnIncreasePlaybackSpeed );
	InputComponent->BindAction( "PrevWeapon", IE_Pressed, this, &AAwmDemoSpectator::OnDecreasePlaybackSpeed );
}

void AAwmDemoSpectator::SetPlayer( UPlayer* InPlayer )
{
	Super::SetPlayer( InPlayer );

	FActorSpawnParameters SpawnInfo;

	SpawnInfo.Owner				= this;
	SpawnInfo.Instigator		= Instigator;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	PlaybackSpeed = 2;
}

static float PlaybackSpeedLUT[5] = { 0.1f, 0.5f, 1.0f, 2.0f, 4.0f };

void AAwmDemoSpectator::OnIncreasePlaybackSpeed()
{
	PlaybackSpeed = FMath::Clamp( PlaybackSpeed + 1, 0, 4 );

	GetWorldSettings()->DemoPlayTimeDilation = PlaybackSpeedLUT[ PlaybackSpeed ];
}

void AAwmDemoSpectator::OnDecreasePlaybackSpeed()
{
	PlaybackSpeed = FMath::Clamp( PlaybackSpeed - 1, 0, 4 );

	GetWorldSettings()->DemoPlayTimeDilation = PlaybackSpeedLUT[ PlaybackSpeed ];
}
