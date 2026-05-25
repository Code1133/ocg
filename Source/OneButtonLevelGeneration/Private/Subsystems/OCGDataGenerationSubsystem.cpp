// Copyright (c) 2025-2026 Code1133. All rights reserved.
#include "Subsystems/OCGDataGenerationSubsystem.h"

#include "Data/MapPreset.h"
#include "Strategies/OCGDefaultHeightmapStrategy.h"
#include "Strategies/OCGDefaultTemperatureStrategy.h"
#include "Strategies/OCGDefaultHumidityStrategy.h"
#include "Strategies/OCGDefaultBiomeStrategy.h"
#include "Strategies/OCGDefaultErosionStrategy.h"

void UOCGDataGenerationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	HeightmapStrategy   = NewObject<UOCGDefaultHeightmapStrategy>(this);
	TemperatureStrategy = NewObject<UOCGDefaultTemperatureStrategy>(this);
	HumidityStrategy    = NewObject<UOCGDefaultHumidityStrategy>(this);
	BiomeStrategy       = NewObject<UOCGDefaultBiomeStrategy>(this);
	ErosionStrategy     = NewObject<UOCGDefaultErosionStrategy>(this);
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
	ErosionStrategy->ApplyErosion(Preset, DataContainer);
}
