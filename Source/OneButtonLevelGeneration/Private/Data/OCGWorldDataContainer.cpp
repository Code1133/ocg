// Copyright (c) 2025-2026 Code1133. All rights reserved.
#include "Data/OCGWorldDataContainer.h"

void FOCGWorldDataContainer::Reset()
{
	HeightMapData.Reset();
	TemperatureMapData.Reset();
	HumidityMapData.Reset();
	WeightLayers.Reset();
	BiomeMap.Reset();
	MinTemp = MaxTemp = 0.0f;
	MinHumidity = MaxHumidity = 0.0f;
	CurMinHeight = CurMaxHeight = 0.0f;
}
