// Copyright (c) 2025-2026 Code1133. All rights reserved.
#include "Subsystems/OCGDataGenerationSubsystem.h"

#include "Data/MapPreset.h"
#include "Data/OCGHeightConverter.h"
#include "Strategies/OCGDefaultHeightmapStrategy.h"
#include "Strategies/OCGDefaultTemperatureStrategy.h"
#include "Strategies/OCGDefaultHumidityStrategy.h"
#include "Strategies/OCGDefaultBiomeStrategy.h"
#include "Strategies/OCGDefaultTerrainModifierStrategy.h"
#include "Strategies/OCGDefaultErosionStrategy.h"
#include "Strategies/OCGDefaultSmoothingStrategy.h"

void UOCGDataGenerationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	HeightmapStrategy       = NewObject<UOCGDefaultHeightmapStrategy>(this);
	TemperatureStrategy     = NewObject<UOCGDefaultTemperatureStrategy>(this);
	HumidityStrategy        = NewObject<UOCGDefaultHumidityStrategy>(this);
	BiomeStrategy           = NewObject<UOCGDefaultBiomeStrategy>(this);
	TerrainModifierStrategy = NewObject<UOCGDefaultTerrainModifierStrategy>(this);
	ErosionStrategy         = NewObject<UOCGDefaultErosionStrategy>(this);
	SmoothingStrategy       = NewObject<UOCGDefaultSmoothingStrategy>(this);
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
	ComputeHeightRange(Preset);
}

void UOCGDataGenerationSubsystem::ComputeHeightRange(const UMapPreset* Preset)
{
	FOCGHeightConverter HeightConverter;
	HeightConverter.Initialize(Preset);

	float Max = Preset->HeightSettings.MinHeight;
	float Min = Preset->HeightSettings.MaxHeight;

	for (const uint16 RawHeight : DataContainer.HeightMapData)
	{
		const float WorldHeight = HeightConverter.ToWorldHeight(RawHeight);
		if (WorldHeight > Max) Max = WorldHeight;
		if (WorldHeight < Min) Min = WorldHeight;
	}

	DataContainer.CurMaxHeight = Max;
	DataContainer.CurMinHeight = Min;
}
