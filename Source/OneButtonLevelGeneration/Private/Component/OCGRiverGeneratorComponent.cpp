// Copyright (c) 2025 Code1133. All rights reserved.


#include "Component/OCGRiverGeneratorComponent.h"

#include "Landscape.h"
#include "OCGLevelGenerator.h"
#include "WaterBodyRiverActor.h"
#include "WaterBodyRiverComponent.h"
#include "WaterEditorSettings.h"
#include "WaterSplineComponent.h"
#include "Component/OCGLandscapeGenerateComponent.h"
#include "Components/SplineComponent.h"
#include "Data/MapPreset.h"
#include "Kismet/GameplayStatics.h"


UOCGRiverGeneratorComponent::UOCGRiverGeneratorComponent()
{
}

void UOCGRiverGeneratorComponent::GenerateRiver(UWorld* InWorld, ALandscape* InLandscape)
{
	TargetLandscape = InLandscape;
	// Clear previously generated rivers
	for (AWaterBodyRiver* River : GeneratedRivers)
	{
		if (River)
		{
			River->Destroy();
		}
	}

	// Clear WaterBrushManager actors
	UClass* WaterBrushManagerClass = StaticLoadClass(UObject::StaticClass(), nullptr, TEXT("/Script/WaterEditor.WaterBrushManager"));
	
	if (WaterBrushManagerClass)
	{
		TArray<AActor*> WaterBrushManagerActors;
		UGameplayStatics::GetAllActorsOfClass(InWorld, WaterBrushManagerClass, WaterBrushManagerActors);

		for (AActor* Actor : WaterBrushManagerActors)
		{
			if (Actor)
			{
				Actor->Destroy();
			}
		}
	}
	
	if (!InWorld || !InLandscape || !MapPreset)
	{
		UE_LOG(LogTemp, Warning, TEXT("River generation failed: Invalid world or landscape or map preset."));
		return;
	}

	if (!MapPreset->bGenerateRiver)
	{
		return;
	}

	FVector LandscapeOrigin = InLandscape->GetActorLocation();
	FVector LandscapeExtent = InLandscape->GetComponentsBoundingBox().GetSize() * 0.5f;

	// Generate River Spline
	for (int RiverCount = 0; RiverCount < MapPreset->RiverCount; RiverCount++)
	{
		FIntPoint MapResolution = MapPreset->MapResolution;
		FIntPoint StartPoint = GetRandomStartPoint(MapPreset->RiverStartPointThresholdMultiplier, InLandscape);

		TSet<FIntPoint> VisitedNodes;
		TMap<FIntPoint, FIntPoint> CameFrom;
		TMap<FIntPoint, float> CostSoFar;

		TArray<TTuple<FIntPoint, float>> Frontier;
		Frontier.Add({StartPoint, 0.f});
		
		CameFrom.Add(StartPoint, StartPoint);
		CostSoFar.Add(StartPoint, 0.f);

		FIntPoint GoalPoint = FIntPoint(-1, -1);

		while (Frontier.Num() > 0)
		{
			int32 BestIndex = 0;
			for (int32 i = 1; i < Frontier.Num(); ++i)
			{
				if (Frontier[i].Get<1>() < Frontier[BestIndex].Get<1>())
				{
					BestIndex = i;
				}
			}

			TTuple<FIntPoint, float> BestNode = Frontier[BestIndex];
			Frontier.RemoveAt(BestIndex);

			FIntPoint Current = BestNode.Get<0>();
			
			if (GetLandscapePointWorldPosition(Current, LandscapeOrigin, LandscapeExtent).Z <= SeaHeight)
			{
				GoalPoint = Current;
				break;
			}
			
			for (int dx = -1; dx <= 1; ++dx)
			{
				for (int dy = -1; dy <= 1; ++dy)
				{
					if (dx == 0 && dy == 0) continue;
            
					FIntPoint Neighbor = FIntPoint(Current.X + dx, Current.Y + dy);
					if (Neighbor.X < 0 || Neighbor.X >= MapResolution.X || Neighbor.Y < 0 || Neighbor.Y >= MapResolution.Y) continue;

					float NewCost = CostSoFar[Current] + 1; 
            
					if (!CostSoFar.Contains(Neighbor) || NewCost < CostSoFar[Neighbor])
					{
						CostSoFar.Add(Neighbor, NewCost);
						int32 nIdx = Neighbor.Y * MapResolution.X + Neighbor.X;
						float Heuristic = HeightMapData[nIdx] - SeaHeight; 
						float NewPriority = NewCost + Heuristic;
						//float NewPriority = HeightMapData[nIdx];
						Frontier.Add({Neighbor, NewPriority});
						CameFrom.Add(Neighbor, Current);
					}
				}
			}
		}
		
		if (GoalPoint != FIntPoint(-1, -1))
		{
			TArray<FVector> RiverPath;
			FIntPoint Current = GoalPoint;
			while (Current != StartPoint)
			{
				RiverPath.Add(GetLandscapePointWorldPosition(Current, LandscapeOrigin, LandscapeExtent));
				Current = CameFrom[Current];
			}
			RiverPath.Add(GetLandscapePointWorldPosition(StartPoint, LandscapeOrigin, LandscapeExtent));
			Algo::Reverse(RiverPath);
			
			TArray<FVector> SimplifiedRiverPath;
			SimplifyPathRDP(RiverPath, SimplifiedRiverPath, MapPreset->RiverSpineSimplifyEpsilon);

			// Generate AWaterBodyRiver Actor
			FVector WaterBodyPos = GetLandscapePointWorldPosition(StartPoint, LandscapeOrigin, LandscapeExtent);
			FTransform WaterBodyTransform = FTransform(WaterBodyPos);
			AWaterBodyRiver* WaterBodyRiver = InWorld->SpawnActor<AWaterBodyRiver>(AWaterBodyRiver::StaticClass(), WaterBodyTransform);

			TArray<AActor*> FoundActors;
			UGameplayStatics::GetAllActorsOfClass(InWorld, AWaterZone::StaticClass(), FoundActors);

			FVector LandscapeSize = InLandscape->GetComponentsBoundingBox().GetSize();
			for (AActor* Actor : FoundActors)
			{
				if (AWaterZone* WaterZone = Cast<AWaterZone>(Actor))
				{
					// 2. WaterZone의 ZoneExtent 프로퍼티를 랜드스케이프의 X, Y 크기로 설정합니다.
					WaterZone->SetZoneExtent(FVector2D(LandscapeSize.X, LandscapeSize.Y));
				}
			}
			SetRiverWidth(WaterBodyRiver, SimplifiedRiverPath);
			SetDefaultRiverProperties(WaterBodyRiver, SimplifiedRiverPath);
			GeneratedRivers.Add(WaterBodyRiver);
		}
	}
}

