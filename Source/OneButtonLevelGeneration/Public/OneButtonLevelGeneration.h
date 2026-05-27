// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "Modules/ModuleManager.h"

/**
 * Main module class for the One Button Level Generation plugin.
 * Editor subsystem registration (toolbar, console commands) is handled by UOCGEditorSubsystem.
 */
class FOneButtonLevelGenerationModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
