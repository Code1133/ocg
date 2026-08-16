// Copyright (c) 2025-2026 Code1133. All rights reserved.
#include "Strategies/OCGDefaultTemperatureStrategy.h"

#include "Data/MapPreset.h"
#include "Data/OCGWorldDataContainer.h"
#include "OCGStats.h"

void UOCGDefaultTemperatureStrategy::GenerateTemperatureMap(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer)
{
	SCOPE_CYCLE_COUNTER(STAT_OCG_TempMapGenerate);

	Initialize(Preset);

	const FIntPoint CurResolution = Preset->LandscapeSettings.MapResolution;
	if (DataContainer.TemperatureMapData.Num() != CurResolution.X * CurResolution.Y)
	{
		DataContainer.TemperatureMapData.SetNumUninitialized(CurResolution.X * CurResolution.Y);
	}

	TArray<float> TempMapFloat;
	TempMapFloat.SetNumUninitialized(CurResolution.X * CurResolution.Y);

	float GlobalMinTemp = TNumericLimits<float>::Max();
	float GlobalMaxTemp = TNumericLimits<float>::Lowest();
	const float TempRange = Preset->TemperatureSettings.MaxTemp - Preset->TemperatureSettings.MinTemp;

	const float SeaLevelHeight = FOCGHeightConverter::GetSeaLevelWorldHeight(Preset);

	for (int32 y = 0; y < CurResolution.Y; ++y)
	{
		for (int32 x = 0; x < CurResolution.X; ++x)
		{
			const int32 Index = y * CurResolution.X + x;

			// Generate base temperature map with low frequency noise
			const float TempNoiseInputX = x * Preset->BasicNoiseSettings.TemperatureNoiseScale + PlainNoiseOffset.X;
			const float TempNoiseInputY = y * Preset->BasicNoiseSettings.TemperatureNoiseScale + PlainNoiseOffset.Y;
			float TempNoiseAlpha = FMath::PerlinNoise2D(FVector2D(TempNoiseInputX, TempNoiseInputY)) * 0.5f + 0.5f;
			float BaseTemp = FMath::Lerp(Preset->TemperatureSettings.MinTemp, Preset->TemperatureSettings.MaxTemp, TempNoiseAlpha);

			// Decrease temperature by altitude
			const float WorldHeight = HeightConverter.ToWorldHeight(DataContainer.HeightMapData[Index]);
			if (WorldHeight > SeaLevelHeight)
			{
				BaseTemp -= ((WorldHeight - SeaLevelHeight) / 1000.0f) * Preset->AdvancedTemperatureSettings.TempDropPer1000Units;
			}

			const float NormalizedBaseTemp = (BaseTemp - Preset->TemperatureSettings.MinTemp) / TempRange;
			BaseTemp = Preset->TemperatureSettings.MinTemp + NormalizedBaseTemp * TempRange;
			const float FinalTemp = FMath::Clamp(BaseTemp, Preset->TemperatureSettings.MinTemp, Preset->TemperatureSettings.MaxTemp);

			TempMapFloat[Index] = FinalTemp;

			if (FinalTemp < GlobalMinTemp) GlobalMinTemp = FinalTemp;
			if (FinalTemp > GlobalMaxTemp) GlobalMaxTemp = FinalTemp;
		}
	}

	// Store global min/max so the Biome strategy can convert uint16 back to actual temperature
	DataContainer.MinTemp = GlobalMinTemp;
	DataContainer.MaxTemp = GlobalMaxTemp;

	// Convert float temperature to uint16
	float NormRange = GlobalMaxTemp - GlobalMinTemp;
	if (NormRange < KINDA_SMALL_NUMBER)
	{
		NormRange = 1.0f; // prevent divide-by-zero
	}

	for (int32 i = 0; i < TempMapFloat.Num(); ++i)
	{
		// Normalize temperature to [0, 1]
		const float NormalizedTemp = (TempMapFloat[i] - GlobalMinTemp) / NormRange;

		// convert [0, 1] to [0, 65535]
		DataContainer.TemperatureMapData[i] = static_cast<uint16>(NormalizedTemp * 65535.0f);
	}
}

void UOCGDefaultTemperatureStrategy::Initialize(const UMapPreset* Preset)
{
	// Replicate the stream draw order from OCGMapGenerateComponent::InitializeNoiseOffsets
	// so that PlainNoiseOffset matches the original component's value for the same Seed.
	FRandomStream Stream;
	Stream.Initialize(Preset->Seed);

	float NoiseScale = 1.0f;
	if (Preset->LandscapeSettings.ApplyScaleToNoise)
	{
		// Alter noise scale based on LandscapeScale; use log so the scale does not increase linearly.
		// A linearly increasing scale results in excessively high values.
		const float LandscapeScale = Preset->LandscapeSettings.LandscapeScale;
		if (LandscapeScale > 0.0f)
		{
			NoiseScale = FMath::LogX(25.0f, LandscapeScale) + 1.0f;
		}
	}

	const float StandardNoiseOffset = Preset->AdvancedNoiseSettings.StandardNoiseOffset * NoiseScale;

	// PlainNoiseOffset is the first pair drawn from the stream — must match component order
	PlainNoiseOffset.X = Stream.FRandRange(-StandardNoiseOffset, StandardNoiseOffset);
	PlainNoiseOffset.Y = Stream.FRandRange(-StandardNoiseOffset, StandardNoiseOffset);

	HeightConverter.Initialize(Preset);
}
