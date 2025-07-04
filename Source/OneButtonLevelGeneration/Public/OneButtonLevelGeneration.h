// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FOneButtonLevelGenerationModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TArray<TSharedRef<class IAssetTypeActions>> RegisteredAssetTypeActions;
};
