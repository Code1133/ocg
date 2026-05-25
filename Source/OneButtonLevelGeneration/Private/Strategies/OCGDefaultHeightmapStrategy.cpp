// Copyright (c) 2025-2026 Code1133. All rights reserved.
#include "Strategies/OCGDefaultHeightmapStrategy.h"

#include "Data/MapPreset.h"
#include "Data/OCGWorldDataContainer.h"

void UOCGDefaultHeightmapStrategy::GenerateHeightMap(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer)
{
	Initialize(Preset);

	const FIntPoint CurMapResolution = Preset->MapResolution;
	DataContainer.HeightMapData.SetNumUninitialized(CurMapResolution.X * CurMapResolution.Y);

	// Fill Height Map
	for (int32 y = 0; y < CurMapResolution.Y; ++y)
	{
		for (int32 x = 0; x < CurMapResolution.X; ++x)
		{
			// Returns a value in the range [0, 1]
			const float CalculatedHeight = CalculateHeightForCoordinate(Preset, x, y);

			// Convert [0, 1] to [0, 65535], the range of a uint16 heightmap
			const float NormalizedHeight = CalculatedHeight * 65535.0f;

			const uint16 HeightValue = FMath::Clamp(FMath::RoundToInt(NormalizedHeight), 0, 65535);
			DataContainer.HeightMapData[y * CurMapResolution.X + x] = HeightValue;
		}
	}
}

void UOCGDefaultHeightmapStrategy::Initialize(const UMapPreset* Preset)
{
	Stream.Initialize(Preset->Seed);

	NoiseScale = 1.0f;
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

	InitializeNoiseOffsets(Preset);

	// Set plain height to just above sea level; use 0 when there is no water.
	PlainHeight = Preset->bContainWater ? Preset->SeaLevel * 1.005f : 0.0f;
}

void UOCGDefaultHeightmapStrategy::InitializeNoiseOffsets(const UMapPreset* Preset)
{
	const float StandardNoiseOffset = Preset->StandardNoiseOffset * NoiseScale;

	PlainNoiseOffset.X    = Stream.FRandRange(-StandardNoiseOffset, StandardNoiseOffset);
	PlainNoiseOffset.Y    = Stream.FRandRange(-StandardNoiseOffset, StandardNoiseOffset);
	MountainNoiseOffset.X = Stream.FRandRange(-StandardNoiseOffset, StandardNoiseOffset);
	MountainNoiseOffset.Y = Stream.FRandRange(-StandardNoiseOffset, StandardNoiseOffset);
	BlendNoiseOffset.X    = Stream.FRandRange(-StandardNoiseOffset, StandardNoiseOffset);
	BlendNoiseOffset.Y    = Stream.FRandRange(-StandardNoiseOffset, StandardNoiseOffset);
	DetailNoiseOffset.X   = Stream.FRandRange(-StandardNoiseOffset, StandardNoiseOffset);
	DetailNoiseOffset.Y   = Stream.FRandRange(-StandardNoiseOffset, StandardNoiseOffset);
	IslandNoiseOffset.X   = Stream.FRandRange(-StandardNoiseOffset, StandardNoiseOffset);
	IslandNoiseOffset.Y   = Stream.FRandRange(-StandardNoiseOffset, StandardNoiseOffset);
}

