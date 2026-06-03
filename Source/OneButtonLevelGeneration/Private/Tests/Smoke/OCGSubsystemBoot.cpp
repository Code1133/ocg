// Copyright (c) 2025-2026 Code1133. All rights reserved.

#include "CoreMinimal.h"
#include "Editor.h"
#include "Misc/AutomationTest.h"

#include "Subsystems/OCGDataGenerationSubsystem.h"
#include "Subsystems/OCGEditorSubsystem.h"
#include "Subsystems/OCGHydrologySubsystem.h"
#include "Subsystems/OCGLandscapeGenSubsystem.h"
#include "Subsystems/OCGPopulationSubsystem.h"

/**
 * Smoke: 5개 EditorSubsystem이 GEditor에서 정상 획득되는지 확인합니다.
 *
 * 이 테스트는 엔진이 모듈을 로드한 뒤 서브시스템 인스턴스가 실제로 살아있는지
 * 가장 빠르게 검증하기 위한 최소 부트 체크입니다.
 * 생성 파이프라인을 실행하지 않으므로 에디터 월드나 에셋이 필요 없습니다.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOCGSubsystemBoot,
	"OCG.Smoke.SubsystemBoot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter
)

bool FOCGSubsystemBoot::RunTest(const FString& Parameters)
{
	if (!GEditor)
	{
		AddError(TEXT("GEditor is null — test must run in editor context."));
		return false;
	}

	bool bAllPresent = true;

	// 1. DataGeneration Subsystem
	if (!GEditor->GetEditorSubsystem<UOCGDataGenerationSubsystem>())
	{
		AddError(TEXT("UOCGDataGenerationSubsystem not found."));
		bAllPresent = false;
	}

	// 2. LandscapeGen Subsystem
	if (!GEditor->GetEditorSubsystem<UOCGLandscapeGenSubsystem>())
	{
		AddError(TEXT("UOCGLandscapeGenSubsystem not found."));
		bAllPresent = false;
	}

	// 3. Hydrology Subsystem
	if (!GEditor->GetEditorSubsystem<UOCGHydrologySubsystem>())
	{
		AddError(TEXT("UOCGHydrologySubsystem not found."));
		bAllPresent = false;
	}

	// 4. Population Subsystem
	if (!GEditor->GetEditorSubsystem<UOCGPopulationSubsystem>())
	{
		AddError(TEXT("UOCGPopulationSubsystem not found."));
		bAllPresent = false;
	}

	// 5. Editor Subsystem
	if (!GEditor->GetEditorSubsystem<UOCGEditorSubsystem>())
	{
		AddError(TEXT("UOCGEditorSubsystem not found."));
		bAllPresent = false;
	}

	return bAllPresent;
}
