// Copyright (c) 2025-2026 Code1133. All rights reserved.
#include "Strategies/OCGDefaultErosionStrategy.h"

#include "Data/MapPreset.h"
#include "Data/OCGWorldDataContainer.h"
#include "OCGStats.h"

void UOCGDefaultErosionStrategy::ApplyErosion(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer)
{
	SCOPE_CYCLE_COUNTER(STAT_OCG_ErosionPass);

	if (Preset->ErosionSettings.NumErosionIterations <= 0 || !Preset->ErosionSettings.bErosion)
	{
		return;
	}

	// 1. Initialize erosion brush which generates brush according to erosion radius
	Initialize(Preset);
	InitializeErosionBrush(Preset);

	// 2. Change Height Map from uint16 to world height float
	TArray<float> HeightMapFloat;
	HeightMapFloat.SetNumUninitialized(DataContainer.HeightMapData.Num());
	for (int32 i = 0; i < DataContainer.HeightMapData.Num(); ++i)
	{
		HeightMapFloat[i] = HeightConverter.ToWorldHeight(DataContainer.HeightMapData[i]);
	}

	const float SeaLevelHeight = FOCGHeightConverter::GetSeaLevelWorldHeight(Preset);
	const float LandscapeScale = Preset->LandscapeSettings.LandscapeScale * 100.0f;

	// 3. Main Erosion loop
	for (int32 i = 0; i < Preset->ErosionSettings.NumErosionIterations; ++i)
	{
		// Initialize droplet
		float PosX = Stream.RandRange(1.0f, Preset->LandscapeSettings.MapResolution.X - 2.0f);
		float PosY = Stream.RandRange(1.0f, Preset->LandscapeSettings.MapResolution.Y - 2.0f);
		float DirX = 0.0f;
		float DirY = 0.0f;
		float Speed = Preset->ErosionSettings.InitialSpeed;
		float Water = Preset->ErosionSettings.InitialWaterVolume;
		float Sediment = 0.0f;

		// Simulate droplet
		for (int32 Lifetime = 0; Lifetime < Preset->ErosionSettings.MaxDropletLifetime; ++Lifetime)
		{
			const int32 NodeX = static_cast<int32>(PosX);
			const int32 NodeY = static_cast<int32>(PosY);
			const int32 DropletIndex = NodeY * Preset->LandscapeSettings.MapResolution.X + NodeX;

			// If droplet is out of map, end simulation
			if (NodeX < 0 || NodeX >= Preset->LandscapeSettings.MapResolution.X - 1 || NodeY < 0 || NodeY >= Preset->LandscapeSettings.MapResolution.Y - 1)
			{
				break;
			}

			// Calculate current pixel's height and gradient
			FVector2D Gradient;
			const float CurrentHeight = CalculateHeightAndGradient(Preset, HeightMapFloat, LandscapeScale, PosX, PosY, Gradient);

			// Apply inertia and calculate droplet's direction
			DirX = (DirX * Preset->ErosionSettings.DropletInertia) - (Gradient.X * (1.0f - Preset->ErosionSettings.DropletInertia));
			DirY = (DirY * Preset->ErosionSettings.DropletInertia) - (Gradient.Y * (1.0f - Preset->ErosionSettings.DropletInertia));

			// Normalize direction
			const float Len = FMath::Sqrt(DirX * DirX + DirY * DirY);
			if (Len > KINDA_SMALL_NUMBER)
			{
				DirX /= Len;
				DirY /= Len;
			}

			// Move to new pixel using calculated direction
			PosX += DirX;
			PosY += DirY;

			if (PosX <= 0 || PosX >= Preset->LandscapeSettings.MapResolution.X - 1 ||
				PosY <= 0 || PosY >= Preset->LandscapeSettings.MapResolution.Y - 1)
			{
				break;
			}

			// Calculate height and gradient from new pixel
			const float NewHeight = CalculateHeightAndGradient(Preset, HeightMapFloat, LandscapeScale, PosX, PosY, Gradient);
			if (NewHeight <= SeaLevelHeight)
			{
				break;
			}
			const float HeightDifference = NewHeight - CurrentHeight;

			// Calculate sediment capacity
			const float SedimentCapacity = FMath::Max(-HeightDifference * Speed * Water * Preset->ErosionSettings.SedimentCapacityFactor, Preset->ErosionSettings.MinSedimentCapacity);

			// Apply sediment or erosion
			if (Sediment > SedimentCapacity || HeightDifference > 0)
			{
				// Sediment: deposit carried material
				const float AmountToDeposit = (HeightDifference > 0)
					? FMath::Min(Sediment, HeightDifference)
					: (Sediment - SedimentCapacity) * Preset->ErosionSettings.DepositSpeed;
				Sediment -= AmountToDeposit;

				// Apply sediment using pre-calculated erosion brush
				const TArray<int32>& Indices = ErosionBrushIndices[DropletIndex];
				const TArray<float>& Weights = ErosionBrushWeights[DropletIndex];
				for (int32 j = 0; j < Indices.Num(); ++j)
				{
					HeightMapFloat[Indices[j]] += AmountToDeposit * Weights[j];
				}
			}
			else
			{
				// Erosion: pick up material from terrain
				const float AmountToErode = FMath::Min((SedimentCapacity - Sediment), -HeightDifference) * Preset->ErosionSettings.ErodeSpeed;

				// Apply erosion using pre-calculated erosion brush
				const TArray<int32>& Indices = ErosionBrushIndices[DropletIndex];
				const TArray<float>& Weights = ErosionBrushWeights[DropletIndex];
				for (int32 j = 0; j < Indices.Num(); ++j)
				{
					HeightMapFloat[Indices[j]] -= AmountToErode * Weights[j];
				}
				Sediment += AmountToErode;
			}

			// Update droplet's water amount and speed
			Speed = FMath::Sqrt(FMath::Max(0.0f, Speed * Speed - HeightDifference * Preset->ErosionSettings.Gravity));
			Water *= (1.0f - Preset->ErosionSettings.EvaporateSpeed);
		}
	}

	const uint16 SeaHeight = HeightConverter.ToHeightMapValue(SeaLevelHeight);

	// 4. Change world height map to height map (uint16)
	for (int32 i = 0; i < HeightMapFloat.Num(); ++i)
	{
		// Erase height higher than original height map due to sediment
		const uint16 Height = HeightConverter.ToHeightMapValue(HeightMapFloat[i]);
		uint16 NewHeight = FMath::Min(Height, DataContainer.HeightMapData[i]);

		// Prevent height from going under sea level due to erosion
		if (DataContainer.HeightMapData[i] >= SeaHeight)
		{
			NewHeight = FMath::Max(NewHeight, SeaHeight);
		}

		DataContainer.HeightMapData[i] = FMath::Clamp(NewHeight, (uint16)0, (uint16)65535);
	}
}