float UOCGDefaultHeightmapStrategy::CalculateHeightForCoordinate(const UMapPreset* Preset, const int32 InX, const int32 InY) const
{
	// 1. Use low-frequency noise to generate large-scale continental and mountain outlines (-1 ~ 1)
	const float MountainNoiseX = InX * Preset->ContinentNoiseScale * NoiseScale + MountainNoiseOffset.X;
	const float MountainNoiseY = InY * Preset->ContinentNoiseScale * NoiseScale + MountainNoiseOffset.Y;
	float MountainHeight = FMath::Clamp(FMath::PerlinNoise2D(FVector2D(MountainNoiseX, MountainNoiseY)) * 2.0f, -1.0f, 1.0f);

	// 2. Accumulate octave noise to add high-frequency terrain detail on top of step 1
	float Amplitude = 1.0f;
	float Frequency = 1.0f;
	float TerrainNoise = 0.0f;
	float MaxPossibleAmplitude = 0.0f;

	for (int32 i = 0; i < Preset->Octaves; ++i)
	{
		const float NoiseInputX = (InX * Preset->TerrainNoiseScale * NoiseScale * Frequency) + DetailNoiseOffset.X;
		const float NoiseInputY = (InY * Preset->TerrainNoiseScale * NoiseScale * Frequency) + DetailNoiseOffset.Y;
		TerrainNoise += FMath::PerlinNoise2D(FVector2D(NoiseInputX, NoiseInputY)) * Amplitude; // -1 ~ 1
		MaxPossibleAmplitude += Amplitude;
		Amplitude *= Preset->Persistence;
		Frequency *= Preset->Lacunarity;
	}
	// Normalize terrain noise to [-1, 1] then scale down so detail does not overpower the base shape
	MountainHeight = FMath::Clamp(MountainHeight + (TerrainNoise / MaxPossibleAmplitude) * 0.3f, -1.0f, 1.0f);

	// 3. Generate a blend mask to control the mountain-to-plain ratio (0 ~ 1)
	const float BlendNoiseX = InX * Preset->ContinentNoiseScale * NoiseScale + BlendNoiseOffset.X;
	const float BlendNoiseY = InY * Preset->ContinentNoiseScale * NoiseScale + BlendNoiseOffset.Y;
	float BlendNoise = FMath::PerlinNoise2D(FVector2D(BlendNoiseX, BlendNoiseY)) * 0.5f + 0.5f;

	// Redistribute BlendNoise so mountain and plain regions are more distinct
	if (Preset->RedistributionFactor > 1.0f && BlendNoise > 0.0f && BlendNoise < 1.0f)
	{
		const float PowX   = FMath::Pow(BlendNoise,        Preset->RedistributionFactor);
		const float Pow1_X = FMath::Pow(1.0f - BlendNoise, Preset->RedistributionFactor);
		BlendNoise = PowX / (PowX + Pow1_X);
	}
	BlendNoise = FMath::SmoothStep(0.0f, 1.0f, BlendNoise);

	// 4. Apply blend mask to combine plain and mountain heights; remap mountain from [-1, 1] to [0, 1]
	float Height = FMath::Clamp(FMath::Lerp(PlainHeight, MountainHeight * 0.5f + 0.5f, BlendNoise), 0.0f, 1.0f);

	// 5. Apply island mask so terrain falls off toward the map edges
	if (Preset->bIsland)
	{
		// Normalized distance from the center of the map, perturbed by noise to avoid a perfect circle
		const float Nx = (static_cast<float>(InX) / Preset->MapResolution.X) * 2.0f - 1.0f;
		const float Ny = (static_cast<float>(InY) / Preset->MapResolution.Y) * 2.0f - 1.0f;
		const float Distance = FMath::Sqrt(Nx * Nx + Ny * Ny);

		const float IslandNoiseX = InX * Preset->IslandShapeNoiseScale * NoiseScale + IslandNoiseOffset.X;
		const float IslandNoiseY = InY * Preset->IslandShapeNoiseScale * NoiseScale + IslandNoiseOffset.Y;
		const float CoastlineNoise = FMath::PerlinNoise2D(FVector2D(IslandNoiseX, IslandNoiseY)); // -1 ~ 1

		float IslandMask = 1.0f - (Distance + CoastlineNoise * Preset->IslandShapeNoiseStrength);
		
		// Scale up so that the interior is solidly land, and more land area is visible
		IslandMask *= 3.0f;
		IslandMask = FMath::Clamp(IslandMask, 0.0f, 1.0f);

		// Sharpen the coastline with a falloff exponent, then smooth the transition
		IslandMask = FMath::Pow(IslandMask, Preset->IslandFalloffExponent);
		IslandMask = FMath::SmoothStep(0.0f, 1.0f, IslandMask);
		IslandMask = FMath::Clamp(IslandMask, 0.0f, 1.0f);

		Height = FMath::Clamp(Height * IslandMask, 0.0f, 1.0f);
	}

	return Height;
}
