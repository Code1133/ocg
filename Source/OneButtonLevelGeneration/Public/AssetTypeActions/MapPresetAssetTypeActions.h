// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"
#include "Styling/SlateStyle.h"

/**
 * Content Browser에서 MapPreset 에셋의 카테고리, 색상, 아이콘을 등록.
 * v2: OpenAssetEditor를 오버라이드하지 않음 -> 기본 Details Panel 사용.
 */
class FMapPresetAssetTypeActions : public FAssetTypeActions_Base
{
public:
	explicit FMapPresetAssetTypeActions(EAssetTypeCategories::Type InCategory);

	// ~ FAssetTypeActions_Base
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;
	// ~ FAssetTypeActions_Base

private:
	EAssetTypeCategories::Type OCGCategory;
};

/**
 * Slate 스타일셋: ClassIcon.MapPreset / ClassThumbnail.MapPreset 등록.
 * MapPreset128.png (Resources/)를 아이콘/썸네일로 사용.
 */
class FOneButtonLevelGenerationStyle
{
public:
	static void Initialize();
	static void Shutdown();

	static TSharedPtr<ISlateStyle> Get() { return StyleSet; }
	static FName GetStyleSetName();

private:
	static TSharedPtr<FSlateStyleSet> StyleSet;
};
