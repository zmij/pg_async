// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#include "Awm.h"

AAwmTankVehicle::AAwmTankVehicle(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UAwmTankMovementComponent>(VehicleMovementComponentName))
{
	VehicleType = EVehicleMovement::Tank;
}