// Copyright (c) 2025-2026 Code1133. All rights reserved.
#include "Strategies/OCGDefaultBiomeStrategy.h"

#include "Data/MapPreset.h"
#include "Data/OCGWorldDataContainer.h"
#include "OCGLog.h"
#include "OCGStats.h"

void UOCGDefaultBiomeStrategy::DecideAndBlendBiomes(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer)
{
	SCOPE_CYCLE_COUNTER(STAT_OCG_BiomeDecide);

	// Calculate total weight across all biomes for the distance metric normalization
	float TotalWeight = 0.0f;
	for (const auto& Biome : Preset->Biomes)
	{
		TotalWeight += Biome.Weight;
	}

	const FIntPoint CurResolution = Preset->MapResolution;
	const int32 PixelCount = CurResolution.X * CurResolution.Y;

	// Initialize per-pixel maps
	BiomeNameMap.SetNumUninitialized(PixelCount);
	DataContainer.BiomeLayerMap.SetNumUninitialized(PixelCount);

	// Initialize weight layers: Layer0 = Water, Layer1..N = Biomes
	DataContainer.WeightLayers.Reset();
	for (int32 LayerIndex = 0; LayerIndex <= Preset->Biomes.Num(); ++LayerIndex)
	{
		FName LayerName = *FString::Printf(TEXT("Layer%d"), LayerIndex);

		TArray<uint8> WeightLayer;
		WeightLayer.SetNumZeroed(PixelCount);
		DataContainer.WeightLayers.Add(LayerName, MoveTemp(WeightLayer));
	}

	const uint16 SeaLevelHeight = Preset->bContainWater
		? static_cast<uint16>(65535 * Preset->SeaLevel)
		: 0;

	// Assign each pixel to the nearest biome via weighted temp/humidity distance
	for (int32 Y = 0; Y < CurResolution.Y; ++Y)
	{
		for (int32 X = 0; X < CurResolution.X; ++X)
		{
			const int32 Index = Y * CurResolution.X + X;
			const float Height = DataContainer.HeightMapData[Index];

			const float NormalizedTemp = static_cast<float>(DataContainer.TemperatureMapData[Index]) / 65535.0f;
			const float Temp = FMath::Lerp(DataContainer.MinTemp, DataContainer.MaxTemp, NormalizedTemp);

			const float NormalizedHumidity = static_cast<float>(DataContainer.HumidityMapData[Index]) / 65535.0f;
			const float Humidity = FMath::Lerp(DataContainer.MinHumidity, DataContainer.MaxHumidity, NormalizedHumidity);

			const FOCGBiomeSettings* CurrentBiome = nullptr;
			const FOCGBiomeSettings* WaterBiome = &Preset->WaterBiome;
			uint32 CurrentBiomeIndex = INDEX_NONE;

			if (WaterBiome && Height < SeaLevelHeight)
			{
				CurrentBiome = WaterBiome;
				CurrentBiomeIndex = 0; // Water biome is first layer
			}
			else
			{
				float MinDist = TNumericLimits<float>::Max();
				const float TempRange = Preset->MaxTemp - Preset->MinTemp;

				for (int32 BiomeIndex = 1; BiomeIndex <= Preset->Biomes.Num(); ++BiomeIndex)
				{
					const FOCGBiomeSettings* BiomeSettings = &Preset->Biomes[BiomeIndex - 1];
					const float TempDiff     = FMath::Abs(BiomeSettings->Temperature - Temp) / TempRange;
					const float HumidityDiff = FMath::Abs(BiomeSettings->Humidity - Humidity);
					const float Weight       = 1.0f - BiomeSettings->Weight / TotalWeight;
					const float Dist         = FVector2D(TempDiff, HumidityDiff).Length() * Weight;

					if (Dist < MinDist)
					{
						MinDist = Dist;
						CurrentBiome = BiomeSettings;
						CurrentBiomeIndex = BiomeIndex;
					}
				}
			}

			if (CurrentBiome)
			{
				if (CurrentBiomeIndex != INDEX_NONE)
				{
					FName LayerName = *FString::Printf(TEXT("Layer%d"), CurrentBiomeIndex);

					DataContainer.WeightLayers[LayerName][Index] = 255;
					BiomeNameMap[Index] = LayerName;
					DataContainer.BiomeLayerMap[Index] = static_cast<int32>(CurrentBiomeIndex);
				}
				else
				{
					UE_LOG(LogOCGModule, Display, TEXT("Current Biome index is invalid"));
				}
			}
		}
	}

	BlendBiomes(Preset, DataContainer);
}

