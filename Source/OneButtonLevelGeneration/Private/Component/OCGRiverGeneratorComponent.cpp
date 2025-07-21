// Copyright (c) 2025 Code1133. All rights reserved.


#include "Component/OCGRiverGeneratorComponent.h"

#include "EngineUtils.h"

#include "OCGLevelGenerator.h"
#include "OCGLog.h"
#include "WaterBodyRiverActor.h"
#include "WaterBodyRiverComponent.h"
#include "WaterEditorSettings.h"
#include "WaterSplineComponent.h"
#include "Components/SplineComponent.h"
#include "Data/MapData.h"
#include "Data/MapPreset.h"
#include "Kismet/GameplayStatics.h"
#include "Utils/OCGLandscapeUtil.h"
#include "Utils/OCGMaterialEditTool.h"

#if WITH_EDITOR
#include "Landscape.h"
#if ENGINE_MINOR_VERSION > 5
#include "LandscapeEditLayer.h"
#endif
#endif


UOCGRiverGenerateComponent::UOCGRiverGenerateComponent()
{
}

void UOCGRiverGenerateComponent::GenerateRiver(UWorld* InWorld, ALandscape* InLandscape, bool bForceCleanUpPrevWaterWeightMap)
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
	if (bForceCleanUpPrevWaterWeightMap)
	{
		PrevWaterWeightMap.Empty();
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
	
	if (!InWorld || !TargetLandscape || !MapPreset)
	{
		UE_LOG(LogOCGModule, Warning, TEXT("River generation failed: Invalid world or landscape or map preset."));
		return;
	}

	if (!MapPreset->bGenerateRiver)
	{
		return;
	}

	const TArray<uint16>& HeightMapData = MapPreset->HeightMapData;
	if (HeightMapData.Num() < MapPreset->MapResolution.X * MapPreset->MapResolution.Y)
	{
		UE_LOG(LogOCGModule, Warning, TEXT("River generation failed: HeightMapData is not set or has insufficient data."));
		return;
	}

	CacheRiverStartPoints();

	FVector LandscapeOrigin = InLandscape->GetActorLocation();
	FVector LandscapeExtent = InLandscape->GetLoadedBounds().GetExtent();
	// Generate River Spline
	for (int RiverCount = 0; RiverCount < MapPreset->RiverCount; RiverCount++)
	{
		FIntPoint MapResolution = MapPreset->MapResolution;
		FIntPoint StartPoint = GetRandomStartPoint();

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
			
			if (GetLandscapePointWorldPosition(Current, LandscapeOrigin, LandscapeExtent).Z < SeaHeight)
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

					// 1. 현재 노드와 이웃 노드의 높이 가져오기
					const int32 CurrentIdx = Current.Y * MapResolution.X + Current.X;
					const int32 NeighborIdx = Neighbor.Y * MapResolution.X + Neighbor.X;
					const float CurrentHeight = static_cast<float>(HeightMapData[CurrentIdx]);
					const float NeighborHeight = static_cast<float>(HeightMapData[NeighborIdx]);

					// 2. 이동 비용 계산 (기본 거리 + 지형 페널티)
					// 2-1. 기본 이동 거리 비용
					float MovementCost = (dx != 0 && dy != 0) ? 1.414f : 1.0f;

					// 2-2. 오르막길 페널티 추가 (핵심)
					const float HeightDifference = NeighborHeight - CurrentHeight;
					if (HeightDifference > 0)
					{
						// 이 값을 0으로 설정하면 페널티가 없어져야 정상입니다.
						// 1.0 ~ 20.0 사이의 값으로 테스트 해보세요.
						const float UphillPenaltyMultiplier = MapPreset->UphillPenaltyMultiplier;
						MovementCost += HeightDifference * UphillPenaltyMultiplier;
					}

					// 3. 시작점부터 이웃 노드까지의 총 예상 비용(G) 계산
					const float TentativeCostG = CostSoFar[Current] + MovementCost;

					// 4. 이 경로가 기존에 발견된 경로보다 더 나은지 확인
					if (!CostSoFar.Contains(Neighbor) || TentativeCostG < CostSoFar[Neighbor])
					{
						// 더 나은 경로이므로 정보 업데이트
						CameFrom.Add(Neighbor, Current);
						CostSoFar.Add(Neighbor, TentativeCostG);

						// 5. 최종 우선순위(F = G + H) 계산 및 Frontier에 추가
						const float HeuristicH = NeighborHeight - SeaHeight;
						const float PriorityF = TentativeCostG + HeuristicH;
						Frontier.Add({Neighbor, PriorityF});
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
			SimplifyPathRDP(RiverPath, SimplifiedRiverPath, MapPreset->RiverSplineSimplifyEpsilon);

			// Generate AWaterBodyRiver Actor
			FVector WaterBodyPos = GetLandscapePointWorldPosition(StartPoint, LandscapeOrigin, LandscapeExtent);
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
			AddRiverProperties(WaterBodyRiver, SimplifiedRiverPath);

			// 에디터 전용으로만 실행
			if (GetWorld()->IsEditorWorld())
			{
				// 1) 컴포넌트 자신을 트랜잭션에 기록
				Modify();
				// 2) 소유 액터도 기록하고 레벨에 더티 플래그 세우기
				if (AActor* Owner = GetOwner())
				{
					Owner->Modify();
					(void)Owner->MarkPackageDirty();
				}
			}

			WaterBodyRiver->Modify();
			
			GeneratedRivers.Add(WaterBodyRiver);
			CachedRivers.Add(TSoftObjectPtr<AWaterBodyRiver>(WaterBodyRiver));

#if ENGINE_MINOR_VERSION > 5
			FGuid WaterLayerGuid = InLandscape->GetEditLayerConst(1)->GetGuid();
#else
			FGuid WaterLayerGuid = InLandscape->GetLayerConst(1)->Guid;
#endif

			FScopedSetLandscapeEditingLayer Scope(InLandscape, WaterLayerGuid, [&]
			{
				check(InLandscape);
				InLandscape->RequestLayersContentUpdate(ELandscapeLayerUpdateMode::Update_Heightmap_All);
			});

			if (ULandscapeInfo* LandscapeInfo = InLandscape->GetLandscapeInfo())
			{
				LandscapeInfo->ForceLayersFullUpdate();
			}
		}
	}

	ApplyWaterWeight();
#endif
}

