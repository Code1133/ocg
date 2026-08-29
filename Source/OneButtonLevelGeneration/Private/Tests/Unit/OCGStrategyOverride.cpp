// Copyright (c) 2025-2026 Code1133. All rights reserved.

#include "CoreMinimal.h"
#include "Editor.h"
#include "Misc/AutomationTest.h"

#include "OCGDeveloperSettings.h"
#include "Strategies/OCGBiomeStrategyBase.h"
#include "Strategies/OCGDefaultErosionStrategy.h"
#include "Strategies/OCGErosionStrategyBase.h"
#include "Strategies/OCGHeightmapStrategyBase.h"
#include "Strategies/OCGHumidityStrategyBase.h"
#include "Strategies/OCGSmoothingStrategyBase.h"
#include "Strategies/OCGTemperatureStrategyBase.h"
#include "Strategies/OCGTerrainModifierStrategyBase.h"
#include "Subsystems/OCGDataGenerationSubsystem.h"
#include "Tests/Unit/OCGTestStrategies.h"

/**
 * Unit: 생성 전략 확장점이 실제로 동작하는지 검증합니다.
 *
 * 이 기능의 값어치는 "프로젝트 설정에 지정한 클래스가 정말 쓰이는가" 하나에 달려 있습니다.
 * 하드코딩으로 되돌아가면 컴파일도 되고 생성 결과도 같아서 눈으로는 구분되지 않으므로,
 * 배선 자체를 단언합니다.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOCGStrategyOverride,
	"OCG.Unit.StrategyOverride",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

namespace
{
	/**
	 * 서브시스템의 private UPROPERTY를 이름으로 읽습니다.
	 *
	 * 테스트만을 위해 제품 헤더에 getter를 늘리지 않으려고 리플렉션을 씁니다.
	 */
	UObject* ReadStrategyProperty(UObject* Owner, const TCHAR* PropertyName)
	{
		const FObjectProperty* Property = FindFProperty<FObjectProperty>(Owner->GetClass(), PropertyName);
		return Property ? Property->GetObjectPropertyValue_InContainer(Owner) : nullptr;
	}
}

bool FOCGStrategyOverride::RunTest(const FString& Parameters)
{
	const UOCGDeveloperSettings* Settings = GetDefault<UOCGDeveloperSettings>();
	if (!Settings)
	{
		AddError(TEXT("UOCGDeveloperSettings CDO is null."));
		return false;
	}

	// 1. ini 기본값이 7단계 모두에 실려 있는가.
	//    비어 있어도 폴백 덕분에 생성은 되지만, 프로젝트 설정 화면이 전부 None으로 보인다.
	//    Pair는 (단계 이름, 설정된 클래스, 그 단계의 베이스 클래스, 서브시스템 프로퍼티 이름).
	struct FStageExpectation
	{
		const TCHAR* StageName;
		UClass* ConfiguredClass;
		UClass* BaseClass;
		const TCHAR* PropertyName;
	};

	const TArray<FStageExpectation> Stages = {
		{TEXT("Heightmap"),        Settings->HeightmapStrategyClass.Get(),        UOCGHeightmapStrategyBase::StaticClass(),        TEXT("HeightmapStrategy")},
		{TEXT("Temperature"),      Settings->TemperatureStrategyClass.Get(),      UOCGTemperatureStrategyBase::StaticClass(),      TEXT("TemperatureStrategy")},
		{TEXT("Humidity"),         Settings->HumidityStrategyClass.Get(),         UOCGHumidityStrategyBase::StaticClass(),         TEXT("HumidityStrategy")},
		{TEXT("Biome"),            Settings->BiomeStrategyClass.Get(),            UOCGBiomeStrategyBase::StaticClass(),            TEXT("BiomeStrategy")},
		{TEXT("TerrainModifier"),  Settings->TerrainModifierStrategyClass.Get(),  UOCGTerrainModifierStrategyBase::StaticClass(),  TEXT("TerrainModifierStrategy")},
		{TEXT("Erosion"),          Settings->ErosionStrategyClass.Get(),          UOCGErosionStrategyBase::StaticClass(),          TEXT("ErosionStrategy")},
		{TEXT("Smoothing"),        Settings->SmoothingStrategyClass.Get(),        UOCGSmoothingStrategyBase::StaticClass(),        TEXT("SmoothingStrategy")},
	};

	for (const FStageExpectation& Stage : Stages)
	{
		if (!Stage.ConfiguredClass)
		{
			AddError(FString::Printf(
				TEXT("%s strategy class is unset. Check DefaultOneButtonLevelGeneration.ini."), Stage.StageName));
			continue;
		}

		if (Stage.ConfiguredClass->HasAnyClassFlags(CLASS_Abstract))
		{
			AddError(FString::Printf(
				TEXT("%s strategy class '%s' is abstract and would always fall back."),
				Stage.StageName, *Stage.ConfiguredClass->GetName()));
		}

		if (!Stage.ConfiguredClass->IsChildOf(Stage.BaseClass))
		{
			AddError(FString::Printf(
				TEXT("%s strategy class '%s' does not derive from %s."),
				Stage.StageName, *Stage.ConfiguredClass->GetName(), *Stage.BaseClass->GetName()));
		}
	}

	// 2. 살아 있는 서브시스템이 실제로 그 클래스를 들고 있는가.
	//    하드코딩으로 되돌아가면 여기서 어긋난다.
	if (!GEditor)
	{
		AddError(TEXT("GEditor is null."));
		return false;
	}

	UOCGDataGenerationSubsystem* DataGen = GEditor->GetEditorSubsystem<UOCGDataGenerationSubsystem>();
	if (!DataGen)
	{
		AddError(TEXT("UOCGDataGenerationSubsystem not available."));
		return false;
	}

	for (const FStageExpectation& Stage : Stages)
	{
		const UObject* Instance = ReadStrategyProperty(DataGen, Stage.PropertyName);
		if (!Instance)
		{
			AddError(FString::Printf(TEXT("%s strategy instance is null."), Stage.StageName));
			continue;
		}

		if (Stage.ConfiguredClass && Instance->GetClass() != Stage.ConfiguredClass)
		{
			AddError(FString::Printf(
				TEXT("%s strategy is '%s' but the configured class is '%s'."),
				Stage.StageName, *Instance->GetClass()->GetName(), *Stage.ConfiguredClass->GetName()));
		}
	}

	// 3. 기본 구현이 아닌 클래스를 지정하면 그 클래스가 만들어지는가.
	//    1·2번은 기본 설정만 확인하므로, 교체가 되는지는 대조군으로 따로 본다.
	const TSubclassOf<UOCGErosionStrategyBase> CustomSelection = UOCGTestErosionStrategy::StaticClass();
	UOCGErosionStrategyBase* Created = NewObject<UOCGErosionStrategyBase>(GetTransientPackage(), CustomSelection);

	TestNotNull(TEXT("Custom erosion strategy is created"), Created);
	if (Created)
	{
		TestTrue(TEXT("Created instance is the selected subclass"), Created->IsA<UOCGTestErosionStrategy>());
		TestFalse(TEXT("Created instance is not the default implementation"), Created->IsA<UOCGDefaultErosionStrategy>());
	}

	return true;
}
