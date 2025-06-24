// Copyright Epic Games, Inc. All Rights Reserved.

#include "OneButtonLevelGeneration.h"

#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Editor/MapPresetAssetTypeActions.h"

#define LOCTEXT_NAMESPACE "FOneButtonLevelGenerationModule"

void FOneButtonLevelGenerationModule::StartupModule()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	// 생성한 에셋 타입 액션을 등록합니다.
	TSharedRef<IAssetTypeActions> Action = MakeShared<FMapPresetAssetTypeActions>();
	AssetTools.RegisterAssetTypeActions(Action);

	// 등록된 액션을 나중에 ShutdownModule에서 해제할 수 있도록 저장합니다.
	RegisteredAssetTypeActions.Add(Action);
}

void FOneButtonLevelGenerationModule::ShutdownModule()
{
	// 모듈이 언로드될 때 등록된 액션을 해제합니다.
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		for (auto Action : RegisteredAssetTypeActions)
		{
			AssetTools.UnregisterAssetTypeActions(Action);
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FOneButtonLevelGenerationModule, OneButtonLevelGeneration)