void UOCGRiverGenerateComponent::SetMapData(const TArray<uint16>& InHeightMap, UMapPreset* InMapPreset, float InMinHeight, float InMaxHeight)
{
	MapPreset = InMapPreset;
}

void UOCGRiverGenerateComponent::AddRiverProperties(AWaterBodyRiver* InRiverActor, const TArray<FVector>& InRiverPath)
{
    // 필수 컴포넌트 및 프리셋 유효성 검사
    if (!InRiverActor || !MapPreset)
    {
        return;
    }

    UWaterSplineComponent* SplineComp = InRiverActor->GetWaterSpline();
    UWaterBodyRiverComponent* RiverComp = Cast<UWaterBodyRiverComponent>(InRiverActor->GetWaterBodyComponent());
    UWaterSplineMetadata* SplineMetadata = RiverComp ? RiverComp->GetWaterSplineMetadata() : nullptr;

    // [변경점 1] 커브 에셋을 가져옵니다. 없으면 nullptr이 됩니다.
    UCurveFloat* RiverWidthCurve = MapPreset->RiverWidthCurve;
    UCurveFloat* RiverDepthCurve = MapPreset->RiverDepthCurve;
    UCurveFloat* RiverVelocityCurve = MapPreset->RiverVelocityCurve;

    // 커브를 제외한 필수 데이터가 유효한지 확인합니다.
    if (!SplineComp || !RiverComp || !SplineMetadata)
    {
        return;
    }

    const int32 NumPoints = SplineComp->GetNumberOfSplinePoints();
    if (NumPoints < 2)
    {
        return;
    }

    // 각 커브가 유효할 경우에만 최소/최대 값을 계산합니다.
    float MinWidth = 0.f, MaxWidth = 1.f;
    if (RiverWidthCurve)
    {
        RiverWidthCurve->GetValueRange(MinWidth, MaxWidth);
    }

    float MinDepth = 0.f, MaxDepth = 1.f;
    if (RiverDepthCurve)
    {
        RiverDepthCurve->GetValueRange(MinDepth, MaxDepth);
    }

    float MinVelocity = 0.f, MaxVelocity = 1.f;
    if (RiverVelocityCurve)
    {
        RiverVelocityCurve->GetValueRange(MinVelocity, MaxVelocity);
    }
    
    // 0으로 나누는 것을 방지하기 위해 Range를 계산합니다.
    const float WidthRange = MaxWidth - MinWidth;
    const float DepthRange = MaxDepth - MinDepth;
    const float VelocityRange = MaxVelocity - MinVelocity;

    for (int32 i = 0; i < NumPoints; ++i)
    {
        // 스플라인의 시작부터 끝까지 0.0 ~ 1.0 범위의 정규화된 거리를 계산합니다.
        const float NormalizedDistance = (NumPoints > 1) ? static_cast<float>(i) / (NumPoints - 1) : 0.0f;

        // --- 1. 너비(Width) 배율 계산 ---
        float WidthMultiplier;
        if (RiverWidthCurve) // [변경점 2] 너비 커브가 유효한 경우
        {
            const float RawWidth = RiverWidthCurve->GetFloatValue(NormalizedDistance);
            WidthMultiplier = !FMath::IsNearlyZero(WidthRange) ? (RawWidth - MinWidth) / WidthRange : 1.0f;
        }
        else // [변경점 3] 너비 커브가 없는 경우 (선형 증가)
        {
            WidthMultiplier = NormalizedDistance;
        }

        // --- 2. 깊이(Depth) 배율 계산 ---
        float DepthMultiplier;
        if (RiverDepthCurve) // [변경점 2] 깊이 커브가 유효한 경우
        {
            const float RawDepth = RiverDepthCurve->GetFloatValue(NormalizedDistance);
            DepthMultiplier = !FMath::IsNearlyZero(DepthRange) ? (RawDepth - MinDepth) / DepthRange : 1.0f;
        }
        else // [변경점 3] 깊이 커브가 없는 경우 (선형 증가)
        {
            DepthMultiplier = NormalizedDistance;
        }

        // --- 3. 유속(Velocity) 배율 계산 ---
        float VelocityMultiplier;
        if (RiverVelocityCurve) // [변경점 2] 유속 커브가 유효한 경우
        {
            const float RawVelocity = RiverVelocityCurve->GetFloatValue(NormalizedDistance);
            VelocityMultiplier = !FMath::IsNearlyZero(VelocityRange) ? (RawVelocity - MinVelocity) / VelocityRange : 1.0f;
        }
        else // [변경점 3] 유속 커브가 없는 경우 (선형 증가)
        {
            VelocityMultiplier = NormalizedDistance;
        }

        // 최종 값 계산: (기본값 * 배율) + 최소값
        const float DesiredWidth = ((MapPreset->RiverWidthBaseValue * WidthMultiplier) + MapPreset->RiverWidthMin) * MapPreset->LandscapeScale;
        const float DesiredDepth = (MapPreset->RiverDepthBaseValue * DepthMultiplier) + MapPreset->RiverDepthMin;
        const float DesiredVelocity = (MapPreset->RiverVelocityBaseValue * VelocityMultiplier) + MapPreset->RiverVelocityMin;

        // 계산된 값들을 Metadata에 적용
        if (SplineMetadata->RiverWidth.Points.IsValidIndex(i))
        {
            SplineMetadata->RiverWidth.Points[i].OutVal = DesiredWidth;
            SplineMetadata->Depth.Points[i].OutVal = DesiredDepth;
            SplineMetadata->WaterVelocityScalar.Points[i].OutVal = DesiredVelocity;
        }
        
        // 스플라인 스케일 업데이트
        if (SplineComp->SplineCurves.Scale.Points.IsValidIndex(i))
        {
            SplineComp->SetScaleAtSplinePoint(i, FVector(DesiredWidth, DesiredDepth, 1.0f), ESplineCoordinateSpace::Local);
        }
    }

    // 스플라인 및 Water Body 컴포넌트 업데이트
    SplineComp->UpdateSpline();
    
    FOnWaterBodyChangedParams Params;
    Params.bShapeOrPositionChanged = true;
    Params.bUserTriggered = true;
    RiverComp->OnWaterBodyChanged(Params);
}