void UOCGRiverGeneratorComponent::SetMapData(const TArray<uint16>& InHeightMap, UMapPreset* InMapPreset, float InMinHeight, float InMaxHeight)
{
	HeightMapData = InHeightMap;
	MapPreset = InMapPreset;
	SeaHeight = MapPreset->MinHeight + 
		(MapPreset->MaxHeight - MapPreset->MinHeight) * MapPreset->SeaLevel - 1;
}

void UOCGRiverGeneratorComponent::SetRiverWidth(AWaterBodyRiver* InRiverActor, const TArray<FVector>& InRiverPath)
{
	// !TODO : MapPreset에 RiverWidth Graph를 추가, 해당 Graph기반으로 강의 폭 설정
	if (!InRiverActor || InRiverPath.Num() < 2)
	{
		return;
	}

	UCurveFloat* RiverWidthCurve = MapPreset ? MapPreset->RiverWidthCurve : nullptr;
	UWaterSplineMetadata* SplineMetadata = Cast<UWaterSplineMetadata>(InRiverActor->GetWaterBodyComponent()->GetWaterSpline()->GetSplinePointsMetadata());
	
	if (!SplineMetadata || !RiverWidthCurve)
	{
		return;
	}

	const int32 NumPoints = InRiverPath.Num();

	SplineMetadata->RiverWidth.Points.SetNum(NumPoints);
	for (int32 i = 0; i < NumPoints; ++i)
	{
		const float NormalizedDistance = (NumPoints > 1) ? static_cast<float>(i) / (NumPoints - 1) : 0.0f;
		const float DesiredWidth = RiverWidthCurve->GetFloatValue(NormalizedDistance);

		SplineMetadata->RiverWidth.Points[i].InVal = NormalizedDistance;
		SplineMetadata->RiverWidth.Points[i].OutVal = DesiredWidth;
	}
	FOnWaterBodyChangedParams Params;
	Params.bShapeOrPositionChanged = true; // Spline 등 Shape이 바뀌었음을 명시
	Params.bUserTriggered = true;          // (선택) 사용자 직접 트리거
	
	InRiverActor->GetWaterBodyComponent()->UpdateAll(Params);
}