void UOCGDefaultBiomeStrategy::BlendBiomes(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer)
{
	SCOPE_CYCLE_COUNTER(STAT_OCG_BiomeBlend);

	const FIntPoint CurResolution = Preset->MapResolution;

	// Reset WeightLayers to 0 and build the unblurred source map from BiomeNameMap
	TMap<FName, TArray<uint8>> OriginalWeightMaps;
	for (int32 LayerIndex = 0; LayerIndex < DataContainer.WeightLayers.Num(); ++LayerIndex)
	{
		FName LayerName = *FString::Printf(TEXT("Layer%d"), LayerIndex);
		DataContainer.WeightLayers[LayerName].SetNumZeroed(CurResolution.X * CurResolution.Y);

		TArray<uint8> InitialWeights;
		InitialWeights.SetNumZeroed(CurResolution.X * CurResolution.Y);
		OriginalWeightMaps.FindOrAdd(LayerName, MoveTemp(InitialWeights));
	}

	// Generate unblurred map: each pixel's assigned layer gets weight 255
	for (int32 i = 0; i < CurResolution.X * CurResolution.Y; ++i)
	{
		if (OriginalWeightMaps.Contains(BiomeNameMap[i]))
		{
			OriginalWeightMaps[BiomeNameMap[i]][i] = 255;
		}
	}

	// Horizontal blur pass (sliding window sum)
	TMap<FName, TArray<float>> HorizontalPassMaps;
	HorizontalPassMaps.Reserve(OriginalWeightMaps.Num());
	for (const auto& Elem : OriginalWeightMaps)
	{
		HorizontalPassMaps.Emplace(Elem.Key).SetNumZeroed(CurResolution.X * CurResolution.Y);
	}

	// Water layer is always Layer0
	static const FName WaterLayerName = TEXT("Layer0");
	for (const auto& [LayerName, OriginalLayer] : OriginalWeightMaps)
	{
		TArray<float>& HorizontalPassLayer = HorizontalPassMaps.FindChecked(LayerName);

		const int32 BlendRadius = (LayerName == WaterLayerName)
		    ? Preset->WaterBlendRadius
		    : Preset->BiomeBlendRadius;

		for (int32 Y = 0; Y < CurResolution.Y; ++Y)
		{
			// Accumulate first pixel's window sum
			float Sum = 0.0f;
			for (int32 i = -BlendRadius; i <= BlendRadius; ++i)
			{
				const int32 SampleX = FMath::Clamp(i, 0, CurResolution.X - 1);
				Sum += OriginalLayer[Y * CurResolution.X + SampleX];
			}
			HorizontalPassLayer[Y * CurResolution.X + 0] = Sum;

			// Slide the window: subtract left edge, add right edge
			for (int32 X = 1; X < CurResolution.X; ++X)
			{
				const int32 OldX = FMath::Clamp(X - BlendRadius - 1, 0, CurResolution.X - 1);
				const int32 NewX = FMath::Clamp(X + BlendRadius,     0, CurResolution.X - 1);
				Sum += OriginalLayer[Y * CurResolution.X + NewX] - OriginalLayer[Y * CurResolution.X + OldX];
				HorizontalPassLayer[Y * CurResolution.X + X] = Sum;
			}
		}
	}

	// Vertical blur pass (sliding window sum on horizontal pass result)
	for (const auto& [LayerName, HorizontalPassLayer] : HorizontalPassMaps)
	{
		TArray<uint8>& FinalLayer = DataContainer.WeightLayers.FindChecked(LayerName);

		const int32 BlendRadius = (LayerName == WaterLayerName)
		    ? Preset->WaterBlendRadius
		    : Preset->BiomeBlendRadius;

		const float BlendFactor = 1.0f / ((BlendRadius * 2 + 1) * (BlendRadius * 2 + 1));

		for (int32 X = 0; X < CurResolution.X; ++X)
		{
			// Accumulate first pixel's window sum
			float Sum = 0.0f;
			for (int32 i = -BlendRadius; i <= BlendRadius; ++i)
			{
				const int32 SampleY = FMath::Clamp(i, 0, CurResolution.Y - 1);
				Sum += HorizontalPassLayer[SampleY * CurResolution.X + X];
			}
			FinalLayer[X] = FMath::RoundToInt(Sum * BlendFactor);

			// Slide the window
			for (int32 Y = 1; Y < CurResolution.Y; ++Y)
			{
				const int32 OldY = FMath::Clamp(Y - BlendRadius - 1, 0, CurResolution.Y - 1);
				const int32 NewY = FMath::Clamp(Y + BlendRadius,     0, CurResolution.Y - 1);
				Sum += HorizontalPassLayer[NewY * CurResolution.X + X] - HorizontalPassLayer[OldY * CurResolution.X + X];
				FinalLayer[Y * CurResolution.X + X] = FMath::RoundToInt(Sum * BlendFactor);
			}
		}
	}

	// make sure each pixel's weight sum is equal to 255
	for (int32 i = 0; i < CurResolution.X * CurResolution.Y; ++i)
	{
		float TotalNormWeight = 0.0f;
		for (int32 LayerIndex = 0; LayerIndex < DataContainer.WeightLayers.Num(); ++LayerIndex)
		{
			FName LayerName = *FString::Printf(TEXT("Layer%d"), LayerIndex);
			TotalNormWeight += DataContainer.WeightLayers[LayerName][i];
		}

		if (TotalNormWeight > 0.0f)
		{
			const float NormFactor = 255.0f / TotalNormWeight;
			for (int32 LayerIndex = 0; LayerIndex < DataContainer.WeightLayers.Num(); ++LayerIndex)
			{
				FName LayerName = *FString::Printf(TEXT("Layer%d"), LayerIndex);
				DataContainer.WeightLayers[LayerName][i] = FMath::RoundToInt(DataContainer.WeightLayers[LayerName][i] * NormFactor);
			}
		}
	}
}