AOCGLevelGenerator* UOCGRiverGenerateComponent::GetLevelGenerator() const
{
	return Cast<AOCGLevelGenerator>(GetOwner());
}

void UOCGRiverGenerateComponent::ExportWaterEditLayerHeightMap()
{
	if (TargetLandscape == nullptr)
	{
		TargetLandscape = GetLevelGenerator()->GetLandscape();
	}
	
	if (TargetLandscape)
	{
		ULandscapeInfo* Info = TargetLandscape->GetLandscapeInfo();
		if (!Info) return;
#if ENGINE_MINOR_VERSION > 5
		FName BaseEditLayerName = FName(TEXT("Layer"));
		const ULandscapeEditLayerBase* BaseLayer = nullptr;
		{
			for (const ULandscapeEditLayerBase* Layer : TargetLandscape->GetEditLayersConst())
			{
				if (Layer->GetName() == BaseEditLayerName)
				{
					BaseLayer = Layer;
					break;
				}
			}
		}
#else
		
		FName BaseEditLayerName = FName(TEXT("Layer"));
		const FLandscapeLayer* BaseLayer = nullptr;
		for (const FLandscapeLayer& Layer : TargetLandscape->GetLayers())
		{
			if (Layer.Name == BaseEditLayerName)
			{
				BaseLayer = &Layer;
				break;
			}
		}
#endif
		
		TArray<uint16> BlendedHeightData;
		int32 SizeX, SizeY;
		OCGLandscapeUtil::ExtractHeightMap(TargetLandscape, FGuid(), SizeX, SizeY, BlendedHeightData);

		TArray<uint16> BaseLayerHeightData;
#if ENGINE_MINOR_VERSION > 5
		OCGLandscapeUtil::ExtractHeightMap(TargetLandscape, BaseLayer->GetGuid(), SizeX, SizeY, BaseLayerHeightData);
#else
		OCGLandscapeUtil::ExtractHeightMap(TargetLandscape, BaseLayer->Guid, SizeX, SizeY, BaseLayerHeightData);
#endif

		CachedRiverHeightMap.Empty();
		CachedRiverHeightMap.AddZeroed(SizeX * SizeY);

		RiverHeightMapWidth = SizeX;
		RiverHeightMapHeight = SizeY;
		
		if (BlendedHeightData.Num() == BaseLayerHeightData.Num() && BlendedHeightData.Num() == SizeY * SizeY)
		{
			for (int i = 0; i < BlendedHeightData.Num(); ++i)
			{
				CachedRiverHeightMap[i] = BlendedHeightData[i] - BaseLayerHeightData[i];
			}
		}
		
		// 2. PNG로 익스포트
		const FIntPoint Resolution = FIntPoint(SizeX, SizeY);

		UMapPreset* CurMapPreset = GetLevelGenerator()->GetMapPreset();
		
		if (CurMapPreset && CurMapPreset->bExportMapTextures)
		{
			OCGMapDataUtils::ExportMap(CachedRiverHeightMap, Resolution, TEXT("WaterHeightMap16.png"));
		}
	}
}

