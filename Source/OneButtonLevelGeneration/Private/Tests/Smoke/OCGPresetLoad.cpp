// Copyright (c) 2025-2026 Code1133. All rights reserved.

#include "CoreMinimal.h"
#include "Editor.h"
#include "HAL/PlatformTime.h"
#include "Misc/AutomationTest.h"

#include "Data/MapPreset.h"
#include "Subsystems/OCGDataGenerationSubsystem.h"

/**
 * Product: 5개 골든 MapPreset 에셋이 로드되는지, 그리고
 * MP_Golden_NoErosion 생성이 1초 이내에 완료되는지 검증합니다.
 *
 * 에셋이 아직 생성되지 않은 경우 Error 없이 Warning으로 건너뜁니다.
 * 생성 시간 임계값(1초)은 순수 데이터 생성(DataGen) 단계만을 측정합니다.
 * 랜드스케이프/하이드롤로지 적용은 포함되지 않습니다.
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
	/** 에셋을 로드하고 없으면 Warning 후 false를 반환합니다. */
	bool TrySmokeLoadPreset(FAutomationTestBase* Test, const TCHAR* Path, UMapPreset*& OutPreset)
	{
		OutPreset = LoadObject<UMapPreset>(nullptr, Path);
		if (!OutPreset)
		{
			Test->AddWarning(FString::Printf(TEXT("Preset not found, skipping: %s"), Path));
			return false;
		}
		return true;
	}
}

bool FOCGPresetLoad::RunTest(const FString& Parameters)
{
	// 1. 5개 골든 프리셋 로드 확인
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

	// 에셋이 하나도 없으면 환경 미구성이므로 전체 건너뜀 (실패 아님)
	if (LoadedCount == 0)
	{
		AddInfo(TEXT("No golden presets found — skipping PresetLoad smoke test."));
		return true;
	}

	AddInfo(FString::Printf(TEXT("Loaded %d / %llu golden presets."), LoadedCount, UE_ARRAY_COUNT(PresetPaths)));

	// 2. MP_Golden_NoErosion 데이터 생성 시간 측정 (<1s)
	UMapPreset* NoErosionPreset = nullptr;
	if (!TrySmokeLoadPreset(this, TEXT("/OneButtonLevelGeneration/Test/MP_Golden_NoErosion.MP_Golden_NoErosion"), NoErosionPreset))
	{
		// 에셋 없으면 타이밍 체크는 건너뜀
		return true;
	}

	if (!GEditor)
	{
		AddError(TEXT("GEditor is null."));
		return false;
	}

	UOCGDataGenerationSubsystem* DataGen = GEditor->GetEditorSubsystem<UOCGDataGenerationSubsystem>();
	if (!DataGen)
	{
		AddError(TEXT("UOCGDataGenerationSubsystem not found."));
		return false;
	}

	const double StartSec = FPlatformTime::Seconds();
	DataGen->GenerateData(NoErosionPreset);
	const double ElapsedMs = (FPlatformTime::Seconds() - StartSec) * 1000.0;

	AddInfo(FString::Printf(TEXT("MP_Golden_NoErosion DataGen took %.1f ms."), ElapsedMs));

	// 1초(1000ms) 이내 완료 여부 확인
	constexpr double MaxAllowedMs = 1000.0;
	if (ElapsedMs > MaxAllowedMs)
	{
		AddError(FString::Printf(
			TEXT("MP_Golden_NoErosion DataGen exceeded time limit: %.1f ms > %.0f ms"),
			ElapsedMs, MaxAllowedMs
		));
		return false;
	}

	return true;
}
