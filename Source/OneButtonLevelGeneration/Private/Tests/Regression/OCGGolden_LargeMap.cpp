// Copyright (c) 2025-2026 Code1133. All rights reserved.
#include "Tests/Regression/OCGGoldenTestCommon.h"

#include "Component/OCGMapGenerateComponent.h"
#include "Misc/AutomationTest.h"

/**
 * 값을 0으로 설정하면 레코드(기록) 모드로 동작합니다.
 * 최초 1회 실행 후, 테스트 출력 로그에 찍힌 CRC 값을 복사하여 여기에 붙여넣으세요.
 */
static constexpr uint32 GExpectedCRC = 0;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOCGGolden_LargeMap,
	"OCG.Regression.LargeMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FOCGGolden_LargeMap::RunTest(const FString& Parameters)
{
	UMapPreset* Preset = TryLoadPreset(this, TEXT("/OneButtonLevelGeneration/Test/MP_Golden_LargeMap.MP_Golden_LargeMap"));
	if (!Preset)
	{
		return true; // skip - asset not created yet
	}

	FOCGGoldenFixture Fixture;
	if (!Fixture.IsValid())
	{
		AddError(TEXT("Failed to spawn AOCGLevelGenerator"));
		return false;
	}

	Fixture.Generator->SetMapPreset(Preset);

	const FOCGBenchmarkEntry Entry = FOCGBenchmarkRunner::Run(FName(TEXT("LargeMap")), [&]
	{
		Fixture.Generator->GetMapGenerateComponent()->GenerateMaps();
	});
	FOCGBenchmarkRunner::AppendToCSV(Entry);

	return CheckHeightMapCRC(this, Preset->HeightMapData, GExpectedCRC, TEXT("LargeMap"));
}
