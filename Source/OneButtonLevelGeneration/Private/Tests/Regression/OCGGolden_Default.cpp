// Copyright (c) 2025-2026 Code1133. All rights reserved.
#include "Tests/Regression/OCGGoldenTestCommon.h"

#include "Data/MapPreset.h"
#include "Editor.h"
#include "Misc/AutomationTest.h"
#include "Subsystems/OCGDataGenerationSubsystem.h"
#include "Tests/Benchmark/FOCGBenchmarkRunner.h"

/**
 * 값을 0으로 설정하면 레코드(기록) 모드로 동작합니다.
 * 최초 1회 실행 후, 테스트 출력 로그에 찍힌 CRC 값을 복사하여 여기에 붙여넣으세요.
 */
static constexpr uint32 GExpectedCRC = 0xEFC311B8;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOCGGolden_Default,
	"OCG.Regression.Default",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FOCGGolden_Default::RunTest(const FString& Parameters)
{
	UMapPreset* Preset = TryLoadPreset(this, TEXT("/OneButtonLevelGeneration/Test/MP_Golden_Default.MP_Golden_Default"));
	if (!Preset)
	{
		return true; // skip - asset not created yet
	}

	UOCGDataGenerationSubsystem* DataGen = GEditor->GetEditorSubsystem<UOCGDataGenerationSubsystem>();
	if (!DataGen)
	{
		AddError(TEXT("UOCGDataGenerationSubsystem not available"));
		return false;
	}

	const FOCGBenchmarkEntry Entry = FOCGBenchmarkRunner::Run(FName(TEXT("Default")), [&]
	{
		DataGen->GenerateData(Preset);
	});
	FOCGBenchmarkRunner::AppendToCSV(Entry);

	return CheckHeightMapCRC(this, DataGen->GetDataContainer().HeightMapData, GExpectedCRC, TEXT("Default"));
}
