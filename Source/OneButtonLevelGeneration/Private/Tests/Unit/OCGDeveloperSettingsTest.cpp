// Copyright (c) 2025-2026 Code1133. All rights reserved.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#include "OCGDeveloperSettings.h"

/**
 * Unit: 에셋 경로 설정이 ini에서 로드되고 실제 에셋으로 해석되는지 검증합니다.
 *
 * 경로가 소스에서 ini로 옮겨졌으므로 오타나 에셋 이동은 컴파일로 잡히지 않습니다.
 * ini 누락, 잘못된 경로, Fix Up Redirectors 이후의 stale 경로를 여기서 잡습니다.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOCGDeveloperSettingsTest,
	"OCG.Unit.DeveloperSettings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FOCGDeveloperSettingsTest::RunTest(const FString& Parameters)
{
	const UOCGDeveloperSettings* Settings = GetDefault<UOCGDeveloperSettings>();
	if (!Settings)
	{
		AddError(TEXT("UOCGDeveloperSettings CDO is null."));
		return false;
	}

	// 1. ini가 실제로 적용됐는지 확인. 비어 있으면 설정이 로드되지 않은 것이다.
	int32 EmptyCount = 0;
	for (TFieldIterator<FSoftObjectProperty> It(Settings->GetClass()); It; ++It)
	{
		const FSoftObjectPtr& Value = *It->ContainerPtrToValuePtr<FSoftObjectPtr>(Settings);
		if (Value.IsNull())
		{
			AddError(FString::Printf(TEXT("Configured asset path is empty: %s"), *It->GetName()));
			++EmptyCount;
		}
	}
	TestEqual(TEXT("All configured asset paths are populated from ini"), EmptyCount, 0);

	TestFalse(TEXT("LayerInfoSavePath is populated from ini"), Settings->LayerInfoSavePath.Path.IsEmpty());

	// 2. 설정된 경로가 실제 에셋을 가리키는지 확인
	const TArray<FString> Unresolved = Settings->FindUnresolvedAssets();
	for (const FString& Failure : Unresolved)
	{
		AddError(FString::Printf(TEXT("Configured asset does not exist: %s"), *Failure));
	}
	TestEqual(TEXT("All configured assets resolve"), Unresolved.Num(), 0);

	return true;
}
