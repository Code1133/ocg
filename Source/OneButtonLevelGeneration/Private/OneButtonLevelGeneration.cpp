// Copyright (c) 2025 Code1133. All rights reserved.

#include "OneButtonLevelGeneration.h"

#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Editor/MapPresetAssetTypeActions.h"
#include "Editor/SOCGWidget.h"

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

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FOneButtonLevelGenerationModule::RegisterMenus));
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(OCGWindowTabName,
		FOnSpawnTab::CreateRaw(this, &FOneButtonLevelGenerationModule::OnSpawnPluginTab))
		.SetDisplayName(FText::FromString("OCG Window"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
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

void FOneButtonLevelGenerationModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);
	// Get 'Windows' section of the main menu
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");

	// Add new section to the menu
	FToolMenuSection& Section = Menu->AddSection("OCG", FText::FromString(TEXT("OCG Tools")));

	// Add a menu entry to the section
	Section.AddMenuEntry("OCGWindowButton",
		FText::FromString(TEXT("OCG Window")),
		FText::FromString(TEXT("Open the OCG Level Generator window")),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FOneButtonLevelGenerationModule::OnPluginButtonClicked))
	);
}

TSharedRef<SDockTab> FOneButtonLevelGenerationModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
	.TabRole(ETabRole::NomadTab)
	[
		SNew(SOCGWidget)
	];
}

void FOneButtonLevelGenerationModule::OnPluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(OCGWindowTabName);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FOneButtonLevelGenerationModule, OneButtonLevelGeneration)
