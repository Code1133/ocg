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
		PrevRiverMaskedWeight.Empty();
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
					// 2. Set the ZoneExtent property of the WaterZone to the landscape's X and Y size.
					WaterZone->SetZoneExtent(FVector2D(LandscapeSize.X, LandscapeSize.Y));
				}
			}
			SetDefaultRiverProperties(WaterBodyRiver, SimplifiedRiverPath);
			AddRiverProperties(WaterBodyRiver, SimplifiedRiverPath);
			
			if (GetWorld()->IsEditorWorld())
			{
				Modify();
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
    if (!InRiverActor || !MapPreset)
    {
        return;
    }

    UWaterSplineComponent* SplineComp = InRiverActor->GetWaterSpline();
    UWaterBodyRiverComponent* RiverComp = Cast<UWaterBodyRiverComponent>(InRiverActor->GetWaterBodyComponent());
    UWaterSplineMetadata* SplineMetadata = RiverComp ? RiverComp->GetWaterSplineMetadata() : nullptr;
	
    UCurveFloat* RiverWidthCurve = MapPreset->RiverWidthCurve;
    UCurveFloat* RiverDepthCurve = MapPreset->RiverDepthCurve;
    UCurveFloat* RiverVelocityCurve = MapPreset->RiverVelocityCurve;
	
    if (!SplineComp || !RiverComp || !SplineMetadata)
    {
        return;
    }

    const int32 NumPoints = SplineComp->GetNumberOfSplinePoints();
    if (NumPoints < 2)
    {
        return;
    }

	// Calculate the minimum and maximum values only if each curve is valid.
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
    
	// Calculate the Range to prevent division by zero.
    const float WidthRange = MaxWidth - MinWidth;
    const float DepthRange = MaxDepth - MinDepth;
    const float VelocityRange = MaxVelocity - MinVelocity;

    for (int32 i = 0; i < NumPoints; ++i)
    {
    	// Calculate the normalized distance ranging from 0.0 to 1.0 from the start to the end of the spline.
        const float NormalizedDistance = (NumPoints > 1) ? static_cast<float>(i) / (NumPoints - 1) : 0.0f;

        // 1. Calculate the width (Width) scale
        float WidthMultiplier;
        if (RiverWidthCurve)
        {
            const float RawWidth = RiverWidthCurve->GetFloatValue(NormalizedDistance);
            WidthMultiplier = !FMath::IsNearlyZero(WidthRange) ? (RawWidth - MinWidth) / WidthRange : 1.0f;
        }
        else // If there is no width curve (linear increase)
        {
            WidthMultiplier = NormalizedDistance;
        }

    	// --- 2. Calculate the depth (Depth) scale ---
        float DepthMultiplier;
        if (RiverDepthCurve)
        {
            const float RawDepth = RiverDepthCurve->GetFloatValue(NormalizedDistance);
            DepthMultiplier = !FMath::IsNearlyZero(DepthRange) ? (RawDepth - MinDepth) / DepthRange : 1.0f;
        }
        else // If there is no depth curve (linear increase)
        {
            DepthMultiplier = NormalizedDistance;
        }

        // 3. Calculate the velocity (Velocity) scale
        float VelocityMultiplier;
        if (RiverVelocityCurve)
        {
            const float RawVelocity = RiverVelocityCurve->GetFloatValue(NormalizedDistance);
            VelocityMultiplier = !FMath::IsNearlyZero(VelocityRange) ? (RawVelocity - MinVelocity) / VelocityRange : 1.0f;
        }
        else // If there is no velocity curve (linear increase)
        {
            VelocityMultiplier = NormalizedDistance;
        }

    	// Calculate the final value: (base value * scale) + minimum value
        const float DesiredWidth = ((MapPreset->RiverWidthBaseValue * WidthMultiplier) + MapPreset->RiverWidthMin) * MapPreset->LandscapeScale;
        const float DesiredDepth = (MapPreset->RiverDepthBaseValue * DepthMultiplier) + MapPreset->RiverDepthMin;
        const float DesiredVelocity = (MapPreset->RiverVelocityBaseValue * VelocityMultiplier) + MapPreset->RiverVelocityMin;
    	
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

	// Update the spline and Water Body components
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

void UOCGRiverGenerateComponent::ExportWaterEditLayerHeightMap(const uint16 MinDiffThreshold)
{
	if (TargetLandscape == nullptr)
	{
		TargetLandscape = GetLevelGenerator()->GetLandscape();
	}
	
	if (TargetLandscape)
	{
		const ULandscapeInfo* Info = TargetLandscape->GetLandscapeInfo();
		if (!Info) return;

		FGuid CurrentLayerGuid = OCGLandscapeUtil::GetLandscapeLayerGuid(TargetLandscape, FName(TEXT("Layer")));
		
		TArray<uint16> BlendedHeightData;
		int32 SizeX, SizeY;
		OCGLandscapeUtil::ExtractHeightMap(TargetLandscape, FGuid(), SizeX, SizeY, BlendedHeightData);

		TArray<uint16> BaseLayerHeightData;
		OCGLandscapeUtil::ExtractHeightMap(TargetLandscape, CurrentLayerGuid, SizeX, SizeY, BaseLayerHeightData);

		CachedRiverHeightMap.Empty();
		CachedRiverHeightMap.AddZeroed(SizeX * SizeY);

		RiverHeightMapWidth = SizeX;
		RiverHeightMapHeight = SizeY;

		const uint16*  BlendedData = BlendedHeightData.GetData();
		uint16*        BaseData    = BaseLayerHeightData.GetData();
		uint16*        CachedData  = CachedRiverHeightMap.GetData();
		
		if (BlendedHeightData.Num() == BaseLayerHeightData.Num() && BlendedHeightData.Num() == SizeY * SizeY)
		{
			for (int i = 0; i < BlendedHeightData.Num(); ++i)
			{
				CachedData[i] = static_cast<uint16>((FMath::Abs(BaseData[i] - BlendedData[i]) > static_cast<uint16>(MinDiffThreshold)) * UINT16_MAX);
			}
		}
		
		// Export to .png File
		const FIntPoint Resolution = FIntPoint(SizeX, SizeY);

		const UMapPreset* CurMapPreset = GetLevelGenerator()->GetMapPreset();
		
		if (CurMapPreset && CurMapPreset->bExportMapTextures)
		{
			OCGMapDataUtils::ExportMap(CachedRiverHeightMap, Resolution, TEXT("WaterHeightMap.png"));
		}
	}
}

void UOCGRiverGenerateComponent::ApplyWaterWeight()
{
	ExportWaterEditLayerHeightMap(2);
	
	if (TargetLandscape == nullptr)
	{
		TargetLandscape = GetLevelGenerator()->GetLandscape();
	}
	
	if (TargetLandscape)
	{
		UMaterial* CurrentLandscapeMaterial = Cast<UMaterial>(GetLevelGenerator()->GetMapPreset()->LandscapeMaterial->Parent);

		TArray<FName> LayerNames = OCGMaterialEditTool::ExtractLandscapeLayerName(CurrentLandscapeMaterial);

		const ULandscapeInfo* LandscapeInfo = TargetLandscape->GetLandscapeInfo();
		ULandscapeLayerInfoObject* FirstLayer = nullptr;
		if (LandscapeInfo && !LayerNames.IsEmpty())
		{
			FirstLayer = LandscapeInfo->GetLayerInfoByName(LayerNames[0]);
		}
		
		if (!PrevRiverMaskedWeight.IsEmpty())
		{
			for (auto Item : PrevRiverMaskedWeight)
			{
				if (LandscapeInfo && !LayerNames.IsEmpty())
				{
					ULandscapeLayerInfoObject* LanyerInfo = LandscapeInfo->GetLayerInfoByName(Item.Key);
					TArray<uint8> OriginWeightMap;
					OCGLandscapeUtil::GetWeightMap(TargetLandscape, LanyerInfo, OriginWeightMap);
					OCGLandscapeUtil::ApplyMaskedWeightMap(TargetLandscape, LanyerInfo, OriginWeightMap, Item.Value.MaskedWeightMap);
				}
			}
		}
		TArray<uint8> WeightMap;
		OCGLandscapeUtil::MakeWeightMapFromHeightDiff(CachedRiverHeightMap, WeightMap);
		
		TArray<uint8> BlurredWeightMap;
		OCGLandscapeUtil::BlurWeightMap(WeightMap, BlurredWeightMap, RiverHeightMapWidth, RiverHeightMapHeight);
		
		if (MapPreset->bExportMapTextures)
		{
			OCGMapDataUtils::ExportMap(WeightMap, FIntPoint(RiverHeightMapWidth, RiverHeightMapHeight), TEXT("AddWeightMap.png"));
			OCGMapDataUtils::ExportMap(BlurredWeightMap, FIntPoint(RiverHeightMapWidth, RiverHeightMapHeight), TEXT("BlurredWeightMap.png"));
		}
			
		for (FLandscapeInfoLayerSettings Layer : LandscapeInfo->Layers)
		{
			FMaskedWeight MaskedWeight;
			OCGLandscapeUtil::GetMaskedWeightMap(TargetLandscape, Layer.LayerInfoObj, BlurredWeightMap, MaskedWeight.MaskedWeightMap);
			if (MapPreset->bExportMapTextures)
			{
				FString FileName = TEXT("River") + Layer.LayerName.ToString() + TEXT(".png");
				OCGMapDataUtils::ExportMap(MaskedWeight.MaskedWeightMap, FIntPoint(RiverHeightMapWidth, RiverHeightMapHeight), FileName);	
			}
			
			PrevRiverMaskedWeight.Add(Layer.LayerName, MaskedWeight);
		}
		
		OCGLandscapeUtil::AddWeightMap(TargetLandscape, FirstLayer, BlurredWeightMap);
	}
}

void UOCGRiverGenerateComponent::ClearAllRivers()
{
#if WITH_EDITOR
	if (GetWorld()->IsEditorWorld())
	{
		Modify();
		if (AActor* Owner = GetOwner())
		{
			Owner->Modify();
			(void)Owner->MarkPackageDirty();
		}
	}
#endif

	// 1) Destroy the already loaded GeneratedRivers
	for (AWaterBodyRiver* River : GeneratedRivers)
	{
		if (!River) continue;
#if WITH_EDITOR
		GEditor->GetEditorWorldContext().World()->EditorDestroyActor(River, /*bShouldModifyLevel=*/true);
#else
		River->Destroy();
#endif
	}
	GeneratedRivers.Empty();

	// 2) Destroy even the instances that are not loaded through the SoftObjectPtr of CachedRivers
	for (TSoftObjectPtr<AWaterBodyRiver>& RiverPtr : CachedRivers)
	{
		// 2-1) Check if the path is valid
		const FSoftObjectPath& Path = RiverPtr.ToSoftObjectPath();
		if (!Path.IsValid()) continue;

		// 2-2) If there is an already loaded instance, use Get(); otherwise, use LoadSynchronous()
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


