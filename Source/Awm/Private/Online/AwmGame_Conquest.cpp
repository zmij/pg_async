// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#include "Awm.h"


AAwmGame_Conquest::AAwmGame_Conquest(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    bRespawn = true;
    bGotAllCaptureAreas = false;
    NeedScoresForVictory = 100;
}
