// Copyright (c) 2025-2026 Code1133. All rights reserved.
#include "Data/OCGWorldDataContainer.h"

void FOCGWorldDataContainer::Reset()
{
	HeightMapData.Reset();
	TemperatureMapData.Reset();
	HumidityMapData.Reset();
	WeightLayers.Reset();
	BiomeLayerMap.Reset();
	MinTemp = MaxTemp = 0.0f;
	MinHumidity = MaxHumidity = 0.0f;
	CurMaxHeight = 0.0f;
}
