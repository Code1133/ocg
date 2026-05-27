// Copyright (c) 2025-2026 Code1133. All rights reserved.
#include "OneButtonLevelGeneration.h"

#include "AssetToolsModule.h"
#include "AssetTypeActions/MapPresetAssetTypeActions.h"

#define LOCTEXT_NAMESPACE "FOneButtonLevelGenerationModule"

void FOneButtonLevelGenerationModule::StartupModule()
{
	// Slate 스타일 초기화 (ClassIcon.MapPreset / ClassThumbnail.MapPreset)
	FOneButtonLevelGenerationStyle::Initialize();

	// AssetTools에 MapPreset 타입 등록 ("OCG" 커스텀 카테고리)
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	const EAssetTypeCategories::Type OCGCategory = AssetTools.RegisterAdvancedAssetCategory(
		FName(TEXT("OCG")),
		LOCTEXT("OCGCategory", "OCG")
	);

	TSharedPtr<FMapPresetAssetTypeActions> MapPresetActions = MakeShared<FMapPresetAssetTypeActions>(OCGCategory);
	AssetTools.RegisterAssetTypeActions(MapPresetActions.ToSharedRef());
	RegisteredAssetTypeActions.Add(MoveTemp(MapPresetActions));
}

void FOneButtonLevelGenerationModule::ShutdownModule()
{
	// AssetTypeActions 해제
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		for (const TSharedPtr<IAssetTypeActions>& Action : RegisteredAssetTypeActions)
		{
			AssetTools.UnregisterAssetTypeActions(Action.ToSharedRef());
		}
	}
	RegisteredAssetTypeActions.Empty();

	// Slate 스타일 해제
	FOneButtonLevelGenerationStyle::Shutdown();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FOneButtonLevelGenerationModule, OneButtonLevelGeneration)
