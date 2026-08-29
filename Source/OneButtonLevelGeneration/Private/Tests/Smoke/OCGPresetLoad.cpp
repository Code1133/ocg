// Copyright (c) 2025-2026 Code1133. All rights reserved.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#include "Data/MapPreset.h"

/**
 * Product: 5개 골든 MapPreset 에셋이 로드되는지 검증합니다.
 *
 * 에셋이 없으면 Error를 출력하고 실패시킵니다.
 *
 * [SmokeFilter → ProductFilter 변경 이유]
 * SmokeFilter는 FEngineLoop::PreInitPostStartupScreen 단계에서 실행됩니다.
 * 이 시점에 StaticLoadObject로 UMapPreset을 로드하면 종속 UMaterial의
 * PostLoad(UpdateCachedExpressionData)가 실행되는데,
 * UMaterialExpressionLandscapeLayerBlend::GetReferencedTextures()에서
 * Landscape 모듈이 아직 완전히 초기화되지 않아 null dereference(AV)가 발생합니다.
 * ProductFilter는 Session Frontend 또는 커맨드라인에서 명시적으로 실행되므로
 * 전체 에디터 초기화 이후에 안전하게 동작합니다.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOCGPresetLoad,
	"OCG.Product.PresetLoad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

namespace
{
	/** 에셋을 로드하고 없으면 Error 후 false를 반환합니다. */
	bool TrySmokeLoadPreset(FAutomationTestBase* Test, const TCHAR* Path, UMapPreset*& OutPreset)
	{
		OutPreset = LoadObject<UMapPreset>(nullptr, Path);
		if (!OutPreset)
		{
			Test->AddError(FString::Printf(TEXT("Golden preset asset is missing: %s"), Path));
			return false;
		}
		return true;
	}
}

bool FOCGPresetLoad::RunTest(const FString& Parameters)
{
	// 5개 골든 프리셋 로드 확인
	static const TCHAR* PresetPaths[] = {
		TEXT("/OneButtonLevelGeneration/Test/MP_Golden_Default.MP_Golden_Default"),
		TEXT("/OneButtonLevelGeneration/Test/MP_Golden_NoErosion.MP_Golden_NoErosion"),
		TEXT("/OneButtonLevelGeneration/Test/MP_Golden_LargeMap.MP_Golden_LargeMap"),
		TEXT("/OneButtonLevelGeneration/Test/MP_Golden_NoIsland.MP_Golden_NoIsland"),
		TEXT("/OneButtonLevelGeneration/Test/MP_Golden_WithRiver.MP_Golden_WithRiver"),
	};

	int32 LoadedCount = 0;
	for (const TCHAR* Path : PresetPaths)
	{
		UMapPreset* Preset = nullptr;
		if (TrySmokeLoadPreset(this, Path, Preset))
		{
			++LoadedCount;
		}
	}

	// 골든 에셋은 저장소에 항상 존재하므로, 하나도 없다면 무언가 문제가 있는 것.
	if (LoadedCount == 0)
	{
		AddError(TEXT("No golden presets found. Check that Content/Test assets are present."));
		return false;
	}

	AddInfo(FString::Printf(TEXT("Loaded %d / %llu golden presets."), LoadedCount, UE_ARRAY_COUNT(PresetPaths)));

	return true;
}
