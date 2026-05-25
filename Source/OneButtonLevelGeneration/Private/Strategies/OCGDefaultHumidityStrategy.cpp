// Copyright (c) 2025-2026 Code1133. All rights reserved.
#include "Strategies/OCGDefaultHumidityStrategy.h"

#include "Data/MapPreset.h"
#include "Data/OCGWorldDataContainer.h"

void UOCGDefaultHumidityStrategy::GenerateHumidityMap(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer)
{
	Initialize(Preset);

	const FIntPoint CurResolution = Preset->MapResolution;
	if (DataContainer.HumidityMapData.Num() != CurResolution.X * CurResolution.Y)
	{
		DataContainer.HumidityMapData.SetNumUninitialized(CurResolution.X * CurResolution.Y);
	}

	const float SeaLevelWorldHeight = Preset->bContainWater
		? Preset->MinHeight + Preset->SeaLevel * (Preset->MaxHeight - Preset->MinHeight)
		: Preset->MinHeight;

	TArray<float> HumidityMapFloat;
	HumidityMapFloat.Init(0.0f, CurResolution.X * CurResolution.Y);

	TArray<float> DistanceToWater;
	DistanceToWater.Init(TNumericLimits<float>::Max(), CurResolution.X * CurResolution.Y);

	// 1. Find water pixels and Enqueue them
	TQueue<FIntPoint> Frontier;
	for (int32 Y = 0; Y < CurResolution.Y; ++Y)
	{
		for (int32 X = 0; X < CurResolution.X; ++X)
		{
			const int32 Index = Y * CurResolution.X + X;
			const float WorldHeight = HeightMapToWorldHeight(DataContainer.HeightMapData[Index]);

			if (WorldHeight <= SeaLevelWorldHeight)
			{
				DistanceToWater[Index] = 0; // water pixels' distance from water is 0
				Frontier.Enqueue(FIntPoint(X, Y));
			}
		}
	}

	// Use BFS to calculate closest distance to water
	constexpr int32 DeltaX[] = { 0, 0, 1, -1 };
	constexpr int32 DeltaY[] = { 1, -1, 0, 0 };

	while (!Frontier.IsEmpty())
	{
		FIntPoint Current;
		Frontier.Dequeue(Current);

		for (int32 i = 0; i < 4; ++i)
		{
			const int32 NeighborX = Current.X + DeltaX[i];
			const int32 NeighborY = Current.Y + DeltaY[i];

			if (NeighborX >= 0 && NeighborX < CurResolution.X && NeighborY >= 0 && NeighborY < CurResolution.Y)
			{
				const int32 NeighborIndex = NeighborY * CurResolution.X + NeighborX;
				// if Neighbor is unvisited (distance to water is initial value)
				if (DistanceToWater[NeighborIndex] == TNumericLimits<float>::Max())
				{
					DistanceToWater[NeighborIndex] = DistanceToWater[Current.Y * CurResolution.X + Current.X] + 1.0f;
					Frontier.Enqueue(FIntPoint(NeighborX, NeighborY));
				}
			}
		}
	}

	// 2. Calculate humidity based on distance and temperature
	float GlobalMinHumidity = TNumericLimits<float>::Max();
	float GlobalMaxHumidity = TNumericLimits<float>::Lowest();

	for (int32 i = 0; i < CurResolution.X * CurResolution.Y; ++i)
	{
		const float FinalHumidity = FMath::Clamp([&]() -> float
		{
			if (DistanceToWater[i] == 0)
			{
				// water pixel's humidity is always 1
				return 1.0f;
			}

			// decide humidity based on distance
			const float HumidityFromDistance = FMath::Exp(-DistanceToWater[i] * Preset->MoistureFalloffRate);

			// apply temperature affect
			const float NormalizedTemp = static_cast<float>(DataContainer.TemperatureMapData[i]) / 65535.0f;
			return HumidityFromDistance * (1.0f - (NormalizedTemp * Preset->TemperatureInfluenceOnHumidity));
		}(), 0.0f, 1.0f);

		HumidityMapFloat[i] = FinalHumidity;

		if (FinalHumidity < GlobalMinHumidity) GlobalMinHumidity = FinalHumidity;
		if (FinalHumidity > GlobalMaxHumidity) GlobalMaxHumidity = FinalHumidity;
	}

	// Store global min/max so the Biome strategy can convert uint16 back to actual humidity
	DataContainer.MinHumidity = GlobalMinHumidity;
	DataContainer.MaxHumidity = GlobalMaxHumidity;

	// 3. Convert to uint16 data
	float HumidityRange = GlobalMaxHumidity - GlobalMinHumidity;
	if (HumidityRange < KINDA_SMALL_NUMBER)
	{
		HumidityRange = 1.0f;
	}

	for (int32 i = 0; i < HumidityMapFloat.Num(); ++i)
	{
		const float NormalizedHumidity = (HumidityMapFloat[i] - GlobalMinHumidity) / HumidityRange;
		DataContainer.HumidityMapData[i] = static_cast<uint16>(NormalizedHumidity * 65535.0f);
	}
}

void UOCGDefaultHumidityStrategy::Initialize(const UMapPreset* Preset)
{
	// HeightMap conversion constants
	LandscapeZScale = (Preset->MaxHeight - Preset->MinHeight) * 0.001953125f;
	const float AbsMaxHeight = FMath::Abs(Preset->MaxHeight);
	const float AbsMinHeight = FMath::Abs(Preset->MinHeight);
	const float AbsOffset    = FMath::Abs(AbsMaxHeight - AbsMinHeight) / 2.0f;
	ZOffset = (AbsMaxHeight < AbsMinHeight) ? -AbsOffset : AbsOffset;
}

float UOCGDefaultHumidityStrategy::HeightMapToWorldHeight(const uint16 Height) const
{
	// Add ZOffset to return actual world height;
	// ZOffset is 0 if the absolute values of MaxHeight and MinHeight are equal.
	return (Height - 32768.f) * LandscapeZScale / 128.f + ZOffset;
}