void UOCGRiverGenerateComponent::ApplyWaterWeight()
{
	ExportWaterEditLayerHeightMap();
	
	if (TargetLandscape == nullptr)
	{
		TargetLandscape = GetLevelGenerator()->GetLandscape();
	}
	
	if (TargetLandscape)
	{
		UMaterial* CurrentLandscapeMaterial = Cast<UMaterial>(GetLevelGenerator()->GetMapPreset()->LandscapeMaterial->Parent);

		TArray<FName> LayerNames = OCGMaterialEditTool::ExtractLandscapeLayerName(CurrentLandscapeMaterial);

		const ULandscapeInfo* LandscapeInfo = TargetLandscape->GetLandscapeInfo();
		int32 LayerIndex = 0;
		if (LandscapeInfo && !LayerNames.IsEmpty())
		{
			LayerIndex = LandscapeInfo->GetLayerInfoIndex(LayerNames[0]);
		}
		
		if (!PrevWaterWeightMap.IsEmpty())
		{
			OCGLandscapeUtil::ApplyWeightMap(TargetLandscape, LayerIndex, PrevWaterWeightMap);
		}
		OCGLandscapeUtil::GetWeightMap(TargetLandscape, LayerIndex, PrevWaterWeightMap);
		OCGLandscapeUtil::AddWeightMap(TargetLandscape, LayerIndex, RiverHeightMapWidth, RiverHeightMapHeight, CachedRiverHeightMap);
	}
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
			(void)Owner->MarkPackageDirty();
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

	TArray<uint16>& HeightMapData = MapPreset->HeightMapData;

	if (HeightMapData.Num() < MapPreset->MapResolution.X * MapPreset->MapResolution.Y)
	{
		UE_LOG(LogOCGModule, Warning, TEXT("HeightMapData is not set or has insufficient data."));
		return FVector::ZeroVector;
	}

	if (MapPreset->MapResolution.X < 1 || MapPreset->MapResolution.Y < 1)
	{
		UE_LOG(LogOCGModule, Warning, TEXT("MapResolution is invalid."));
		return FVector::ZeroVector;
	}

	FVector WorldLocation = LandscapeOrigin + FVector(
		2 * (MapPoint.X / static_cast<float>(MapPreset->MapResolution.X - 1)) * LandscapeExtent.X,
		2 * (MapPoint.Y / static_cast<float>(MapPreset->MapResolution.Y - 1)) * LandscapeExtent.Y,
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
		WorldLocation.Z = (HeightMapValue - 32768) / 128 * 100 * MapPreset->LandscapeScale; 
	}
	return WorldLocation;
}

