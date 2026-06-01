// Copyright (c) 2025-2026 Code1133. All rights reserved.
#include "Strategies/OCGDefaultTemperatureStrategy.h"

#include "Data/MapPreset.h"
#include "Data/OCGWorldDataContainer.h"
#include "OCGStats.h"

void UOCGDefaultTemperatureStrategy::GenerateTemperatureMap(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer)
{
	SCOPE_CYCLE_COUNTER(STAT_OCG_TempMapGenerate);

	Initialize(Preset);

	const FIntPoint CurResolution = Preset->MapResolution;
	if (DataContainer.TemperatureMapData.Num() != CurResolution.X * CurResolution.Y)
	{
		DataContainer.TemperatureMapData.SetNumUninitialized(CurResolution.X * CurResolution.Y);
	}

	TArray<float> TempMapFloat;
	TempMapFloat.SetNumUninitialized(CurResolution.X * CurResolution.Y);

	float GlobalMinTemp = TNumericLimits<float>::Max();
	float GlobalMaxTemp = TNumericLimits<float>::Lowest();
	const float TempRange = Preset->MaxTemp - Preset->MinTemp;

	const float SeaLevelHeight = Preset->bContainWater
		? Preset->MinHeight + Preset->SeaLevel * (Preset->MaxHeight - Preset->MinHeight)
		: Preset->MinHeight;

	for (int32 y = 0; y < CurResolution.Y; ++y)
	{
		for (int32 x = 0; x < CurResolution.X; ++x)
		{
			const int32 Index = y * CurResolution.X + x;

			// Generate base temperature map with low frequency noise
			const float TempNoiseInputX = x * Preset->TemperatureNoiseScale + PlainNoiseOffset.X;
			const float TempNoiseInputY = y * Preset->TemperatureNoiseScale + PlainNoiseOffset.Y;
			float TempNoiseAlpha = FMath::PerlinNoise2D(FVector2D(TempNoiseInputX, TempNoiseInputY)) * 0.5f + 0.5f;
			float BaseTemp = FMath::Lerp(Preset->MinTemp, Preset->MaxTemp, TempNoiseAlpha);

			// Decrease temperature by altitude
			const float WorldHeight = HeightMapToWorldHeight(DataContainer.HeightMapData[Index]);
			if (WorldHeight > SeaLevelHeight)
			{
				BaseTemp -= ((WorldHeight - SeaLevelHeight) / 1000.0f) * Preset->TempDropPer1000Units;
			}

			const float NormalizedBaseTemp = (BaseTemp - Preset->MinTemp) / TempRange;
			BaseTemp = Preset->MinTemp + NormalizedBaseTemp * TempRange;
			const float FinalTemp = FMath::Clamp(BaseTemp, Preset->MinTemp, Preset->MaxTemp);

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
	if (Preset->ApplyScaleToNoise)
	{
		// Alter noise scale based on LandscapeScale; use log so the scale does not increase linearly.
		// A linearly increasing scale results in excessively high values.
		const float LandscapeScale = Preset->LandscapeScale;
		if (LandscapeScale > 0.0f)
		{
			NoiseScale = FMath::LogX(25.0f, LandscapeScale) + 1.0f;
		}
	}

	const float StandardNoiseOffset = Preset->StandardNoiseOffset * NoiseScale;

	// PlainNoiseOffset is the first pair drawn from the stream — must match component order
	PlainNoiseOffset.X = Stream.FRandRange(-StandardNoiseOffset, StandardNoiseOffset);
	PlainNoiseOffset.Y = Stream.FRandRange(-StandardNoiseOffset, StandardNoiseOffset);

	// HeightMap conversion constants (same formula as OCGMapGenerateComponent::Initialize)
	LandscapeZScale = (Preset->MaxHeight - Preset->MinHeight) * 0.001953125f;
	const float AbsMaxHeight = FMath::Abs(Preset->MaxHeight);
	const float AbsMinHeight = FMath::Abs(Preset->MinHeight);
	const float AbsOffset    = FMath::Abs(AbsMaxHeight - AbsMinHeight) / 2.0f;
	ZOffset = (AbsMaxHeight < AbsMinHeight) ? -AbsOffset : AbsOffset;
}

float UOCGDefaultTemperatureStrategy::HeightMapToWorldHeight(const uint16 Height) const
{
	// Add ZOffset to return actual world height;
	// ZOffset is 0 if the absolute values of MaxHeight and MinHeight are equal.
	return (Height - 32768.0f) * LandscapeZScale / 128.0f + ZOffset;
}
