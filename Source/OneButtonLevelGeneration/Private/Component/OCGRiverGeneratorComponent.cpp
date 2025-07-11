// Copyright (c) 2025 Code1133. All rights reserved.


#include "Component/OCGRiverGeneratorComponent.h"

#include "EngineUtils.h"
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


UOCGRiverGenerateComponent::UOCGRiverGenerateComponent()
{
}

void UOCGRiverGenerateComponent::GenerateRiver(UWorld* InWorld, ALandscape* InLandscape)
{
#if WITH_EDITOR
	if (InWorld == nullptr)
	{
		return;
	}
	TargetLandscape = InLandscape;

	if (TargetLandscape == nullptr)
	{
		// find landscape actor in the world
		for (ALandscape* Actor : TActorRange<ALandscape>(InWorld))
		{
			TargetLandscape = Actor;
			break;
		}

	}

	if (TargetLandscape == nullptr)
	{
		return;		
	}
	
	MapPreset = GetLevelGenerator()->GetMapPreset();
	
	ClearAllRivers();

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
	
	if (!InWorld || !TargetLandscape || !MapPreset)
	{
		UE_LOG(LogTemp, Warning, TEXT("River generation failed: Invalid world or landscape or map preset."));
		return;
	}

	if (!MapPreset->bGenerateRiver)
	{
		return;
	}

	// FVector LandscapeOrigin = InLandscape->GetActorLocation();
	// FVector LandscapeExtent = InLandscape->GetLoadedBounds().GetExtent();
	// Generate River Spline
	FVector VolumeOrigin = GetLevelGenerator()->GetVolumeOrigin();
	FVector VolumeExtent = GetLevelGenerator()->GetVolumeExtent();
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
			
			if (GetLandscapePointWorldPosition(Current, VolumeOrigin, VolumeExtent).Z < SeaHeight)
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
				RiverPath.Add(GetLandscapePointWorldPosition(Current, VolumeOrigin, VolumeExtent));
				Current = CameFrom[Current];
			}
			RiverPath.Add(GetLandscapePointWorldPosition(StartPoint, VolumeOrigin, VolumeExtent));
			Algo::Reverse(RiverPath);
			
			TArray<FVector> SimplifiedRiverPath;
			SimplifyPathRDP(RiverPath, SimplifiedRiverPath, MapPreset->RiverSpineSimplifyEpsilon);

			// Generate AWaterBodyRiver Actor
			FVector WaterBodyPos = GetLandscapePointWorldPosition(StartPoint, VolumeOrigin, VolumeExtent);
			FTransform WaterBodyTransform = FTransform(WaterBodyPos);
			AWaterBodyRiver* WaterBodyRiver = InWorld->SpawnActor<AWaterBodyRiver>(AWaterBodyRiver::StaticClass(), WaterBodyTransform);

			TArray<AActor*> FoundActors;
			UGameplayStatics::GetAllActorsOfClass(InWorld, AWaterZone::StaticClass(), FoundActors);

			FVector LandscapeSize = InLandscape->GetLoadedBounds().GetSize();
			for (AActor* Actor : FoundActors)
			{
				if (AWaterZone* WaterZone = Cast<AWaterZone>(Actor))
				{
					// 2. WaterZone의 ZoneExtent 프로퍼티를 랜드스케이프의 X, Y 크기로 설정합니다.
					WaterZone->SetZoneExtent(FVector2D(LandscapeSize.X, LandscapeSize.Y));
				}
			}
			SetDefaultRiverProperties(WaterBodyRiver, SimplifiedRiverPath);
			SetRiverWidth(WaterBodyRiver, SimplifiedRiverPath);

			// 에디터 전용으로만 실행
			if (GetWorld()->IsEditorWorld())
			{
				// 1) 컴포넌트 자신을 트랜잭션에 기록
				Modify();
				// 2) 소유 액터도 기록하고 레벨에 더티 플래그 세우기
				if (AActor* Owner = GetOwner())
				{
					Owner->Modify();
					Owner->MarkPackageDirty();
				}
			}

			WaterBodyRiver->Modify();
			
			GeneratedRivers.Add(WaterBodyRiver);
			CachedRivers.Add(TSoftObjectPtr<AWaterBodyRiver>(WaterBodyRiver));
		}
	}
