// Copyright (c) 2025-2026 Code1133. All rights reserved.
#include "Strategies/OCGDefaultTerrainModifierStrategy.h"

#include "Data/MapData.h"
#include "Data/MapPreset.h"
#include "Data/OCGWorldDataContainer.h"

void UOCGDefaultTerrainModifierStrategy::ModifyTerrainByBiome(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer, const FOCGHeightConverter& Converter)
{
	if (!Preset->BiomeTerrainSettings.bModifyTerrainByBiome)
	{
		return;
	}

	TArray<float> MinHeights;
	TArray<float> BlurredMinHeights;

	const float HeightRange = Preset->HeightSettings.MaxHeight - Preset->HeightSettings.MinHeight;
	CalculateBiomeMinHeights(Preset, DataContainer.HeightMapData, DataContainer.BiomeLayerMap, MinHeights, Converter);

	if (Preset->BiomeTerrainSettings.BiomeHeightBlendRadius > 0)
	{
		BlurBiomeMinHeights(Preset, MinHeights, BlurredMinHeights);
	}
	else
	{
		BlurredMinHeights = MinHeights;
	}

	const float SeaLevelHeightF = (Preset->OceanSettings.bContainWater ? Preset->HeightSettings.SeaLevel * HeightRange : 0.0f) + Preset->HeightSettings.MinHeight;
	const uint16 SeaLevelHeight = Converter.ToHeightMapValue(SeaLevelHeightF);

	for (int32 Y = 0; Y < Preset->LandscapeSettings.MapResolution.Y; ++Y)
	{
		for (int32 X = 0; X < Preset->LandscapeSettings.MapResolution.X; ++X)
		{
			const int32 Index = Y * Preset->LandscapeSettings.MapResolution.X + X;
			const int32 CurrentLayerIdx = DataContainer.BiomeLayerMap[Index];
			if (CurrentLayerIdx <= 0) continue; // 0 = Water, skip

			const uint16 CurrentHeight = DataContainer.HeightMapData[Index];
			float MtoPRatio = 0.0f;

			for (int32 I = 1; I < DataContainer.WeightLayers.Num(); ++I)
			{
				const FName LayerName = OCGMapDataUtils::MakeLayerName(I);
				const float CurrentBiomeWeight = DataContainer.WeightLayers[LayerName][Index] / 255.0f;
				if (CurrentBiomeWeight <= 0.0f) continue;
				MtoPRatio += Preset->Biomes[I - 1].MountainRatio * CurrentBiomeWeight;
			}

			const uint16 BiomeMinHeight = Converter.ToHeightMapValue(BlurredMinHeights[Index]);
			const uint16 TargetPlainHeight = FMath::Lerp(CurrentHeight, BiomeMinHeight, (1.0f - MtoPRatio) * Preset->BiomeTerrainSettings.PlainSmoothFactor);

			const float MaxAmplitude = (65535.0f - TargetPlainHeight) * Converter.ZScale / HeightRange / 128.0f;
			const float Amplitude = MaxAmplitude * Preset->BiomeTerrainSettings.BiomeNoiseAmplitude;
			const float DetailNoise = FMath::PerlinNoise2D(FVector2D(static_cast<float>(X), static_cast<float>(Y)) * Preset->BiomeTerrainSettings.BiomeNoiseScale) * Amplitude + Amplitude;
			const float HeightToAdd = DetailNoise * HeightRange * 128.0f / Converter.ZScale;
			const float MountainHeight = FMath::Clamp(HeightToAdd + TargetPlainHeight, 0.0f, 65535.0f);

			uint16 NewHeight = FMath::Lerp(TargetPlainHeight, static_cast<uint16>(MountainHeight), MtoPRatio);
			NewHeight = static_cast<uint16>(FMath::Clamp(static_cast<int32>(FMath::Max(NewHeight, SeaLevelHeight)), 0, 65535));
			DataContainer.HeightMapData[Index] = NewHeight;
		}
	}
}

void UOCGDefaultTerrainModifierStrategy::CalculateBiomeMinHeights(const UMapPreset* Preset, const TArray<uint16>& InHeightMap, const TArray<int32>& InBiomeLayerMap, TArray<float>& OutMinHeights, const FOCGHeightConverter& Converter)
{
	const FIntPoint MapSize = Preset->LandscapeSettings.MapResolution;
	const int32 TotalPixels = MapSize.X * MapSize.Y;

	TArray<int32> RegionIDMap;
	RegionIDMap.Init(0, TotalPixels);
	OutMinHeights.Init(0.0f, TotalPixels);

	TMap<int32, float> RegionMinHeight;
	int32 CurrentRegionID = 1;

	for (int32 Y = 0; Y < MapSize.Y; ++Y)
	{
		for (int32 X = 0; X < MapSize.X; ++X)
		{
			if (RegionIDMap[Y * MapSize.X + X] == 0)
			{
				float MinimumHeight;
				GetBiomeStats(MapSize, X, Y, CurrentRegionID, MinimumHeight, RegionIDMap, InHeightMap, InBiomeLayerMap, Converter);
				RegionMinHeight.Add(CurrentRegionID, MinimumHeight);
				++CurrentRegionID;
			}
		}
	}

	for (int32 I = 0; I < TotalPixels; ++I)
	{
		OutMinHeights[I] = RegionMinHeight.FindRef(RegionIDMap[I]);
	}
}

