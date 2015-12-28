// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#include "Awm.h"

AAwmGame_CapturePoint::AAwmGame_CapturePoint(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    bRespawn = false;
    bGotAllCaptureAreas = true;
    NeedScoresForVictory = 0;
}