void UOCGDefaultBiomeStrategy::FinalizeBiomes(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer)
{
	if (!Preset->bContainWater)
	{
		return;
	}

	float TotalWeight = 0.0f;
	for (const auto& Biome : Preset->Biomes)
	{
		TotalWeight += Biome.Weight;
	}

	const FIntPoint CurResolution = Preset->MapResolution;
	const uint16 SeaLevelHeight = static_cast<uint16>(65535 * Preset->SeaLevel);

	for (int32 Y = 0; Y < CurResolution.Y; ++Y)
	{
		for (int32 X = 0; X < CurResolution.X; ++X)
		{
			const int32 Index = Y * CurResolution.X + X;
			const float Height = DataContainer.HeightMapData[Index];

			const float NormalizedTemp = static_cast<float>(DataContainer.TemperatureMapData[Index]) / 65535.0f;
			const float Temp = FMath::Lerp(DataContainer.MinTemp, DataContainer.MaxTemp, NormalizedTemp);

			const float NormalizedHumidity = static_cast<float>(DataContainer.HumidityMapData[Index]) / 65535.0f;
			const float Humidity = FMath::Lerp(DataContainer.MinHumidity, DataContainer.MaxHumidity, NormalizedHumidity);

			const FOCGBiomeSettings* CurrentBiome = nullptr;
			const FOCGBiomeSettings* WaterBiome = &Preset->WaterBiome;
			uint32 CurrentBiomeIndex = INDEX_NONE;

			if (WaterBiome && Height < SeaLevelHeight)
			{
				CurrentBiome = WaterBiome;
				CurrentBiomeIndex = 0;
			}
			else
			{
				float MinDist = TNumericLimits<float>::Max();
				const float TempRange = Preset->MaxTemp - Preset->MinTemp;

				for (int32 BiomeIndex = 1; BiomeIndex <= Preset->Biomes.Num(); ++BiomeIndex)
				{
					const FOCGBiomeSettings* BiomeSettings = &Preset->Biomes[BiomeIndex - 1];
					const float TempDiff     = FMath::Abs(BiomeSettings->Temperature - Temp) / TempRange;
					const float HumidityDiff = FMath::Abs(BiomeSettings->Humidity - Humidity);
					const float Weight       = 1.0f - BiomeSettings->Weight / TotalWeight;
					const float Dist         = FVector2D(TempDiff, HumidityDiff).Length() * Weight;

					if (Dist < MinDist)
					{
						MinDist = Dist;
						CurrentBiome = BiomeSettings;
						CurrentBiomeIndex = BiomeIndex;
					}
				}
			}

			if (CurrentBiome && CurrentBiomeIndex != INDEX_NONE)
			{
				const FName LayerName = *FString::Printf(TEXT("Layer%d"), CurrentBiomeIndex);
				BiomeNameMap[Index] = LayerName;
				DataContainer.BiomeLayerMap[Index] = static_cast<int32>(CurrentBiomeIndex);
			}
		}
	}

	BlendBiomes(Preset, DataContainer);
}
