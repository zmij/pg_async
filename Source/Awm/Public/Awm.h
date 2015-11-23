// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#pragma once

#include "Engine.h"

#include "Net/UnrealNetwork.h"

#include "ParticleDefinitions.h"
#include "Particles/ParticleSystemComponent.h"

#include "AES.h"
#include "Base64.h"

#include "AwmClasses.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAwmAI, Display, All);
DECLARE_LOG_CATEGORY_EXTERN(LogAwmAll, Display, All);
DECLARE_LOG_CATEGORY_EXTERN(LogAwmVehicle, Display, All);

/** when you modify this, please note that this information can be saved with instances
* also DefaultEngine.ini [/Script/Engine.CollisionProfile] should match with this list **/
#define COLLISION_WEAPON		ECC_GameTraceChannel1
#define COLLISION_PROJECTILE	ECC_GameTraceChannel2
