// Copyright (c) 2025-2026 Code1133. All rights reserved.
#include "AssetTypeActions/MapPresetAssetTypeActions.h"

#include "Data/MapPreset.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateStyle.h"

#define LOCTEXT_NAMESPACE "FOneButtonLevelGenerationModule"

FMapPresetAssetTypeActions::FMapPresetAssetTypeActions(EAssetTypeCategories::Type InCategory)
	: OCGCategory(InCategory)
{
}

FText FMapPresetAssetTypeActions::GetName() const
{
	return LOCTEXT("MapPresetAssetName", "Map Preset");
}

FColor FMapPresetAssetTypeActions::GetTypeColor() const
{
	return FColor::Cyan;
}

UClass* FMapPresetAssetTypeActions::GetSupportedClass() const
{
	return UMapPreset::StaticClass();
}

uint32 FMapPresetAssetTypeActions::GetCategories()
{
	return OCGCategory;
}


TSharedPtr<FSlateStyleSet> FOneButtonLevelGenerationStyle::StyleSet = nullptr;

FName FOneButtonLevelGenerationStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("OneButtonLevelGenerationStyle"));
	return StyleSetName;
}

void FOneButtonLevelGenerationStyle::Initialize()
{
	if (StyleSet.IsValid())
	{
		return;
	}

	StyleSet = MakeShared<FSlateStyleSet>(GetStyleSetName());

	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("OneButtonLevelGeneration"));
	if (!ensureAlwaysMsgf(Plugin.IsValid(), TEXT("OCGStyle: plugin 'OneButtonLevelGeneration' not found in IPluginManager")))
	{
		StyleSet.Reset();
		return;
	}
	const FString ResourcesDir = Plugin->GetBaseDir() / TEXT("Resources");
	StyleSet->SetContentRoot(ResourcesDir);

	// Content Browser 리스트 뷰 아이콘 (16x16)
	StyleSet->Set(
		"ClassIcon.MapPreset",
		new FSlateImageBrush(ResourcesDir / TEXT("MapPreset128.png"), FVector2D(16.0f, 16.0f))
	);

	// Content Browser 타일 뷰 썸네일 (64x64)
	StyleSet->Set(
		"ClassThumbnail.MapPreset",
		new FSlateImageBrush(ResourcesDir / TEXT("MapPreset128.png"), FVector2D(64.0f, 64.0f))
	);

	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet);
}

void FOneButtonLevelGenerationStyle::Shutdown()
{
	if (!StyleSet.IsValid())
	{
		return;
	}

	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet);
	StyleSet.Reset();
}

#undef LOCTEXT_NAMESPACE
