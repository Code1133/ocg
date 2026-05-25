// Copyright (c) 2025-2026 Code1133. All rights reserved.
#include "Subsystems/OCGDataGenerationSubsystem.h"

#include "Data/MapPreset.h"
#include "Strategies/OCGDefaultHeightmapStrategy.h"

void UOCGDataGenerationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	HeightmapStrategy = NewObject<UOCGDefaultHeightmapStrategy>(this);
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
}