#endif
}

void UOCGRiverGenerateComponent::SetMapData(const TArray<uint16>& InHeightMap, UMapPreset* InMapPreset, float InMinHeight, float InMaxHeight)
{
	HeightMapData = InHeightMap;
	MapPreset = InMapPreset;
	SeaHeight = MapPreset->MinHeight + 
		(MapPreset->MaxHeight - MapPreset->MinHeight) * MapPreset->SeaLevel - 1;
}

void UOCGRiverGenerateComponent::SetRiverWidth(AWaterBodyRiver* InRiverActor, const TArray<FVector>& InRiverPath)
{
	// 필수 컴포넌트들을 가져옵니다.
    if (!InRiverActor || !MapPreset)
    {
        return;
    }

    UWaterSplineComponent* SplineComp = InRiverActor->GetWaterSpline();
    UWaterBodyRiverComponent* RiverComp = Cast<UWaterBodyRiverComponent>(InRiverActor->GetWaterBodyComponent());
    UWaterSplineMetadata* SplineMetadata = RiverComp ? RiverComp->GetWaterSplineMetadata() : nullptr;

    // [핵심 변경] 너비 커브만 사용합니다.
    UCurveFloat* RiverWidthCurve = MapPreset->RiverWidthCurve;

    if (!SplineComp || !RiverComp || !SplineMetadata || !RiverWidthCurve)
    {
        return;
    }

    const int32 NumPoints = SplineComp->GetNumberOfSplinePoints();
    if (NumPoints < 2)
    {
        return;
    }

    // 너비 커브의 최소/최대 값을 가져와 정규화를 준비합니다.
    float MinWidth, MaxWidth;
    RiverWidthCurve->GetValueRange(MinWidth, MaxWidth);
    const float WidthRange = MaxWidth - MinWidth;

    for (int32 i = 0; i < NumPoints; ++i)
    {
        const float NormalizedDistance = (NumPoints > 1) ? static_cast<float>(i) / (NumPoints - 1) : 0.0f;

        // --- 너비(Width) 기준 값 계산 ---
        const float RawWidth = RiverWidthCurve->GetFloatValue(NormalizedDistance);
        // 너비 커브 값을 0.0 ~ 1.0 범위의 배율로 정규화합니다.
        const float NormalizedWidthMultiplier = !FMath::IsNearlyZero(WidthRange) ? (RawWidth - MinWidth) / WidthRange : 1.0f;

        // --- 1. 최종 너비(Width) 계산 및 적용 ---
        const float DesiredWidth = MapPreset->RiverWidthBaseValue * NormalizedWidthMultiplier;

        // --- 2. 최종 깊이(Depth) 계산 및 적용 (너비에 비례) ---
        // 너비 배율을 그대로 사용하여 기본 깊이 값에 곱합니다.
        const float DesiredDepth = MapPreset->RiverDepthBaseValue * NormalizedWidthMultiplier;

        // --- 3. 최종 유속(Velocity) 계산 및 적용 (너비에 반비례) ---
        // 너비 배율을 반전시켜 (1.0 - 배율) 유속을 계산합니다.
        // 강이 가장 넓을 때(배율=1.0) 유속은 최소가 되고, 가장 좁을 때(배율=0.0) 유속은 최대가 됩니다.
        const float InverseNormalizedWidthMultiplier = 1.0f - NormalizedWidthMultiplier;
        const float DesiredVelocity = MapPreset->RiverVelocityBaseValue * InverseNormalizedWidthMultiplier;

        // 계산된 값들을 Metadata와 SplineComponent에 적용합니다.
        if (SplineMetadata->RiverWidth.Points.IsValidIndex(i))
        {
            SplineMetadata->RiverWidth.Points[i].OutVal = DesiredWidth;
            SplineMetadata->Depth.Points[i].OutVal = DesiredDepth;
            SplineMetadata->WaterVelocityScalar.Points[i].OutVal = DesiredVelocity;
        }
        if (SplineComp->SplineCurves.Scale.Points.IsValidIndex(i))
        {
            SplineComp->SplineCurves.Scale.Points[i].OutVal.X = DesiredWidth;
            SplineComp->SplineCurves.Scale.Points[i].OutVal.Y = DesiredDepth;
        }
    }

    // 스플라인 커브 데이터가 변경되었음을 알리고 내부 데이터를 업데이트합니다.
    SplineComp->UpdateSpline();

    // Water Body Component에 변경 사항을 알려 메시를 다시 생성하도록 합니다.
    FOnWaterBodyChangedParams Params;
    Params.bShapeOrPositionChanged = true;
    Params.bUserTriggered = true;
    
    RiverComp->OnWaterBodyChanged(Params);
}

