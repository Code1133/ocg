// Copyright (c) 2025-2026 Code1133. All rights reserved.

#include "CoreMinimal.h"
#include "Editor.h"
#include "Misc/AutomationTest.h"

#include "Data/MapPreset.h"
#include "Data/OCGWorldDataContainer.h"
#include "Subsystems/OCGDataGenerationSubsystem.h"
#include "Tests/Regression/OCGGoldenTestCommon.h"

/**
 * Unit: 세로로 긴 맵에서 침식이 아래쪽 행까지 적용되는지 검증합니다.
 *
 * 침식 브러시는 선형 인덱스를 행/열로 되푸는데, 분모를 MapResolution.Y로 쓰면
 * 브러시 중심 행이 X-1을 넘지 못해 맵 아래쪽이 통째로 침식되지 않습니다.
 * 정사각형 맵은 X == Y라 두 식의 결과가 같아 골든 CRC 5종이 이를 잡지 못합니다.
 *
 * 골든 프리셋을 복제해 세로만 늘립니다. 인메모리로 프리셋을 새로 만들면
 * LandscapeScale이 재계산되지 않아 지형이 해수면 아래로 깔리고, 물방울이
 * 첫 스텝에서 break 하여 침식이 아예 일어나지 않습니다.
 *
 * 기준 CRC 대신 "침식이 맵 아래쪽 행도 바꾼다"는 불변식을 씁니다.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOCGErosionNonSquare,
	"OCG.Unit.ErosionNonSquare",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

namespace
{
	/** 골든 프리셋의 트랜지언트 사본. 원본 에셋은 절대 수정하지 않습니다. */
	UMapPreset* MakeTallCopy(const UMapPreset* Source, bool bErosion)
	{
		UMapPreset* Preset = DuplicateObject<UMapPreset>(Source, GetTransientPackage());

		// 가로는 그대로 두고 세로만 늘려 비정사각형으로 만듭니다.
		Preset->LandscapeSettings.MapResolution.Y = Preset->LandscapeSettings.MapResolution.X * 2 - 1;
		Preset->ErosionSettings.bErosion = bErosion;
		return Preset;
	}
}

bool FOCGErosionNonSquare::RunTest(const FString& Parameters)
{
	UMapPreset* Golden = TryLoadPreset(this, TEXT("/OneButtonLevelGeneration/Test/MP_Golden_Default.MP_Golden_Default"));
	if (!Golden)
	{
		return false;
	}

	UOCGDataGenerationSubsystem* DataGen = GEditor->GetEditorSubsystem<UOCGDataGenerationSubsystem>();
	if (!DataGen)
	{
		AddError(TEXT("UOCGDataGenerationSubsystem not available"));
		return false;
	}

	UMapPreset* ErodedPreset = MakeTallCopy(Golden, true);
	const FIntPoint Resolution = ErodedPreset->LandscapeSettings.MapResolution;
	AddInfo(FString::Printf(TEXT("Testing a %dx%d map."), Resolution.X, Resolution.Y));

	DataGen->GenerateData(ErodedPreset);
	const TArray<uint16> Eroded = DataGen->GetDataContainer().HeightMapData;

	DataGen->GenerateData(MakeTallCopy(Golden, false));
	const TArray<uint16> Untouched = DataGen->GetDataContainer().HeightMapData;

	if (!TestEqual(TEXT("Heightmap size matches the requested resolution"), Eroded.Num(), Resolution.X * Resolution.Y))
	{
		return false;
	}
	if (!TestEqual(TEXT("Both runs produce the same heightmap size"), Untouched.Num(), Eroded.Num()))
	{
		return false;
	}

	// 침식 경로는 전 픽셀을 float로 왕복시키므로 ±1 LSB 양자화 노이즈가 깔립니다.
	// 실제 침식만 세려면 그 위 임계값을 씁니다.
	constexpr int32 QuantizationNoise = 2;

	auto CountErodedPixels = [&](int32 RowBegin, int32 RowEnd)
	{
		int32 Count = 0;
		for (int32 Index = RowBegin * Resolution.X; Index < RowEnd * Resolution.X; ++Index)
		{
			if (FMath::Abs(static_cast<int32>(Eroded[Index]) - static_cast<int32>(Untouched[Index])) > QuantizationNoise)
			{
				++Count;
			}
		}
		return Count;
	};

	const int32 TopRows = Resolution.Y / 4;
	const int32 BottomRows = Resolution.Y * 3 / 4;

	const int32 TopCount = CountErodedPixels(0, TopRows);
	const int32 BottomCount = CountErodedPixels(BottomRows, Resolution.Y);

	AddInfo(FString::Printf(
		TEXT("Eroded pixels: %d in the top quarter (rows 0-%d), %d in the bottom quarter (rows %d-%d)."),
		TopCount, TopRows - 1, BottomCount, BottomRows, Resolution.Y - 1));

	if (!TestTrue(TEXT("Erosion affects the top of the map"), TopCount > 0))
	{
		return false;
	}

	// 버그가 있으면 브러시 중심이 X-1행을 넘지 못해 아래쪽이 0이 됩니다.
	TestTrue(TEXT("Erosion also affects the bottom quarter of a tall map"), BottomCount > 0);

	return true;
}
