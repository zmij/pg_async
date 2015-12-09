// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#pragma once

#include "Engine.h"

#include "PxPhysicsAPI.h"
#include "PxAllocatorCallback.h"
#include "PxErrorCallback.h"

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

#define MAX_PLAYER_NAME_LENGTH 16













//////////////////////////////////////////////////////
// for PhysX mem allocation hack!!

void ensurePhysXFoundationSetup();

class SimpleMemAllocator : public physx::PxAllocatorCallback {
    
public:
    
    SimpleMemAllocator() { }
    
    virtual ~SimpleMemAllocator() { }
    
    virtual void *allocate(size_t size, const char *typeName, const char *filename, int line) override {
        void *ptr = FMemory::Malloc(size, 16);
        return ptr;
    }
    
    virtual void deallocate(void *ptr) override {
        FMemory::Free(ptr);
    }
};

class DummyErrorCallback : public physx::PxErrorCallback {

public:

    virtual void reportError(physx::PxErrorCode::Enum e, const char *message, const char *file, int line) override {}

};

//////////////////////////////////////////////////////