void UOCGDefaultTerrainModifierStrategy::BlurBiomeMinHeights(const UMapPreset* Preset, const TArray<float>& InMinHeights, TArray<float>& OutMinHeights)
{
	const int32 BlendRadius = static_cast<int32>(Preset->BiomeTerrainSettings.BiomeHeightBlendRadius);
	const FIntPoint MapSize = Preset->LandscapeSettings.MapResolution;
	const int32 TotalPixels = MapSize.X * MapSize.Y;
	OutMinHeights.SetNumUninitialized(TotalPixels);

	TArray<float> HorizontalPass;
	HorizontalPass.Init(0.0f, TotalPixels);

	for (int32 Y = 0; Y < MapSize.Y; ++Y)
	{
		float Sum = 0.0f;
		int32 ValidPixelCount = 0;
		for (int32 I = -BlendRadius; I <= BlendRadius; ++I)
		{
			const int32 CurrentX = FMath::Clamp(I, 0, MapSize.X - 1);
			Sum += InMinHeights[Y * MapSize.X + CurrentX];
			++ValidPixelCount;
		}
		HorizontalPass[Y * MapSize.X + 0] = ValidPixelCount > 0 ? Sum / ValidPixelCount : InMinHeights[Y * MapSize.X + 0];

		for (int32 X = 1; X < MapSize.X; ++X)
		{
			const int32 OldX = FMath::Clamp(X - BlendRadius - 1, 0, MapSize.X - 1);
			Sum -= InMinHeights[Y * MapSize.X + OldX];
			--ValidPixelCount;
			const int32 NewX = FMath::Clamp(X + BlendRadius, 0, MapSize.X - 1);
			Sum += InMinHeights[Y * MapSize.X + NewX];
			++ValidPixelCount;
			HorizontalPass[Y * MapSize.X + X] = ValidPixelCount > 0 ? Sum / ValidPixelCount : InMinHeights[Y * MapSize.X + X];
		}
	}

	for (int32 X = 0; X < MapSize.X; ++X)
	{
		float Sum = 0.0f;
		int32 ValidPixelCount = 0;
		for (int32 I = -BlendRadius; I <= BlendRadius; ++I)
		{
			const int32 CurrentY = FMath::Clamp(I, 0, MapSize.Y - 1);
			Sum += HorizontalPass[CurrentY * MapSize.X + X];
			++ValidPixelCount;
		}
		OutMinHeights[0 * MapSize.X + X] = ValidPixelCount > 0 ? Sum / ValidPixelCount : HorizontalPass[0 * MapSize.X + X];

		for (int32 Y = 1; Y < MapSize.Y; ++Y)
		{
			const int32 OldY = FMath::Clamp(Y - BlendRadius - 1, 0, MapSize.Y - 1);
			Sum -= HorizontalPass[OldY * MapSize.X + X];
			--ValidPixelCount;
			const int32 NewY = FMath::Clamp(Y + BlendRadius, 0, MapSize.Y - 1);
			Sum += HorizontalPass[NewY * MapSize.X + X];
			++ValidPixelCount;
			OutMinHeights[Y * MapSize.X + X] = ValidPixelCount > 0 ? Sum / ValidPixelCount : HorizontalPass[Y * MapSize.X + X];
		}
	}
}

void UOCGDefaultTerrainModifierStrategy::GetBiomeStats(FIntPoint MapSize, int32 X, int32 Y, int32 RegionID, float& OutMinHeight, TArray<int32>& RegionIDMap, const TArray<uint16>& InHeightMap, const TArray<int32>& InBiomeLayerMap, const FOCGHeightConverter& Converter)
{
	TQueue<FIntPoint> Queue;
	Queue.Enqueue(FIntPoint(X, Y));

	const int32 TargetLayerIdx = InBiomeLayerMap[Y * MapSize.X + X];
	RegionIDMap[Y * MapSize.X + X] = RegionID;
	OutMinHeight = FLT_MAX;

	FIntPoint CurrentPoint;
	while (Queue.Dequeue(CurrentPoint))
	{
		const uint32 CurrentIndex = CurrentPoint.Y * MapSize.X + CurrentPoint.X;
		const float CurrentHeight = Converter.ToWorldHeight(InHeightMap[CurrentIndex]);
		if (CurrentHeight < OutMinHeight) OutMinHeight = CurrentHeight;

		const FIntPoint Neighbors[] =
		{
			FIntPoint(CurrentPoint.X + 1, CurrentPoint.Y), FIntPoint(CurrentPoint.X - 1, CurrentPoint.Y),
			FIntPoint(CurrentPoint.X, CurrentPoint.Y + 1), FIntPoint(CurrentPoint.X, CurrentPoint.Y - 1),
		};

		for (const FIntPoint& Neighbor : Neighbors)
		{
			if (Neighbor.X >= 0 && Neighbor.X < MapSize.X && Neighbor.Y >= 0 && Neighbor.Y < MapSize.Y)
			{
				const int32 NeighborIndex = Neighbor.Y * MapSize.X + Neighbor.X;
				if (RegionIDMap[NeighborIndex] == 0 && InBiomeLayerMap[NeighborIndex] == TargetLayerIdx)
				{
					RegionIDMap[NeighborIndex] = RegionID;
					Queue.Enqueue(Neighbor);
				}
			}
		}
	}
}
