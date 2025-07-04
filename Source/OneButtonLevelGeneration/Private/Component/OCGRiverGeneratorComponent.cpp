// Copyright (c) 2025 Code1133. All rights reserved.


#include "Component/OCGRiverGeneratorComponent.h"

#include "Landscape.h"
#include "Components/SplineComponent.h"
#include "Data/MapPreset.h"


UOCGRiverGeneratorComponent::UOCGRiverGeneratorComponent()
{
}

void UOCGRiverGeneratorComponent::GenerateRiver(UWorld* InWorld, ALandscape* InLandscape)
{
	if (!InWorld || !InLandscape || !MapPreset)
	{
		UE_LOG(LogTemp, Warning, TEXT("River generation failed: Invalid world or landscape or map preset."));
		return;
	}

	if (!MapPreset->bGenerateRiver)
	{
		return;
	}
	
	FIntPoint MapResolution = MapPreset->MapResolution;
	FIntPoint StartPoint = GetRandomStartPoint(MapPreset->RiverStartPointThresholdMultiplier);

	TSet<FIntPoint> Visited;
	FIntPoint Current = StartPoint;
	TArray<FVector> RiverPath;
 
	while (!Visited.Contains(Current))
	{
		Visited.Add(Current);
		int32 Index = Current.Y * MapResolution.X + Current.X;

		float Height = HeightMapData[Index];

		FVector WorldLocation = GetWorldLocation(Current, InLandscape);
		RiverPath.Add(WorldLocation);
		UE_LOG(LogTemp, Log, TEXT("River Point: %s at Height: %f"), *WorldLocation.ToString(), WorldLocation.Z);

		if (WorldLocation.Z <= SeaHeight)
		{
			break; // Stop if we reach sea level
		}

		float MinHeight = Height;
		FIntPoint Next = Current;
		for (int dx = -1; dx <=1; ++dx)
		{
			for (int dy = -1; dy <= 1; ++dy)
			{
				if (dx == 0 && dy == 0) continue;
				FIntPoint Neighbor = FIntPoint(Current.X + dx, Current.Y + dy);
				if (Neighbor.X < 0 || Neighbor.X >= MapResolution.X || Neighbor.Y < 0 || Neighbor.Y >= MapResolution.Y)
					continue;
				int32 nIdx = Neighbor.Y * MapResolution.X + Neighbor.X;
				float NeighborHeight = HeightMapData[nIdx];
				if (NeighborHeight < MinHeight)
				{
					MinHeight = NeighborHeight;
					Next = Neighbor;
				}
			}
		}
		if (Next == Current) break; // No lower neighbor found, stop the river generation
		Current = Next;
	}

	if (!RiverSpline)
	{
		RiverSpline = NewObject<USplineComponent>(this, USplineComponent::StaticClass(), TEXT("RiverSpline"));
	}
	for (int32 i = 0; i < RiverPath.Num(); ++i)
	{
		RiverSpline->AddSplinePoint(RiverPath[i], ESplineCoordinateSpace::World);
	}

	// !TODO : WaterPlugin의 WaterVolumeRiver 추가
}

void UOCGRiverGeneratorComponent::SetMapData(const TArray<uint16>& InHeightMap, UMapPreset* InMapPreset)
{
	HeightMapData = InHeightMap;
	MapPreset = InMapPreset;
}

void UOCGRiverGeneratorComponent::SpawnWaterRiver()
{
}

FVector UOCGRiverGeneratorComponent::GetWorldLocation(const FIntPoint& MapPoint, ALandscape* InLandscape) const
{
	if (!InLandscape || !MapPreset)
	{
		return FVector::ZeroVector;
	}

	// Convert MapPoint to world coordinates
	FBox LandscapeBounds = InLandscape->GetComponentsBoundingBox();
	FVector LandscapeOrigin = LandscapeBounds.GetCenter();
	FVector LandscapeExtent = LandscapeBounds.GetSize() / 2.0f;
	FVector WorldLocation = LandscapeOrigin + FVector(
		(MapPoint.X / (float)MapPreset->MapResolution.X) * LandscapeExtent.X,
		(MapPoint.Y / (float)MapPreset->MapResolution.Y) * LandscapeExtent.Y,
		0.0f 
	);

	TOptional<float> Height = InLandscape->GetHeightAtLocation(WorldLocation);
	if (Height)
	{
		WorldLocation.Z = Height.GetValue();
	}

	return WorldLocation;
}

FIntPoint UOCGRiverGeneratorComponent::GetRandomStartPoint(float StartPointThresholdMultiplier)
{
	if (!MapPreset)
	{
		UE_LOG(LogTemp, Error, TEXT("MapPreset is not set. Cannot generate random start point for river."));
		return FIntPoint(0, 0);
	}
	// Ensure the multiplier is within a reasonable range
	StartPointThresholdMultiplier = FMath::Clamp(StartPointThresholdMultiplier, 0.5f, 1.0f);
	
	TArray<FIntPoint> HighPoints;
	uint16 HighThreshold = SeaHeight + (MapPreset->MaxHeight - SeaHeight) * StartPointThresholdMultiplier;
	for (int32 y = 0; y < MapPreset->MapResolution.Y; ++y)
	{
		for (int32 x = 0; x < MapPreset->MapResolution.X; ++x)
		{
			int32 idx = y * MapPreset->MapResolution.X + x;
			if (HeightMapData[idx] > HighThreshold)
				HighPoints.Add(FIntPoint(x, y));
		}
	}

	// Select a random high point or default to the center of the map
	FIntPoint StartPoint = HighPoints.Num() > 0 ?
		HighPoints[FMath::RandRange(0, HighPoints.Num() - 1)] :
		FIntPoint(MapPreset->MapResolution.X / 2, MapPreset->MapResolution.Y - 1);

	return StartPoint;
}


