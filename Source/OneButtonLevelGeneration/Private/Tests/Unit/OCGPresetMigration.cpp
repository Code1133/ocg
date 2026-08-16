// Copyright (c) 2025-2026 Code1133. All rights reserved.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

#include "Data/MapPreset.h"

/**
 * Unit: MapPreset에 걸린 CoreRedirect가 실제로 존재하는 프로퍼티를 가리키는지 검증합니다.
 *
 * 로더는 리다이렉트를 적용한 뒤 그 이름으로 프로퍼티를 찾고, 없으면 값을 버립니다
 * (Class.cpp:1342-1361). 즉 대상이 없는 리다이렉트는 무해한 게 아니라 데이터를 잃습니다.
 * 저장소 에셋은 대부분 기본값이라 델타 직렬화에서 생략되므로 값 비교로는 드러나지 않습니다.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOCGPresetMigration,
	"OCG.Unit.PresetMigration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FOCGPresetMigration::RunTest(const FString& Parameters)
{
	UClass* PresetClass = UMapPreset::StaticClass();

	int32 RedirectCount = 0;
	int32 BrokenCount = 0;
	for (TFieldIterator<FProperty> It(PresetClass); It; ++It)
	{
		const FName OldName = It->GetFName();
		const FName Redirected = FProperty::FindRedirectedPropertyName(PresetClass, OldName);
		if (Redirected.IsNone())
		{
			continue;
		}
		++RedirectCount;

		if (!PresetClass->FindPropertyByName(Redirected))
		{
			AddError(FString::Printf(
				TEXT("CoreRedirect points at a property that does not exist: %s -> %s (value would be discarded on load)"),
				*OldName.ToString(), *Redirected.ToString()));
			++BrokenCount;
		}
	}

	AddInfo(FString::Printf(TEXT("Found %d redirect(s) on UMapPreset properties."), RedirectCount));
	TestEqual(TEXT("No CoreRedirect points at a missing property"), BrokenCount, 0);

	// 중첩 struct의 각 필드는 UMapPreset에 같은 이름의 레거시 프로퍼티가 있어야 구버전
	// 에셋의 값을 받을 수 있습니다. UHT가 _DEPRECATED 접미사를 떼고 등록하므로
	// 이름이 그대로 일치합니다.
	int32 FieldCount = 0;
	int32 MissingCount = 0;
	for (TFieldIterator<FStructProperty> StructIt(PresetClass); StructIt; ++StructIt)
	{
		const UScriptStruct* Inner = StructIt->Struct;
		if (!Inner || !Inner->GetName().StartsWith(TEXT("OCG")))
		{
			continue;
		}

		for (TFieldIterator<FProperty> FieldIt(Inner); FieldIt; ++FieldIt)
		{
			++FieldCount;
			const FName FieldName = FieldIt->GetFName();
			if (!PresetClass->FindPropertyByName(FieldName))
			{
				AddError(FString::Printf(
					TEXT("No legacy property on UMapPreset for %s.%s — old asset values cannot migrate"),
					*Inner->GetName(), *FieldName.ToString()));
				++MissingCount;
			}
		}
	}

	AddInfo(FString::Printf(TEXT("Checked %d nested field(s) for a legacy counterpart."), FieldCount));
	TestEqual(TEXT("Every nested settings field has a legacy property to migrate from"), MissingCount, 0);

	return true;
}