void UOCGRiverGenerateComponent::SetDefaultRiverProperties(AWaterBodyRiver* InRiverActor, const TArray<FVector>& InRiverPath)
{
	UWaterBodyComponent* WaterBodyComponent = CastChecked<AWaterBody>(InRiverActor)->GetWaterBodyComponent();
	check(MapPreset && WaterBodyComponent);

	if (const FWaterBrushActorDefaults* WaterBrushActorDefaults = &GetDefault<UWaterEditorSettings>()->WaterBodyRiverDefaults.BrushDefaults)
	{
		WaterBodyComponent->CurveSettings = WaterBrushActorDefaults->CurveSettings;
		WaterBodyComponent->WaterHeightmapSettings = WaterBrushActorDefaults->HeightmapSettings;
		WaterBodyComponent->LayerWeightmapSettings = WaterBrushActorDefaults->LayerWeightmapSettings;
	}

	WaterBodyComponent->SetWaterMaterial(MapPreset->RiverWaterMaterial.LoadSynchronous());
	WaterBodyComponent->SetWaterStaticMeshMaterial(MapPreset->RiverWaterStaticMeshMaterial.LoadSynchronous());
	WaterBodyComponent->SetHLODMaterial(MapPreset->WaterHLODMaterial.LoadSynchronous());
	WaterBodyComponent->SetUnderwaterPostProcessMaterial(MapPreset->UnderwaterPostProcessMaterial.LoadSynchronous());
	
	if (const FWaterBodyDefaults* WaterBodyDefaults = &GetDefault<UWaterEditorSettings>()->WaterBodyRiverDefaults)
	{
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
	WaterBodyRiverComponent->SetLakeTransitionMaterial(MapPreset->RiverToLakeTransitionMaterial.LoadSynchronous());
	WaterBodyRiverComponent->SetOceanTransitionMaterial(MapPreset->RiverToOceanTransitionMaterial.LoadSynchronous());

	InRiverActor->PostEditChange();
	InRiverActor->PostEditMove(true);
	
	UWaterSplineComponent* WaterSpline = InRiverActor->GetWaterBodyComponent()->GetWaterSpline();
	WaterSpline->ClearSplinePoints();
	WaterSpline->SetSplinePoints(InRiverPath, ESplineCoordinateSpace::World, true);

	FOnWaterBodyChangedParams Params;
	Params.bShapeOrPositionChanged = true; 
	Params.bUserTriggered = true;       

	InRiverActor->GetWaterBodyComponent()->GetWaterSpline()->GetSplinePointsMetadata();
	InRiverActor->GetWaterBodyComponent()->UpdateAll(Params); 

	InRiverActor->GetWaterBodyComponent()->UpdateWaterBodyRenderData();
}

FIntPoint UOCGRiverGenerateComponent::GetRandomStartPoint()
{
	AOCGLevelGenerator* LevelGenerator = Cast<AOCGLevelGenerator>(GetOwner());
	
	if (!MapPreset || !LevelGenerator)
	{
		UE_LOG(LogOCGModule, Error, TEXT("MapPreset is not set. Cannot generate random start point for river."));
		return FIntPoint(0, 0);
	}

	// Select a random high point or default to the center of the map
	FIntPoint StartPoint = CachedRiverStartPoints.Num() > 0 ?
		CachedRiverStartPoints[FMath::RandRange(0, CachedRiverStartPoints.Num() - 1)] :
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

void UOCGRiverGenerateComponent::CacheRiverStartPoints()
{
	if (!TargetLandscape || !GetLevelGenerator() || !GetLevelGenerator()->GetMapPreset())
	{
		UE_LOG(LogOCGModule, Error, TEXT("TargetLandscape or LevelGenerator is not set. Cannot cache river start points."));
		return;
	}
	
	CachedRiverStartPoints.Empty();
    
	// Ensure the multiplier is within a reasonable range
	float StartPointThresholdMultiplier = FMath::Clamp(MapPreset->RiverSourceElevationRatio, 0.0f, 1.0f);
	float MaxHeight = MapPreset->MaxHeight;
	float MinHeight = MapPreset->MinHeight;

	
	SeaHeight = MinHeight + 
		(MaxHeight - MinHeight) * MapPreset->SeaLevel - 5;
    
	uint16 HighThreshold = SeaHeight + (MaxHeight - SeaHeight) * StartPointThresholdMultiplier;
	UE_LOG(LogOCGModule, Log, TEXT("High Threshold for River Start Point: %d"), HighThreshold);

	FVector LandscapeOrigin = TargetLandscape->GetLoadedBounds().GetCenter();
	FVector LandscapeExtent = TargetLandscape->GetLoadedBounds().GetExtent();
	for (int32 y = 0; y < MapPreset->MapResolution.Y; ++y)
	{
		for (int32 x = 0; x < MapPreset->MapResolution.X; ++x)
		{
			FVector WorldLocation = GetLandscapePointWorldPosition(FIntPoint(x, y), LandscapeOrigin, LandscapeExtent);
			if (WorldLocation.Z >= HighThreshold)
			{
				CachedRiverStartPoints.Add(FIntPoint(x, y));
			}
		}
	}
}


