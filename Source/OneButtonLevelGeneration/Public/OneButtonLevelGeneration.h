// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FOneButtonLevelGenerationModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	void RegisterMenus();
	TSharedRef<SDockTab> OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs);
	void OnPluginButtonClicked();

	static inline const FName OCGWindowTabName = TEXT("OCGWindowTab");

private:
	TArray<TSharedRef<class IAssetTypeActions>> RegisteredAssetTypeActions;
};