FVector UOCGRiverGeneratorComponent::GetLandscapePointWorldPosition(const FIntPoint& MapPoint, const FVector& LandscapeOrigin, const FVector& LandscapeExtent) const
{
	if (!MapPreset)
	{
		return FVector::ZeroVector;
	}

	
	FVector WorldLocation = LandscapeOrigin + FVector(
		2 * (MapPoint.X / (float)MapPreset->MapResolution.X) * LandscapeExtent.X,
		2 * (MapPoint.Y / (float)MapPreset->MapResolution.Y) * LandscapeExtent.Y ,
		0.0f 
	);

	int Index = MapPoint.Y * MapPreset->MapResolution.X + MapPoint.X;
	float HeightMapValue = HeightMapData[Index];

	TOptional<float> Height = TargetLandscape->GetHeightAtLocation(WorldLocation);
	if (Height.IsSet())
	{
		WorldLocation.Z = Height.GetValue();
	}
	else
	{
		WorldLocation.Z = (HeightMapValue - 32768) / 128 * 100 * MapPreset->LandscapeScale; // Default to 0 if height is not set
	}
	return WorldLocation;
}

void UOCGRiverGeneratorComponent::SetDefaultRiverProperties(class AWaterBodyRiver* InRiverActor, const TArray<FVector>& InRiverPath)
{
	UWaterBodyComponent* WaterBodyComponent = CastChecked<AWaterBody>(InRiverActor)->GetWaterBodyComponent();
	check(WaterBodyComponent);

	if (const FWaterBrushActorDefaults* WaterBrushActorDefaults = &GetDefault<UWaterEditorSettings>()->WaterBodyRiverDefaults.BrushDefaults)
	{
		WaterBodyComponent->CurveSettings = WaterBrushActorDefaults->CurveSettings;
		WaterBodyComponent->WaterHeightmapSettings = WaterBrushActorDefaults->HeightmapSettings;
		WaterBodyComponent->LayerWeightmapSettings = WaterBrushActorDefaults->LayerWeightmapSettings;
	}

	if (const FWaterBodyDefaults* WaterBodyDefaults = &GetDefault<UWaterEditorSettings>()->WaterBodyRiverDefaults)
	{
		WaterBodyComponent->SetWaterMaterial(WaterBodyDefaults->GetWaterMaterial());
		WaterBodyComponent->SetWaterStaticMeshMaterial(WaterBodyDefaults->GetWaterStaticMeshMaterial());
		WaterBodyComponent->SetHLODMaterial(WaterBodyDefaults->GetWaterHLODMaterial());
		WaterBodyComponent->SetUnderwaterPostProcessMaterial(WaterBodyDefaults->GetUnderwaterPostProcessMaterial());

		UWaterSplineComponent* WaterSpline = WaterBodyComponent->GetWaterSpline();
		WaterSpline->WaterSplineDefaults = WaterBodyDefaults->SplineDefaults;
	}

	// If the water body is spawned into a zone which is using local only tessellation, we must default to enabling static meshes.
	if (const AWaterZone* WaterZone = WaterBodyComponent->GetWaterZone())
	{
		if (WaterZone->IsLocalOnlyTessellationEnabled())
		{
			WaterBodyComponent->SetWaterBodyStaticMeshEnabled(true);
		}
	}
	
	UWaterBodyRiverComponent* WaterBodyRiverComponent = CastChecked<UWaterBodyRiverComponent>(InRiverActor->GetWaterBodyComponent());
	WaterBodyRiverComponent->SetLakeTransitionMaterial(GetDefault<UWaterEditorSettings>()->WaterBodyRiverDefaults.GetRiverToLakeTransitionMaterial());
	WaterBodyRiverComponent->SetOceanTransitionMaterial(GetDefault<UWaterEditorSettings>()->WaterBodyRiverDefaults.GetRiverToOceanTransitionMaterial());

	InRiverActor->PostEditChange();
	InRiverActor->PostEditMove(true);
	
	UWaterSplineComponent* WaterSpline = InRiverActor->GetWaterBodyComponent()->GetWaterSpline();
	WaterSpline->ClearSplinePoints();
	WaterSpline->SetSplinePoints(InRiverPath, ESplineCoordinateSpace::World, true);

	FOnWaterBodyChangedParams Params;
	Params.bShapeOrPositionChanged = true; // Spline 등 Shape이 바뀌었음을 명시
	Params.bUserTriggered = true;          // (선택) 사용자 직접 트리거

	InRiverActor->GetWaterBodyComponent()->GetWaterSpline()->GetSplinePointsMetadata();
	InRiverActor->GetWaterBodyComponent()->UpdateAll(Params); // 또는 UpdateAll()

	InRiverActor->GetWaterBodyComponent()->UpdateWaterBodyRenderData();
}

