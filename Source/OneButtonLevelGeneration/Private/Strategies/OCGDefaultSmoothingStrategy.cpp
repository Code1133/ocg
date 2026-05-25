// Copyright (c) 2025-2026 Code1133. All rights reserved.
#include "Strategies/OCGDefaultSmoothingStrategy.h"

#include "Data/MapPreset.h"
#include "Data/OCGWorldDataContainer.h"
#include "OCGStats.h"

void UOCGDefaultSmoothingStrategy::SmoothHeightMap(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer)
{
	if (!Preset->bSmoothHeight)
	{
		return;
	}

	Initialize(Preset);
	ApplySpikeSmooth(Preset, DataContainer.HeightMapData);

	// TODO: OutBlurredMap은 호출부에서 사용되지 않음. ApplyGaussianBlur 내부 지역 변수로 이동하고 파라미터 제거 고려
	TArray<uint16> BlurredHeightMap;
	ApplyGaussianBlur(Preset, DataContainer.HeightMapData, BlurredHeightMap);

	MedianSmooth(Preset, DataContainer.HeightMapData);
}

void UOCGDefaultSmoothingStrategy::Initialize(const UMapPreset* Preset)
{
	LandscapeZScale = (Preset->MaxHeight - Preset->MinHeight) * 0.001953125f;
	const float AbsMaxHeight = FMath::Abs(Preset->MaxHeight);
	const float AbsMinHeight = FMath::Abs(Preset->MinHeight);
	const float AbsOffset    = FMath::Abs(AbsMaxHeight - AbsMinHeight) / 2.0f;
	ZOffset = (AbsMaxHeight < AbsMinHeight) ? -AbsOffset : AbsOffset;
}

void UOCGDefaultSmoothingStrategy::ApplyGaussianBlur(const UMapPreset* Preset, TArray<uint16>& InOutHeightMap,
	TArray<uint16>& OutBlurredMap)
{
	SCOPE_CYCLE_COUNTER(STAT_OCG_SmoothGaussian);

	const int32 Radius = Preset->GaussianBlurRadius;
	const FIntPoint MapSize = Preset->MapResolution;
	const int32 TotalPixels = MapSize.X * MapSize.Y;

	OutBlurredMap.SetNumUninitialized(TotalPixels);

	TArray<float> TempMap;
	TempMap.SetNumUninitialized(TotalPixels);

	// Horizontal pass
	for (int32 Y = 0; Y < MapSize.Y; ++Y)
	{
		for (int32 X = 0; X < MapSize.X; ++X)
		{
			float Sum = 0.0f;
			float WeightSum = 0.0f;
			for (int32 I = -Radius; I <= Radius; ++I)
			{
				const int32 SampleX = FMath::Clamp(X + I, 0, MapSize.X - 1);
				const int32 Index = Y * MapSize.X + SampleX;
				const float Weight = FMath::Exp(-(I * I) / (2.0f * Radius * Radius));
				Sum += InOutHeightMap[Index] * Weight;
				WeightSum += Weight;
			}
			TempMap[Y * MapSize.X + X] = Sum / WeightSum;
		}
	}

	// Vertical pass
	for (int32 X = 0; X < MapSize.X; ++X)
	{
		for (int32 Y = 0; Y < MapSize.Y; ++Y)
		{
			float Sum = 0.0f;
			float WeightSum = 0.0f;
			for (int32 I = -Radius; I <= Radius; ++I)
			{
				const int32 SampleY = FMath::Clamp(Y + I, 0, MapSize.Y - 1);
				const int32 Index = SampleY * MapSize.X + X;
				const float Weight = FMath::Exp(-(I * I) / (2.0f * Radius * Radius));
				Sum += TempMap[Index] * Weight;
				WeightSum += Weight;
			}
			OutBlurredMap[Y * MapSize.X + X] = static_cast<uint16>(FMath::Clamp(FMath::RoundToInt(Sum / WeightSum), 0, 65535));
		}
	}

	InOutHeightMap = OutBlurredMap;
}

void UOCGDefaultSmoothingStrategy::ApplySpikeSmooth(const UMapPreset* Preset, TArray<uint16>& InOutHeightMap)
{
	SCOPE_CYCLE_COUNTER(STAT_OCG_SmoothSpike);

	if (!Preset->bSmoothBySlope)
	{
		return;
	}

	const FIntPoint MapSize = Preset->MapResolution;
	const int32 KernelRadius = static_cast<int32>(Preset->GaussianBlurRadius / 2.0f);
	const int32 KernelSize = 2 * KernelRadius + 1;
	const float MaxAllowedSlope = FMath::Tan(FMath::DegreesToRadians(Preset->MaxSlopeAngle));
	const int32 Step = FMath::Max(static_cast<int32>(KernelRadius / 2.0f), 1);

	for (int32 Iteration = 0; Iteration < Preset->SmoothingIteration; ++Iteration)
	{
		int32 SmoothedRegion = 0;
		TArray<uint16> OriginalHeightMap = InOutHeightMap;

		for (int32 Y = KernelRadius; Y < MapSize.Y - KernelRadius; Y += Step)
		{
			for (int32 X = KernelRadius; X < MapSize.X - KernelRadius; X += Step)
			{
				ProcessPlane(Preset, X, Y, MapSize, KernelRadius, KernelSize, MaxAllowedSlope, SmoothedRegion, OriginalHeightMap, InOutHeightMap);
			}
		}

		if (SmoothedRegion == 0)
		{
			break;
		}
	}
}

