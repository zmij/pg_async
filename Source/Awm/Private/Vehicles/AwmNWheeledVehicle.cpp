// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#include "Awm.h"

AAwmNWheeledVehicle::AAwmNWheeledVehicle(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer.SetDefaultSubobjectClass<UAwmNWheeledMovementComponent>(VehicleMovementComponentName))
{
    
}