AOCGLevelGenerator* UOCGRiverGenerateComponent::GetLevelGenerator() const
{
	return Cast<AOCGLevelGenerator>(GetOwner());
}

void UOCGRiverGenerateComponent::ClearAllRivers()
{
#if WITH_EDITOR
	if (GetWorld()->IsEditorWorld())
	{
		// 컴포넌트와 소유 액터를 수정 표시
		Modify();
		if (AActor* Owner = GetOwner())
		{
			Owner->Modify();
			Owner->MarkPackageDirty();
		}
	}
#endif

	// 1) 이미 로드된 GeneratedRivers 파괴
	for (AWaterBodyRiver* River : GeneratedRivers)
	{
		if (!River) continue;
#if WITH_EDITOR
		// 에디터에서는 EditorDestroyActor 사용
		GEditor->GetEditorWorldContext().World()->EditorDestroyActor(River, /*bShouldModifyLevel=*/true);
#else
		River->Destroy();
#endif
	}
	GeneratedRivers.Empty();

	// 2) CachedRivers 의 SoftObjectPtr 통해 로드되지 않은 인스턴스까지 파괴
	for (TSoftObjectPtr<AWaterBodyRiver>& RiverPtr : CachedRivers)
	{
		// 2-1) 유효한 경로인지 확인
		const FSoftObjectPath& Path = RiverPtr.ToSoftObjectPath();
		if (!Path.IsValid()) continue;

		// 2-2) 이미 로드된 인스턴스가 있으면 Get(), 없으면 LoadSynchronous()
		AWaterBodyRiver* RiverInst = RiverPtr.IsValid()
			? RiverPtr.Get()
			: Cast<AWaterBodyRiver>(RiverPtr.LoadSynchronous());
		if (!RiverInst) continue;

#if WITH_EDITOR
		GEditor->GetEditorWorldContext().World()->EditorDestroyActor(RiverInst, /*bShouldModifyLevel=*/true);
#else
		RiverInst->Destroy();
#endif
	}
	CachedRivers.Empty();
}

FVector UOCGRiverGenerateComponent::GetLandscapePointWorldPosition(const FIntPoint& MapPoint, const FVector& LandscapeOrigin, const FVector& LandscapeExtent) const
{
	if (!MapPreset)
	{
		return FVector::ZeroVector;
	}

	FVector WorldLocation = LandscapeOrigin + FVector(
		2 * (MapPoint.X / (float)MapPreset->MapResolution.X) * LandscapeExtent.X,
		2 * (MapPoint.Y / (float)MapPreset->MapResolution.Y) * LandscapeExtent.Y,
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

void UOCGRiverGenerateComponent::SetDefaultRiverProperties(class AWaterBodyRiver* InRiverActor, const TArray<FVector>& InRiverPath)
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

FIntPoint UOCGRiverGenerateComponent::GetRandomStartPoint(float StartPointThresholdMultiplier, ALandscape* InLandscape)
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

void UOCGRiverGenerateComponent::SimplifyPathRDP(const TArray<FVector>& InPoints, TArray<FVector>& OutPoints,
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


