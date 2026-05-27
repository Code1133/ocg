// Copyright (c) 2025-2026 Code1133. All rights reserved.
#include "OneButtonLevelGeneration.h"

#define LOCTEXT_NAMESPACE "FOneButtonLevelGenerationModule"

void FOneButtonLevelGenerationModule::StartupModule()
{
	// UOCGEditorSubsystem handles toolbar and console command registration.
}

void FOneButtonLevelGenerationModule::ShutdownModule()
{
	// UOCGEditorSubsystem handles cleanup.
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FOneButtonLevelGenerationModule, OneButtonLevelGeneration)
