// Copyright (c) 2025-2026 Code1133. All rights reserved.
#include "Subsystems/OCGDataGenerationSubsystem.h"

#include "OCGDeveloperSettings.h"
#include "OCGLog.h"
#include "Data/MapPreset.h"
#include "Data/OCGHeightConverter.h"
#include "Strategies/OCGDefaultBiomeStrategy.h"
#include "Strategies/OCGDefaultErosionStrategy.h"
#include "Strategies/OCGDefaultHeightmapStrategy.h"
#include "Strategies/OCGDefaultHumidityStrategy.h"
#include "Strategies/OCGDefaultSmoothingStrategy.h"
#include "Strategies/OCGDefaultTemperatureStrategy.h"
#include "Strategies/OCGDefaultTerrainModifierStrategy.h"

namespace
{
/**
 * 프로젝트 설정에 지정된 클래스로 전략을 만듭니다.
 * 비어 있거나 추상 클래스면 기본 구현으로 폴백합니다.
 */
template <typename TStrategyBase, typename TDefaultStrategy>
TStrategyBase* CreateStrategy(UObject* Outer, const TSubclassOf<TStrategyBase>& ConfiguredClass, const TCHAR* StageName)
{
	UClass* StrategyClass = ConfiguredClass.Get();
	if (!StrategyClass)
	{
		return NewObject<TDefaultStrategy>(Outer);
	}

	if (StrategyClass->HasAnyClassFlags(CLASS_Abstract))
	{
		UE_LOG(
			LogOCGModule, Warning,
			TEXT("%s strategy class '%s' is abstract. Falling back to the default implementation."),
			StageName, *StrategyClass->GetName()
		);
		return NewObject<TDefaultStrategy>(Outer);
	}

	return NewObject<TStrategyBase>(Outer, StrategyClass);
}
}

void UOCGDataGenerationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const UOCGDeveloperSettings* Settings = GetDefault<UOCGDeveloperSettings>();

	HeightmapStrategy = CreateStrategy<UOCGHeightmapStrategyBase, UOCGDefaultHeightmapStrategy>(
		this, Settings->HeightmapStrategyClass, TEXT("Heightmap"));
	TemperatureStrategy = CreateStrategy<UOCGTemperatureStrategyBase, UOCGDefaultTemperatureStrategy>(
		this, Settings->TemperatureStrategyClass, TEXT("Temperature"));
	HumidityStrategy = CreateStrategy<UOCGHumidityStrategyBase, UOCGDefaultHumidityStrategy>(
		this, Settings->HumidityStrategyClass, TEXT("Humidity"));
	BiomeStrategy = CreateStrategy<UOCGBiomeStrategyBase, UOCGDefaultBiomeStrategy>(
		this, Settings->BiomeStrategyClass, TEXT("Biome"));
	TerrainModifierStrategy = CreateStrategy<UOCGTerrainModifierStrategyBase, UOCGDefaultTerrainModifierStrategy>(
		this, Settings->TerrainModifierStrategyClass, TEXT("Terrain Modifier"));
	ErosionStrategy = CreateStrategy<UOCGErosionStrategyBase, UOCGDefaultErosionStrategy>(
		this, Settings->ErosionStrategyClass, TEXT("Erosion"));
	SmoothingStrategy = CreateStrategy<UOCGSmoothingStrategyBase, UOCGDefaultSmoothingStrategy>(
		this, Settings->SmoothingStrategyClass, TEXT("Smoothing"));
}

void UOCGDataGenerationSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UOCGDataGenerationSubsystem::GenerateData(const UMapPreset* Preset)
{
	if (!IsValid(Preset))
	{
		return;
	}

	DataContainer.Reset();
	HeightmapStrategy->GenerateHeightMap(Preset, DataContainer);
	TemperatureStrategy->GenerateTemperatureMap(Preset, DataContainer);
	HumidityStrategy->GenerateHumidityMap(Preset, DataContainer);
	BiomeStrategy->DecideAndBlendBiomes(Preset, DataContainer);

	// v1 파이프라인 순서: TerrainModify -> Smooth -> FinalizeBiomes -> Erosion
	FOCGHeightConverter HeightConverter;
	HeightConverter.Initialize(Preset);

	TerrainModifierStrategy->ModifyTerrainByBiome(Preset, DataContainer, HeightConverter);
	SmoothingStrategy->SmoothHeightMap(Preset, DataContainer);
	BiomeStrategy->FinalizeBiomes(Preset, DataContainer);
	ErosionStrategy->ApplyErosion(Preset, DataContainer);
	ComputeMaxHeight(Preset);
}

void UOCGDataGenerationSubsystem::ComputeMaxHeight(const UMapPreset* Preset)
{
	FOCGHeightConverter HeightConverter;
	HeightConverter.Initialize(Preset);

	float Max = Preset->HeightSettings.MinHeight;

	for (const uint16 RawHeight : DataContainer.HeightMapData)
	{
		const float WorldHeight = HeightConverter.ToWorldHeight(RawHeight);
		if (WorldHeight > Max) Max = WorldHeight;
	}

	DataContainer.CurMaxHeight = Max;
}
