// Copyright (c) 2025-2026 Code1133. All rights reserved.
#include "Subsystems/OCGDataGenerationSubsystem.h"

#include "Data/MapPreset.h"
#include "Strategies/OCGDefaultHeightmapStrategy.h"
#include "Strategies/OCGDefaultTemperatureStrategy.h"
#include "Strategies/OCGDefaultHumidityStrategy.h"
#include "Strategies/OCGDefaultBiomeStrategy.h"
#include "Strategies/OCGDefaultErosionStrategy.h"
#include "Strategies/OCGDefaultSmoothingStrategy.h"

void UOCGDataGenerationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	HeightmapStrategy   = NewObject<UOCGDefaultHeightmapStrategy>(this);
	TemperatureStrategy = NewObject<UOCGDefaultTemperatureStrategy>(this);
	HumidityStrategy    = NewObject<UOCGDefaultHumidityStrategy>(this);
	BiomeStrategy       = NewObject<UOCGDefaultBiomeStrategy>(this);
	ErosionStrategy     = NewObject<UOCGDefaultErosionStrategy>(this);
	SmoothingStrategy   = NewObject<UOCGDefaultSmoothingStrategy>(this);
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
	SmoothingStrategy->SmoothHeightMap(Preset, DataContainer);
	ComputeHeightRange(Preset);
}

void UOCGDataGenerationSubsystem::ComputeHeightRange(const UMapPreset* Preset)
{
	const float LandscapeZScale = (Preset->MaxHeight - Preset->MinHeight) * 0.001953125f;
	const float AbsMaxHeight = FMath::Abs(Preset->MaxHeight);
	const float AbsMinHeight = FMath::Abs(Preset->MinHeight);
	const float AbsOffset    = FMath::Abs(AbsMaxHeight - AbsMinHeight) / 2.0f;
	const float ZOffset      = (AbsMaxHeight < AbsMinHeight) ? -AbsOffset : AbsOffset;

	float Max = Preset->MinHeight;
	float Min = Preset->MaxHeight;

	for (const uint16 RawHeight : DataContainer.HeightMapData)
	{
		const float WorldHeight = (RawHeight - 32768.0f) * LandscapeZScale / 128.0f + ZOffset;
		if (WorldHeight > Max) Max = WorldHeight;
		if (WorldHeight < Min) Min = WorldHeight;
	}

	DataContainer.CurMaxHeight = Max;
	DataContainer.CurMinHeight = Min;
}
