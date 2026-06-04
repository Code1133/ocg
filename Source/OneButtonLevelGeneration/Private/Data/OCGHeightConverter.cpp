// Copyright (c) 2025-2026 Code1133. All rights reserved.
#include "Data/OCGHeightConverter.h"

#include "Data/MapPreset.h"

void FOCGHeightConverter::Initialize(const UMapPreset* Preset)
{
	ZScale = (Preset->HeightSettings.MaxHeight - Preset->HeightSettings.MinHeight) * 0.001953125f;

	const float AbsMaxHeight = FMath::Abs(Preset->HeightSettings.MaxHeight);
	const float AbsMinHeight = FMath::Abs(Preset->HeightSettings.MinHeight);
	const float AbsOffset    = FMath::Abs(AbsMaxHeight - AbsMinHeight) / 2.0f;
	ZOffset = (AbsMaxHeight < AbsMinHeight) ? -AbsOffset : AbsOffset;
}

float FOCGHeightConverter::GetSeaLevelWorldHeight(const UMapPreset* Preset)
{
	return Preset->bContainWater
		? Preset->HeightSettings.MinHeight + Preset->HeightSettings.SeaLevel * (Preset->HeightSettings.MaxHeight - Preset->HeightSettings.MinHeight)
		: Preset->HeightSettings.MinHeight;
}