void UOCGDefaultErosionStrategy::Initialize(const UMapPreset* Preset)
{
	Stream.Initialize(Preset->Seed);
	HeightConverter.Initialize(Preset);
}

void UOCGDefaultErosionStrategy::InitializeErosionBrush(const UMapPreset* Preset)
{
	SCOPE_CYCLE_COUNTER(STAT_OCG_ErosionBrushInit);

	const int32 NewSize = Preset->LandscapeSettings.MapResolution.X * Preset->LandscapeSettings.MapResolution.Y;

	// if current erosion brush is initialized with current map resolution and erosion radius
	if (CachedErosionRadius == Preset->ErosionSettings.ErosionRadius && ErosionBrushIndices.Num() == NewSize)
	{
		return;
	}

	ErosionBrushIndices.Empty();
	ErosionBrushWeights.Empty();
	ErosionBrushIndices.SetNum(NewSize);
	ErosionBrushWeights.SetNum(NewSize);

	for (int32 i = 0; i < NewSize; ++i)
	{
		const int32 CenterX = i % Preset->LandscapeSettings.MapResolution.X;
		const int32 CenterY = i / Preset->LandscapeSettings.MapResolution.X;

		float WeightSum = 0.0f;
		TArray<int32>& Indices = ErosionBrushIndices[i];
		TArray<float>& Weights = ErosionBrushWeights[i];

		for (int32 BrushY = -Preset->ErosionSettings.ErosionRadius; BrushY <= Preset->ErosionSettings.ErosionRadius; ++BrushY)
		{
			for (int32 BrushX = -Preset->ErosionSettings.ErosionRadius; BrushX <= Preset->ErosionSettings.ErosionRadius; ++BrushX)
			{
				const float Dist = FMath::Sqrt(static_cast<float>(BrushX * BrushX + BrushY * BrushY));
				if (Dist <= Preset->ErosionSettings.ErosionRadius)
				{
					const int32 CoordX = CenterX + BrushX;
					const int32 CoordY = CenterY + BrushY;
					if (CoordX >= 0 && CoordX < Preset->LandscapeSettings.MapResolution.X && CoordY >= 0 && CoordY < Preset->LandscapeSettings.MapResolution.Y)
					{
						int32 Index = CoordY * Preset->LandscapeSettings.MapResolution.X + CoordX;
						Index = FMath::Clamp(Index, 0, NewSize - 1);
						const float Weight = 1.0f - (Dist / Preset->ErosionSettings.ErosionRadius);
						WeightSum += Weight;
						Indices.Add(Index);
						Weights.Add(Weight);
					}
				}
			}
		}

		// Normalize Weights so that weight sum at any index is 1
		if (WeightSum > 0.0f)
		{
			for (int32 j = 0; j < Weights.Num(); ++j)
			{
				Weights[j] /= WeightSum;
			}
		}
	}

	CachedErosionRadius = Preset->ErosionSettings.ErosionRadius;
}

