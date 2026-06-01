// Copyright (c) 2025-2026 Code1133. All rights reserved.
#include "Data/OCGHeightConverter.h"

#include "Data/MapPreset.h"

void FOCGHeightConverter::Initialize(const UMapPreset* Preset)
{
	ZScale = (Preset->MaxHeight - Preset->MinHeight) * 0.001953125f;

	const float AbsMaxHeight = FMath::Abs(Preset->MaxHeight);
	const float AbsMinHeight = FMath::Abs(Preset->MinHeight);
	const float AbsOffset    = FMath::Abs(AbsMaxHeight - AbsMinHeight) / 2.0f;
	ZOffset = (AbsMaxHeight < AbsMinHeight) ? -AbsOffset : AbsOffset;
}

float FOCGHeightConverter::GetSeaLevelWorldHeight(const UMapPreset* Preset)
{
	return Preset->bContainWater
		? Preset->MinHeight + Preset->SeaLevel * (Preset->MaxHeight - Preset->MinHeight)
		: Preset->MinHeight;
}
