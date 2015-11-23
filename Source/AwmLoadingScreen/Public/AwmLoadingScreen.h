// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#pragma once

#include "ModuleInterface.h"

/** Module interface for this game's loading screens */
class IAwmLoadingScreenModule : public IModuleInterface
{
public:
	/** Kicks off the loading screen for in game loading (not startup) */
	virtual void StartInGameLoadingScreen() = 0;
};
