// Copyright (c) 2025-2026 Code1133. All rights reserved.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

#include "Data/MapPreset.h"

/**
 * Unit: 중첩 struct 내부 필드 변경이 UMapPreset의 연동 로직을 발화시키는지 검증합니다.
 * 디테일 패널을 거치지 않는 회귀라 골든 CRC 테스트로는 잡을 수 없어, 체인 이벤트를 직접 구성합니다.
 *
 * @note HeightmapFilePath는 읽기 실패 시 FMessageDialog로 unattended 실행을 막으므로 제외했습니다.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOCGPresetPropertyChain,
	"OCG.Unit.PresetPropertyChain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

namespace
{
	/** 디테일 패널과 같은 모양의 [Member, Sub, (Leaf)] 체인을 만들어 발화시킵니다. */
	void FireChain(UMapPreset* Preset, FProperty* MemberProp, FProperty* SubProp, FProperty* LeafProp)
	{
		FEditPropertyChain Chain;
		Chain.AddTail(MemberProp);
		Chain.AddTail(SubProp);
		if (LeafProp)
		{
			Chain.AddTail(LeafProp);
		}

		FProperty* ActiveProp = LeafProp ? LeafProp : SubProp;
		Chain.SetActivePropertyNode(ActiveProp);
		Chain.SetActiveMemberPropertyNode(MemberProp);

		FPropertyChangedEvent Event(ActiveProp, EPropertyChangeType::ValueSet);
		FPropertyChangedChainEvent ChainEvent(Chain, Event);
		Preset->PostEditChangeChainProperty(ChainEvent);
	}
}

bool FOCGPresetPropertyChain::RunTest(const FString& Parameters)
{
	UMapPreset* Preset = NewObject<UMapPreset>(GetTransientPackage());
	if (!Preset)
	{
		AddError(TEXT("Failed to create a transient UMapPreset."));
		return false;
	}

	FProperty* LandscapeMember = UMapPreset::StaticClass()->FindPropertyByName(
		GET_MEMBER_NAME_CHECKED(UMapPreset, LandscapeSettings));
	FProperty* SmoothingMember = UMapPreset::StaticClass()->FindPropertyByName(
		GET_MEMBER_NAME_CHECKED(UMapPreset, SmoothingSettings));
	FProperty* MapResolutionProp = FOCGLandscapeSettings::StaticStruct()->FindPropertyByName(
		GET_MEMBER_NAME_CHECKED(FOCGLandscapeSettings, MapResolution));
	FProperty* ComponentCountProp = FOCGLandscapeSettings::StaticStruct()->FindPropertyByName(
		GET_MEMBER_NAME_CHECKED(FOCGLandscapeSettings, Landscape_ComponentCount));
	FProperty* SmoothHeightProp = FOCGSmoothingSettings::StaticStruct()->FindPropertyByName(
		GET_MEMBER_NAME_CHECKED(FOCGSmoothingSettings, bSmoothHeight));
	FProperty* IntPointXProp = TBaseStructure<FIntPoint>::Get()->FindPropertyByName(TEXT("X"));

	if (!LandscapeMember || !SmoothingMember || !MapResolutionProp || !ComponentCountProp
		|| !SmoothHeightProp || !IntPointXProp)
	{
		AddError(TEXT("Failed to resolve one or more FProperty handles — property was renamed or removed."));
		return false;
	}

	// 이 테스트가 실제로 3단 중첩 상황을 재현하는지 먼저 확인합니다.
	// 꼬리가 MapResolution이 아니라 X여야 회귀 조건이 성립합니다.
	{
		const FPropertyChangedEvent TailEvent(IntPointXProp, EPropertyChangeType::ValueSet);
		TestEqual(
			TEXT("Chain tail must be X, not MapResolution"),
			TailEvent.GetPropertyName(), FName(TEXT("X")));
	}

	// 1. MapResolution 변경 -> Landscape_ComponentCount 역산
	// ComponentSize = QuadsPerSection(63) * SectionsPerComponent(1) = 63
	// (2017 - 1) / 63 = 32
	Preset->LandscapeSettings.Landscape_QuadsPerSection = ELandscapeQuadsPerSection::Q63;
	Preset->LandscapeSettings.Landscape_SectionsPerComponent = 1;
	Preset->LandscapeSettings.Landscape_ComponentCount = FIntPoint(1, 1);
	Preset->LandscapeSettings.MapResolution = FIntPoint(2017, 2017);

	FireChain(Preset, LandscapeMember, MapResolutionProp, IntPointXProp);

	TestEqual(TEXT("ComponentCount is recomputed when MapResolution changes"),
		Preset->LandscapeSettings.Landscape_ComponentCount, FIntPoint(32, 32));

	// 2. Landscape_ComponentCount 변경 -> MapResolution 역산
	// 63 * 8 + 1 = 505
	Preset->LandscapeSettings.Landscape_ComponentCount = FIntPoint(8, 8);

	FireChain(Preset, LandscapeMember, ComponentCountProp, IntPointXProp);

	TestEqual(TEXT("MapResolution is recomputed when ComponentCount changes"),
		Preset->LandscapeSettings.MapResolution, FIntPoint(505, 505));

	// 3. LandscapeScale이 함께 갱신되는지 확인
	TestEqual(TEXT("LandscapeScale equals LandscapeSize * 1000 / MapResolution.X"),
		Preset->LandscapeSettings.LandscapeScale,
		Preset->LandscapeSettings.LandscapeSize * 1000.0f / Preset->LandscapeSettings.MapResolution.X);

	// 4. SmoothingSettings.bSmoothHeight 해제 -> 하위 두 옵션 동반 해제 (2단 중첩)
	Preset->SmoothingSettings.bSmoothHeight = false;
	Preset->SmoothingSettings.bSmoothBySlope = true;
	Preset->SmoothingSettings.bSmoothByMediumHeight = true;

	FireChain(Preset, SmoothingMember, SmoothHeightProp, nullptr);

	TestFalse(TEXT("bSmoothBySlope is cleared when bSmoothHeight is disabled"),
		Preset->SmoothingSettings.bSmoothBySlope);
	TestFalse(TEXT("bSmoothByMediumHeight is cleared when bSmoothHeight is disabled"),
		Preset->SmoothingSettings.bSmoothByMediumHeight);

	return true;
}
