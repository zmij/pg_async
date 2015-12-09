// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#include "Awm.h"

IMPLEMENT_PRIMARY_GAME_MODULE( FDefaultGameModuleImpl, Awm, "Awm" );

DEFINE_LOG_CATEGORY(LogAwmAI)
DEFINE_LOG_CATEGORY(LogAwmAll)
DEFINE_LOG_CATEGORY(LogAwmVehicle)













//////////////////////////////////////////////////////
// for PhysX mem allocation hack!!

physx::PxFoundation* GModulePhysXFoundation = nullptr;

void ensurePhysXFoundationSetup() {

#if PLATFORM_MAC
    if (GModulePhysXFoundation == nullptr) {
        auto allocator = new SimpleMemAllocator();
        DummyErrorCallback* ErrorCallback = new DummyErrorCallback();

        GModulePhysXFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, *allocator, *ErrorCallback);
    }
#endif

}

//////////////////////////////////////////////////////