void UOCGDefaultSmoothingStrategy::ProcessPlane(
	const UMapPreset* Preset,
	int32 CenterX,
	int32 CenterY,
	FIntPoint MapSize,
	int32 KernelRadius,
	int32 KernelSize,
	float MaxAllowedSlope,
	int32& SmoothedRegion,
	const TArray<uint16>& InOriginalHeightMap,
	TArray<uint16>& OutHeightMap
)
{
	const float LandscapeScale = Preset->LandscapeScale * 100.0f;
	const float Length = KernelSize * LandscapeScale;

	const float TLHeight = HeightMapToWorldHeight(InOriginalHeightMap[(CenterY - KernelRadius) * MapSize.X + (CenterX - KernelRadius)]);
	const float TRHeight = HeightMapToWorldHeight(InOriginalHeightMap[(CenterY - KernelRadius) * MapSize.X + (CenterX + KernelRadius)]);
	const float BLHeight = HeightMapToWorldHeight(InOriginalHeightMap[(CenterY + KernelRadius) * MapSize.X + (CenterX - KernelRadius)]);
	const float BRHeight = HeightMapToWorldHeight(InOriginalHeightMap[(CenterY + KernelRadius) * MapSize.X + (CenterX + KernelRadius)]);

	const float TopSlope    = (TRHeight - TLHeight) / Length;
	const float BottomSlope = (BRHeight - BLHeight) / Length;
	const float RightSlope  = (BRHeight - TRHeight) / Length;
	const float LeftSlope   = (BLHeight - TLHeight) / Length;

	const float SlopeX = (BottomSlope + TopSlope) / 2.0f;
	const float SlopeY = (RightSlope + LeftSlope) / 2.0f;
	const float CurrentSlope = FMath::Sqrt(SlopeX * SlopeX + SlopeY * SlopeY);

	if (CurrentSlope > MaxAllowedSlope)
	{
		++SmoothedRegion;

		const float AverageHeight = (TLHeight + TRHeight + BRHeight + BLHeight) / 4.0f;
		const FVector Plane = { SlopeX, SlopeY, AverageHeight };
		const float CorrectionFactor = MaxAllowedSlope / CurrentSlope;
		const float CorrectedSlopeX = SlopeX * CorrectionFactor;
		const float CorrectedSlopeY = SlopeY * CorrectionFactor;

		for (int32 KernelY = -KernelRadius; KernelY <= KernelRadius; ++KernelY)
		{
			for (int32 KernelX = -KernelRadius; KernelX <= KernelRadius; ++KernelX)
			{
				const int32 Index = (CenterY + KernelY) * MapSize.X + (CenterX + KernelX);
				const float OriginalWorldHeight = HeightMapToWorldHeight(InOriginalHeightMap[Index]);
				const float CurrentHeight = Plane.X * KernelX * LandscapeScale + Plane.Y * KernelY * LandscapeScale + Plane.Z;
				const float CorrectedHeight = CorrectedSlopeX * KernelX * LandscapeScale + CorrectedSlopeY * KernelY * LandscapeScale + Plane.Z;
				float NewWorldHeight = OriginalWorldHeight + (CorrectedHeight - CurrentHeight);
				NewWorldHeight = FMath::Clamp(NewWorldHeight, Preset->MinHeight + ZOffset, Preset->MaxHeight + ZOffset);

				const uint16 NewHeight = WorldHeightToHeightMap(NewWorldHeight);
				const uint16 OriginalHeight = WorldHeightToHeightMap(OriginalWorldHeight);
				OutHeightMap[Index] = static_cast<uint16>(FMath::Lerp(static_cast<float>(OriginalHeight), static_cast<float>(NewHeight), Preset->SmoothingStrength));
			}
		}
	}
}

void UOCGDefaultSmoothingStrategy::MedianSmooth(const UMapPreset* Preset, TArray<uint16>& InOutHeightMap)
{
	SCOPE_CYCLE_COUNTER(STAT_OCG_SmoothMedian);

	if (!Preset->bSmoothByMediumHeight)
	{
		return;
	}

	const int32 Radius = Preset->MedianSmoothRadius;
	const FIntPoint MapSize = Preset->MapResolution;

	TArray<uint16> OriginalHeightMap = InOutHeightMap;
	TArray<uint16> Window;
	Window.Reserve((Radius * 2 + 1) * (Radius * 2 + 1));

	for (int32 Y = Radius; Y < MapSize.Y - Radius; ++Y)
	{
		for (int32 X = Radius; X < MapSize.X - Radius; ++X)
		{
			Window.Reset();
			for (int32 WindowY = -Radius; WindowY <= Radius; ++WindowY)
			{
				for (int32 WindowX = -Radius; WindowX <= Radius; ++WindowX)
				{
					Window.Add(OriginalHeightMap[(Y + WindowY) * MapSize.X + (X + WindowX)]);
				}
			}
			Window.Sort();
			InOutHeightMap[Y * MapSize.X + X] = Window[Window.Num() / 2];
		}
	}
}

float UOCGDefaultSmoothingStrategy::HeightMapToWorldHeight(const uint16 Height) const
{
	return (Height - 32768.0f) * LandscapeZScale / 128.0f + ZOffset;
}

uint16 UOCGDefaultSmoothingStrategy::WorldHeightToHeightMap(const float Height) const
{
	return static_cast<uint16>((Height - ZOffset) * 128.0f / LandscapeZScale + 32768.0f);
}