FIntPoint UOCGRiverGeneratorComponent::GetRandomStartPoint(float StartPointThresholdMultiplier, ALandscape* InLandscape)
{
	AOCGLevelGenerator* LevelGenerator = Cast<AOCGLevelGenerator>(GetOwner());
	
	if (!MapPreset || !LevelGenerator)
	{
		UE_LOG(LogTemp, Error, TEXT("MapPreset is not set. Cannot generate random start point for river."));
		return FIntPoint(0, 0);
	}

	const TArray<FIntPoint>& HighPoints = LevelGenerator->GetLandscapeGenerateComponent()->GetCachedRiverStartPoints();
	// Select a random high point or default to the center of the map
	FIntPoint StartPoint = HighPoints.Num() > 0 ?
		HighPoints[FMath::RandRange(0, HighPoints.Num() - 1)] :
		FIntPoint(MapPreset->MapResolution.X / 2, MapPreset->MapResolution.Y - 1);

	return StartPoint;
}

void UOCGRiverGeneratorComponent::SimplifyPathRDP(const TArray<FVector>& InPoints, TArray<FVector>& OutPoints,
	float Epsilon)
{
	if (InPoints.Num() < 3)
	{
		OutPoints = InPoints;
		return;
	}

	float dmax = 0.0f;
	int32 index = 0;
	int32 end = InPoints.Num() - 1;

	for (int32 i = 1; i < end; i++)
	{
		float d = FMath::PointDistToSegment(InPoints[i], InPoints[0], InPoints[end]);
		if (d > dmax)
		{
			index = i;
			dmax = d;
		}
	}

	if (dmax > Epsilon)
	{
		TArray<FVector> RecResults1;
		TArray<FVector> RecResults2;
        
		TArray<FVector> FirstHalf(InPoints.GetData(), index + 1);
		SimplifyPathRDP(FirstHalf, RecResults1, Epsilon);

		TArray<FVector> SecondHalf(InPoints.GetData() + index, InPoints.Num() - index);
		SimplifyPathRDP(SecondHalf, RecResults2, Epsilon);

		OutPoints.Append(RecResults1.GetData(), RecResults1.Num() - 1);
		OutPoints.Append(RecResults2);
	}
	else
	{
		OutPoints.Add(InPoints[0]);
		OutPoints.Add(InPoints[end]);
	}
}


