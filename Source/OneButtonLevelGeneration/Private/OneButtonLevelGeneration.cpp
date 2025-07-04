// Copyright (c) 2025 Code1133. All rights reserved.

#include "OneButtonLevelGeneration.h"

#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Editor/MapPresetAssetTypeActions.h"

#define LOCTEXT_NAMESPACE "FOneButtonLevelGenerationModule"

void FOneButtonLevelGenerationModule::StartupModule()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	// Register the custom asset type actions for Map Preset
	TSharedRef<IAssetTypeActions> Action = MakeShared<FMapPresetAssetTypeActions>();
	AssetTools.RegisterAssetTypeActions(Action);

	// Cache the registered asset type actions
	RegisteredAssetTypeActions.Add(Action);

	// Initialize the style for the module
	FOneButtonLevelGenerationStyle::Initialize();
}

void FOneButtonLevelGenerationModule::ShutdownModule()
{
	// Unregister the custom asset type actions for Map Preset
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		for (auto Action : RegisteredAssetTypeActions)
		{
			AssetTools.UnregisterAssetTypeActions(Action);
		}
	}

	// Shutdown the style for the module
	FOneButtonLevelGenerationStyle::Shutdown();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FOneButtonLevelGenerationModule, OneButtonLevelGeneration)