float UOCGDefaultErosionStrategy::CalculateHeightAndGradient(const UMapPreset* Preset, const TArray<float>& HeightMap,
	float LandscapeScale, float PosX, float PosY, FVector2D& OutGradient) const
{
	const int32 CoordX = static_cast<int32>(PosX);
	const int32 CoordY = static_cast<int32>(PosY);

	// Fractional offsets within the cell (0 to 1)
	const float FracX = PosX - CoordX;
	const float FracY = PosY - CoordY;

	// 4 close indices
	const int32 Index_00 = CoordY * Preset->LandscapeSettings.MapResolution.X + CoordX;    // Closest index
	const int32 Index_10 = Index_00 + 1;                                   // Index of pixel at right
	const int32 Index_01 = Index_00 + Preset->LandscapeSettings.MapResolution.X;            // Index of pixel at bottom
	const int32 Index_11 = Index_01 + 1;                                   // Index of pixel at bottom right

	// Heights at each index
	const float Height_00 = HeightMap[Index_00];
	const float Height_10 = HeightMap[Index_10];
	const float Height_01 = HeightMap[Index_01];
	const float Height_11 = HeightMap[Index_11];

	// Calculate Gradient
	OutGradient.X = ((Height_10 - Height_00) * (1.0f - FracY) + (Height_11 - Height_01)) / LandscapeScale;
	OutGradient.Y = ((Height_01 - Height_00) * (1.0f - FracX) + (Height_11 - Height_10)) / LandscapeScale;

	// Calculate and return bilinear interpolated height
	return Height_00 * (1.0f - FracX) * (1.0f - FracY)
	     + Height_10 * FracX           * (1.0f - FracY)
	     + Height_01 * (1.0f - FracX) * FracY
	     + Height_11 * FracX           * FracY;
}
