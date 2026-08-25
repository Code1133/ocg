// Copyright (c) 2025-2026 Code1133. All rights reserved.
#include "Subsystems/OCGHydrologySubsystem.h"

#include "OCGLog.h"
#include "OCGStats.h"
#include "Data/MapPreset.h"
#include "Data/OCGHeightConverter.h"
#include "Data/OCGWorldDataContainer.h"
#include "Subsystems/OCGLandscapeGenSubsystem.h"
#include "Utils/OCGLandscapeUtils.h"
#include "Utils/OCGMaterialEditTool.h"

#include "Editor.h"
#include "Algo/Reverse.h"
#include "Engine/Level.h"
#include "Kismet/GameplayStatics.h"

#include "Landscape.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
	#include "LandscapeEditLayer.h"
#endif
#include "LandscapeInfo.h"

#include "WaterBodyActor.h"
#include "WaterBodyComponent.h"
#include "WaterBodyOceanActor.h"
#include "WaterBodyOceanComponent.h"
#include "WaterBodyRiverActor.h"
#include "WaterBodyRiverComponent.h"
#include "WaterEditorSettings.h"
#include "WaterRuntimeSettings.h"
#include "WaterSplineComponent.h"
#include "WaterZoneActor.h"

namespace
{
/** */
[[nodiscard]] UMaterialInterface* ResolveWaterMaterial(const TSoftObjectPtr<UMaterialInterface>& CustomMaterial, UMaterialInterface* DefaultMaterial)
{
	if (UMaterialInterface* Material = CustomMaterial.LoadSynchronous())
	{
		return Material;
	}
	return DefaultMaterial;
}

/** */
[[nodiscard]] bool ShouldOverrideWaterSplineDefaults(const UWaterSplineComponent* WaterSpline)
{
	check(WaterSpline);
	if (const AWaterBody* OwningBody = WaterSpline->GetTypedOuter<AWaterBody>())
	{
		return OwningBody->GetClass()->ClassGeneratedBy == nullptr;
	}
	return false;
}

/**
 *
 */
template <typename TWaterBodyDefaults>
	requires std::derived_from<TWaterBodyDefaults, FWaterBodyDefaults>
	&& requires(TWaterBodyDefaults WaterBodyDefaults)
	{
		WaterBodyDefaults.BrushDefaults;
		WaterBodyDefaults.SplineDefaults;
	}
void ApplyCommonWaterComponentSettings(
	UWaterBodyComponent* WaterBodyComponent,
	const TWaterBodyDefaults& WaterBodyDefaults,
	const TSoftObjectPtr<UMaterialInterface>& WaterMaterial,
	const TSoftObjectPtr<UMaterialInterface>& MeshMaterial,
	const TSoftObjectPtr<UMaterialInterface>& HLODMaterial,
	const TSoftObjectPtr<UMaterialInterface>& PPMaterial
)
{
	// --- WaterBodyActorFactory.cpp의 UWaterBodyActorFactory::PostSpawnActor(...) 참고 ---
	check(WaterBodyComponent);

	// Water Brush Settings
	{
		const FWaterBrushActorDefaults& WaterBrushActorDefaults = WaterBodyDefaults.BrushDefaults;
		WaterBodyComponent->CurveSettings = WaterBrushActorDefaults.CurveSettings;
		WaterBodyComponent->WaterHeightmapSettings = WaterBrushActorDefaults.HeightmapSettings;
		WaterBodyComponent->LayerWeightmapSettings = WaterBrushActorDefaults.LayerWeightmapSettings;
	}

	// Water Material Settings
	{
		// MapPreset에 설정된 Material이 있다면 로드하고, 없으면 엔진 디폴트 사용
		WaterBodyComponent->SetWaterMaterial(ResolveWaterMaterial(WaterMaterial, WaterBodyDefaults.GetWaterMaterial()));
		WaterBodyComponent->SetWaterStaticMeshMaterial(ResolveWaterMaterial(MeshMaterial, WaterBodyDefaults.GetWaterStaticMeshMaterial()));
		WaterBodyComponent->SetHLODMaterial(ResolveWaterMaterial(HLODMaterial, WaterBodyDefaults.GetWaterHLODMaterial()));
		WaterBodyComponent->SetUnderwaterPostProcessMaterial(ResolveWaterMaterial(PPMaterial, WaterBodyDefaults.GetUnderwaterPostProcessMaterial()));

		// UWaterBodyActorFactory를 거치지 않고 직접 스폰하므로, 여기서 직접 InfoMaterial을 지정
		WaterBodyComponent->SetWaterInfoMaterial(GetDefault<UWaterRuntimeSettings>()->GetDefaultWaterInfoMaterial());

		UWaterSplineComponent* WaterSpline = WaterBodyComponent->GetWaterSpline();
		if (ShouldOverrideWaterSplineDefaults(WaterSpline))
		{
			WaterSpline->WaterSplineDefaults = WaterBodyDefaults.SplineDefaults;
		}
	}

	// If the water body is spawned into a zone which is using local only tessellation, we must default to enabling static meshes.
	if (const AWaterZone* WaterZone = WaterBodyComponent->GetWaterZone())
	{
		if (WaterZone->IsLocalOnlyTessellationEnabled())
		{
			WaterBodyComponent->SetWaterBodyStaticMeshEnabled(true);
		}
	}
}

namespace Compat
{
#if ENGINE_MAJOR_VERSION == 5
	FORCEINLINE void ClearEditLayer(ALandscape* Landscape, FGuid LayerGuid)
	{
#if ENGINE_MINOR_VERSION <= 6
		Landscape->ClearLayer(LayerGuid);
#else
		Landscape->ClearEditLayer(LayerGuid);
#endif
	}
#endif
} // namespace Compat
} // annonimus namespace

void UOCGHydrologySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UOCGHydrologySubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UOCGHydrologySubsystem::ApplyHydrology(const UMapPreset* Preset, const FOCGWorldDataContainer& DataContainer)
{
	if (!Preset)
	{
		UE_LOG(LogOCGModule, Warning, TEXT("ApplyHydrology: Preset is null."));
		return;
	}

	UOCGLandscapeGenSubsystem* LandscapeSubsystem = GEditor->GetEditorSubsystem<UOCGLandscapeGenSubsystem>();
	if (!LandscapeSubsystem)
	{
		UE_LOG(LogOCGModule, Warning, TEXT("ApplyHydrology: OCGLandscapeGenSubsystem not found."));
		return;
	}

	ALandscape* Landscape = LandscapeSubsystem->GetLandscape();
	if (!Landscape)
	{
		UE_LOG(LogOCGModule, Warning, TEXT("ApplyHydrology: No landscape found."));
		return;
	}

	UWorld* World = Landscape->GetWorld();
	if (!World)
	{
		return;
	}

	if (Preset->RiverSettings.bGenerateRiver)
	{
		GenerateRivers(World, Landscape, Preset, DataContainer);
	}

	if (Preset->OceanSettings.bContainWater)
	{
		const FVector VolumeOrigin = LandscapeSubsystem->GetVolumeOrigin();
		const FVector VolumeExtent = LandscapeSubsystem->GetVolumeExtent();
		CreateOcean(World, Landscape, Preset, DataContainer, VolumeOrigin, VolumeExtent);
	}
}

void UOCGHydrologySubsystem::GenerateRivers(UWorld* World, ALandscape* InLandscape, const UMapPreset* Preset, const FOCGWorldDataContainer& DataContainer)
{
	SCOPE_CYCLE_COUNTER(STAT_OCG_RiverPathFind);

	const TArray<uint16>& HeightMapData = DataContainer.HeightMapData;
	const FIntPoint MapResolution = Preset->LandscapeSettings.MapResolution;

	if (HeightMapData.Num() < MapResolution.X * MapResolution.Y)
	{
		UE_LOG(LogOCGModule, Warning, TEXT("GenerateRivers: HeightMapData has insufficient data."));
		return;
	}

	if (bIsRiverExists && CurrentRiverSeed == Preset->RiverSettings.RiverSeed)
	{
		return;
	}

	ClearAllRivers(InLandscape);
	PrevRiverMaskedWeights.Empty();

	// WaterBrushManager 액터 제거
	UClass* WaterBrushManagerClass = StaticLoadClass(UObject::StaticClass(), nullptr, TEXT("/Script/WaterEditor.WaterBrushManager"));
	if (WaterBrushManagerClass)
	{
		TArray<AActor*> WaterBrushManagerActors;
		UGameplayStatics::GetAllActorsOfClass(World, WaterBrushManagerClass, WaterBrushManagerActors);
		for (AActor* Actor : WaterBrushManagerActors)
		{
			if (Actor)
			{
				Actor->Destroy();
			}
		}
	}

	CurrentRiverSeed = Preset->RiverSettings.RiverSeed;

	UOCGLandscapeGenSubsystem* LandscapeSubsystem = GEditor->GetEditorSubsystem<UOCGLandscapeGenSubsystem>();

	CacheRiverStartPoints(HeightMapData, Preset, DataContainer.CurMinHeight, DataContainer.CurMaxHeight);

	for (int32 RiverCount = 0; RiverCount < Preset->RiverSettings.RiverCount; RiverCount++)
	{
		const FIntPoint StartPoint = GetRandomStartPoint(RiverCount, Preset);

		TMap<FIntPoint, FIntPoint> CameFrom;
		TMap<FIntPoint, float> CostSoFar;
		TArray<TTuple<FIntPoint, float>> Frontier;

		Frontier.Add({StartPoint, 0.0f});
		CameFrom.Add(StartPoint, StartPoint);
		CostSoFar.Add(StartPoint, 0.0f);

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

			const TTuple<FIntPoint, float> BestNode = Frontier[BestIndex];
			Frontier.RemoveAt(BestIndex);

			const FIntPoint Current = BestNode.Get<0>();
			const FVector CurrentWorldPos = LandscapeSubsystem
				? LandscapeSubsystem->GetLandscapePointWorldPosition(Current, Preset, &HeightMapData)
				: FVector::ZeroVector;

			if (CurrentWorldPos.Z < SeaHeight)
			{
				GoalPoint = Current;
				break;
			}

			for (int32 OffsetX = -1; OffsetX <= 1; ++OffsetX)
			{
				for (int32 OffsetY = -1; OffsetY <= 1; ++OffsetY)
				{
					if (OffsetX == 0 && OffsetY == 0)
					{
						continue;
					}

					const FIntPoint Neighbor = FIntPoint(Current.X + OffsetX, Current.Y + OffsetY);
					if (Neighbor.X < 0 || Neighbor.X >= MapResolution.X ||
						Neighbor.Y < 0 || Neighbor.Y >= MapResolution.Y)
					{
						continue;
					}

					const float NewCost = CostSoFar[Current] + 1.0f;
					if (!CostSoFar.Contains(Neighbor) || NewCost < CostSoFar[Neighbor])
					{
						CostSoFar.Add(Neighbor, NewCost);
						const int32 NeighborIndex = Neighbor.Y * MapResolution.X + Neighbor.X;
						const float Heuristic = HeightMapData[NeighborIndex] - SeaHeight;
						Frontier.Add({Neighbor, NewCost + Heuristic});
						CameFrom.Add(Neighbor, Current);
					}
				}
			}
		}

		if (GoalPoint == FIntPoint(-1, -1))
		{
			continue;
		}

		if (!LandscapeSubsystem)
		{
			continue;
		}

		TArray<FVector> RiverPath;
		FIntPoint Current = GoalPoint;
		while (Current != StartPoint)
		{
			RiverPath.Add(LandscapeSubsystem->GetLandscapePointWorldPosition(Current, Preset, &HeightMapData));
			Current = CameFrom[Current];
		}
		RiverPath.Add(LandscapeSubsystem->GetLandscapePointWorldPosition(StartPoint, Preset, &HeightMapData));
		Algo::Reverse(RiverPath);

		TArray<FVector> SimplifiedPath;
		SimplifyPathRDP(RiverPath, SimplifiedPath, Preset->RiverSettings.RiverSplineSimplifyEpsilon);

		const FVector WaterBodyPos = LandscapeSubsystem->GetLandscapePointWorldPosition(StartPoint, Preset, &HeightMapData);
		AWaterBodyRiver* WaterBodyRiver = World->SpawnActor<AWaterBodyRiver>(AWaterBodyRiver::StaticClass(), FTransform(WaterBodyPos));
		if (!WaterBodyRiver)
		{
			continue;
		}

		// WaterZone을 랜드스케이프 전체 크기로 확장
		TArray<AActor*> FoundZones;
		UGameplayStatics::GetAllActorsOfClass(World, AWaterZone::StaticClass(), FoundZones);
		const FVector LandscapeSize = InLandscape->GetLoadedBounds().GetSize();
		for (AActor* Actor : FoundZones)
		{
			if (AWaterZone* WaterZone = Cast<AWaterZone>(Actor))
			{
				WaterZone->SetZoneExtent(FVector2D(LandscapeSize.X, LandscapeSize.Y));
			}
		}

		SetDefaultRiverProperties(WaterBodyRiver, SimplifiedPath, Preset);
		AddRiverProperties(WaterBodyRiver, SimplifiedPath, Preset);
		WaterBodyRiver->Modify();
		if (ULevel* Level = WaterBodyRiver->GetLevel())
		{
			(void)Level->MarkPackageDirty();
		}

		GeneratedRivers.Add(WaterBodyRiver);
		CachedRivers.Add(WaterBodyRiver);

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
		const auto* WaterEditLayer = InLandscape->GetEditLayerConst(1);
		if (!WaterEditLayer)
		{
			continue;
		}
		const FGuid WaterLayerGuid = WaterEditLayer->GetGuid();
#else
		const auto* WaterEditLayer = InLandscape->GetLayerConst(1);
		if (!WaterEditLayer)
		{
			continue;
		}
		const FGuid WaterLayerGuid = WaterEditLayer->Guid;
#endif

		FScopedSetLandscapeEditingLayer Scope(InLandscape, WaterLayerGuid, [&]
		{
			InLandscape->RequestLayersContentUpdate(ELandscapeLayerUpdateMode::Update_Heightmap_All);
		});

		if (ULandscapeInfo* LandscapeInfo = InLandscape->GetLandscapeInfo())
		{
			LandscapeInfo->ForceLayersFullUpdate();
		}
	}

	ApplyWaterWeight(InLandscape, Preset);
	bIsRiverExists = true;
}

void UOCGHydrologySubsystem::CreateOcean(UWorld* World, ALandscape* InLandscape, const UMapPreset* Preset, const FOCGWorldDataContainer& DataContainer, const FVector& VolumeOrigin, const FVector& VolumeExtent)
{
	if (CachedOcean.IsValid())
	{
		CachedOcean->Destroy();
		CachedOcean = nullptr;
	}
	else if (AWaterBodyOcean* PrevOcean = CachedOceanAsset.Get())
	{
		PrevOcean->Destroy();
	}
	CachedOceanAsset.Reset();

	AWaterBodyOcean* Ocean = World->SpawnActor<AWaterBodyOcean>(AWaterBodyOcean::StaticClass());
	if (!Ocean)
	{
		return;
	}
	Ocean->SetIsSpatiallyLoaded(false);
	Ocean->Modify();
	if (const ULevel* Level = Ocean->GetLevel())
	{
		(void)Level->MarkPackageDirty();
	}

	// --- WaterBodyActorFactory.cpp의 UWaterBodyActorFactory::PostSpawnActor(...) 참고 ---
	UWaterBodyComponent* WaterBodyComponent = Ocean->GetWaterBodyComponent();
	check(WaterBodyComponent);

	const UWaterEditorSettings* WaterSettings = GetDefault<UWaterEditorSettings>();
	ApplyCommonWaterComponentSettings(
		WaterBodyComponent,
		WaterSettings->WaterBodyOceanDefaults,
		Preset->OceanSettings.OceanWaterMaterial,
		Preset->OceanSettings.OceanWaterStaticMeshMaterial,
		Preset->OceanSettings.WaterHLODMaterial,
		Preset->OceanSettings.UnderwaterPostProcessMaterial
	);

	// --- UWaterBodyOceanActorFactory::PostSpawnActor(...) ---
	if (const UWaterWavesBase* DefaultWaterWaves = GetDefault<UWaterEditorSettings>()->WaterBodyOceanDefaults.WaterWaves)
	{
		UWaterWavesBase* WaterWaves = DuplicateObject(DefaultWaterWaves, Ocean, MakeUniqueObjectName(Ocean, DefaultWaterWaves->GetClass(), TEXT("OceanWaterWaves")));
		Ocean->SetWaterWaves(WaterWaves);
	}

	// Spline의 위치를 FVector::ZeroVector로 하여, 바다가 쫙 깔리게 설정
	WaterBodyComponent->GetWaterSpline()->ResetSpline({ FVector::ZeroVector, FVector::ZeroVector, FVector::ZeroVector });

	if (const AWaterZone* OwningWaterZone = WaterBodyComponent->GetWaterZone())
	{
		if (UWaterBodyOceanComponent* OceanComponent = Cast<UWaterBodyOceanComponent>(WaterBodyComponent))
		{
			const double ExistingCollisionHeight = OceanComponent->GetCollisionExtents().Z;
			OceanComponent->bAffectsLandscape = false;
			OceanComponent->SetCollisionExtents(FVector(OwningWaterZone->GetZoneExtent() / 2.0, ExistingCollisionHeight));
			OceanComponent->FillWaterZoneWithOcean();
		}
	}

	const float OceanSeaHeight = FOCGHeightConverter::GetSeaLevelWorldHeight(Preset) - 5.0f;
	Ocean->SetActorLocation(FVector(VolumeOrigin.X, VolumeOrigin.Y, OceanSeaHeight));

	Ocean->PostEditChange();
	Ocean->PostEditMove(true);

	FOnWaterBodyChangedParams Params;
	Params.bShapeOrPositionChanged = true;
	Params.bUserTriggered = true;
	WaterBodyComponent->UpdateAll(Params);
	WaterBodyComponent->UpdateWaterBodyRenderData();

	CachedOcean = Ocean;
	CachedOceanAsset = Ocean;
}

void UOCGHydrologySubsystem::ClearAllRivers(ALandscape* InLandscape)
{
	UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	const bool bHadRivers = !GeneratedRivers.IsEmpty() || !CachedRivers.IsEmpty();

	for (const TWeakObjectPtr<AWaterBodyRiver>& River : GeneratedRivers)
	{
		if (River.IsValid() && EditorWorld)
		{
			EditorWorld->EditorDestroyActor(River.Get(), true);
		}
	}
	GeneratedRivers.Empty();

	for (TSoftObjectPtr<AWaterBodyRiver>& RiverPtr : CachedRivers)
	{
		const FSoftObjectPath& Path = RiverPtr.ToSoftObjectPath();
		if (!Path.IsValid())
		{
			continue;
		}

		AWaterBodyRiver* River = RiverPtr.IsValid() ? RiverPtr.Get() : Cast<AWaterBodyRiver>(RiverPtr.LoadSynchronous());
		if (River && EditorWorld)
		{
			EditorWorld->EditorDestroyActor(River, true);
		}
	}
	CachedRivers.Empty();

	if (InLandscape)
	{
		const FGuid WaterLayerGuid = FOCGLandscapeUtils::GetLandscapeLayerGuid(InLandscape, FName(TEXT("Water")));
		Compat::ClearEditLayer(InLandscape, WaterLayerGuid);
		InLandscape->RequestLayersContentUpdate(ELandscapeLayerUpdateMode::Update_Heightmap_All);
	}

	if (bHadRivers && EditorWorld)
	{
		if (ULevel* CurrentLevel = EditorWorld->GetCurrentLevel())
		{
			(void)CurrentLevel->MarkPackageDirty();
		}
	}

	bIsRiverExists = false;
}

void UOCGHydrologySubsystem::ExportWaterEditLayerHeightMap(ALandscape* InLandscape, const UMapPreset* Preset, uint16 MinDiffThreshold)
{
	if (!InLandscape)
	{
		return;
	}

	if (!InLandscape->GetLandscapeInfo())
	{
		return;
	}

	const FGuid CurrentLayerGuid = FOCGLandscapeUtils::GetLandscapeLayerGuid(InLandscape, FName(TEXT("Layer")));

	int32 SizeX = 0, SizeY = 0;
	TArray<uint16> BlendedHeightData;
	FOCGLandscapeUtils::ExtractHeightMap(InLandscape, FGuid(), SizeX, SizeY, BlendedHeightData);

	TArray<uint16> BaseLayerHeightData;
	FOCGLandscapeUtils::ExtractHeightMap(InLandscape, CurrentLayerGuid, SizeX, SizeY, BaseLayerHeightData);

	CachedWaterHeightMap.Empty();
	CachedWaterHeightMap.AddZeroed(SizeX * SizeY);
	WaterHeightMapWidth = SizeX;
	WaterHeightMapHeight = SizeY;

	if (BlendedHeightData.Num() == BaseLayerHeightData.Num() && BlendedHeightData.Num() == SizeX * SizeY)
	{
		for (int32 i = 0; i < BlendedHeightData.Num(); ++i)
		{
			const uint16 Diff = static_cast<uint16>(FMath::Abs(static_cast<int32>(BaseLayerHeightData[i]) - static_cast<int32>(BlendedHeightData[i])));
			CachedWaterHeightMap[i] = (Diff > MinDiffThreshold) ? UINT16_MAX : 0;
		}
	}
}

void UOCGHydrologySubsystem::ApplyWaterWeight(ALandscape* InLandscape, const UMapPreset* Preset)
{
	if (!InLandscape)
	{
		return;
	}

	ExportWaterEditLayerHeightMap(InLandscape, Preset, 2);

	UMaterial* LandscapeMaterial = nullptr;
	if (Preset->LandscapeSettings.LandscapeMaterial)
	{
		LandscapeMaterial = Cast<UMaterial>(Preset->LandscapeSettings.LandscapeMaterial->Parent);
	}
	const TArray<FName> LayerNames = FOCGMaterialEditTool::ExtractLandscapeLayerName(LandscapeMaterial);

	const ULandscapeInfo* LandscapeInfo = InLandscape->GetLandscapeInfo();
	if (!LandscapeInfo)
	{
		return;
	}

	ULandscapeLayerInfoObject* FirstLayer = nullptr;
	if (!LayerNames.IsEmpty())
	{
		FirstLayer = LandscapeInfo->GetLayerInfoByName(LayerNames[0]);
	}

	for (const auto& Pair : PrevRiverMaskedWeights)
	{
		ULandscapeLayerInfoObject* LayerInfo = LandscapeInfo->GetLayerInfoByName(Pair.Key);
		if (!LayerInfo)
		{
			continue;
		}

		TArray<uint8> OriginWeightMap;
		FOCGLandscapeUtils::GetWeightMap(InLandscape, LayerInfo, OriginWeightMap);
		FOCGLandscapeUtils::ApplyMaskedWeightMap(InLandscape, LayerInfo, OriginWeightMap, Pair.Value);
	}

	TArray<uint8> WeightMap;
	FOCGLandscapeUtils::MakeWeightMapFromHeightDiff(CachedWaterHeightMap, WeightMap);

	TArray<uint8> BlurredWeightMap;
	FOCGLandscapeUtils::BlurWeightMap(WeightMap, BlurredWeightMap, WaterHeightMapWidth, WaterHeightMapHeight);

	PrevRiverMaskedWeights.Empty();
	for (const FLandscapeInfoLayerSettings& Layer : LandscapeInfo->Layers)
	{
		TArray<uint8> MaskedWeight;
		FOCGLandscapeUtils::GetMaskedWeightMap(InLandscape, Layer.LayerInfoObj, BlurredWeightMap, MaskedWeight);
		PrevRiverMaskedWeights.Add(Layer.LayerName, MoveTemp(MaskedWeight));
	}

	FOCGLandscapeUtils::AddWeightMap(InLandscape, FirstLayer, BlurredWeightMap);
}

// TODO: CreateOcean하고 로직이 겹치는데?
void UOCGHydrologySubsystem::SetDefaultRiverProperties(AWaterBodyRiver* InRiverActor, const TArray<FVector>& InRiverPath, const UMapPreset* Preset)
{
	UWaterBodyComponent* WaterBodyComponent = InRiverActor->GetWaterBodyComponent();
	check(Preset && WaterBodyComponent);

	const UWaterEditorSettings* WaterSettings = GetDefault<UWaterEditorSettings>();
	ApplyCommonWaterComponentSettings(
		WaterBodyComponent,
		WaterSettings->WaterBodyRiverDefaults,
		Preset->RiverSettings.RiverWaterMaterial,
		Preset->RiverSettings.RiverWaterStaticMeshMaterial,
		Preset->RiverSettings.WaterHLODMaterial,
		Preset->RiverSettings.UnderwaterPostProcessMaterial
	);

	// --- UWaterBodyOceanActorFactory::PostSpawnActor(...) ---
	{
		const FWaterBodyRiverDefaults& WaterBodyRiverDefaults = WaterSettings->WaterBodyRiverDefaults;
		UWaterBodyRiverComponent* WaterBodyRiverComponent = CastChecked<UWaterBodyRiverComponent>(WaterBodyComponent);

		UMaterialInterface* CustomLakeTransitionMat = Preset->RiverSettings.RiverToLakeTransitionMaterial.LoadSynchronous();
		WaterBodyRiverComponent->SetLakeTransitionMaterial(CustomLakeTransitionMat ? CustomLakeTransitionMat : WaterBodyRiverDefaults.GetRiverToLakeTransitionMaterial());

		UMaterialInterface* CustomOceanTransitionMat = Preset->RiverSettings.RiverToOceanTransitionMaterial.LoadSynchronous();
		WaterBodyRiverComponent->SetOceanTransitionMaterial(CustomOceanTransitionMat ? CustomOceanTransitionMat : WaterBodyRiverDefaults.GetRiverToOceanTransitionMaterial());
	}

	InRiverActor->PostEditChange();
	InRiverActor->PostEditMove(true);

	UWaterSplineComponent* WaterSpline = WaterBodyComponent->GetWaterSpline();
	WaterSpline->ClearSplinePoints();
	WaterSpline->SetSplinePoints(InRiverPath, ESplineCoordinateSpace::World, true);

	FOnWaterBodyChangedParams Params;
	Params.bShapeOrPositionChanged = true;
	Params.bUserTriggered = true;
	WaterBodyComponent->GetWaterSpline()->GetSplinePointsMetadata();
	WaterBodyComponent->UpdateAll(Params);
	WaterBodyComponent->UpdateWaterBodyRenderData();
}

void UOCGHydrologySubsystem::AddRiverProperties(AWaterBodyRiver* InRiverActor, const TArray<FVector>& InRiverPath, const UMapPreset* Preset) const
{
	if (!InRiverActor || !Preset)
	{
		return;
	}

	UWaterSplineComponent* SplineComp = InRiverActor->GetWaterSpline();
	UWaterBodyRiverComponent* RiverComp = Cast<UWaterBodyRiverComponent>(InRiverActor->GetWaterBodyComponent());
	UWaterSplineMetadata* SplineMetadata = RiverComp ? RiverComp->GetWaterSplineMetadata() : nullptr;

	if (!SplineComp || !RiverComp || !SplineMetadata)
	{
		return;
	}

	const int32 NumPoints = SplineComp->GetNumberOfSplinePoints();
	if (NumPoints < 2)
	{
		return;
	}

	UCurveFloat* WidthCurve = Preset->RiverSettings.RiverWidthCurve;
	UCurveFloat* DepthCurve = Preset->RiverSettings.RiverDepthCurve;
	UCurveFloat* VelocityCurve = Preset->RiverSettings.RiverVelocityCurve;

	float MinWidth = 0.0f, MaxWidth = 1.0f;
	if (WidthCurve)
	{
		WidthCurve->GetValueRange(MinWidth, MaxWidth);
	}

	float MinDepth = 0.0f, MaxDepth = 1.0f;
	if (DepthCurve)
	{
		DepthCurve->GetValueRange(MinDepth, MaxDepth);
	}

	float MinVelocity = 0.0f, MaxVelocity = 1.0f;
	if (VelocityCurve)
	{
		VelocityCurve->GetValueRange(MinVelocity, MaxVelocity);
	}

	const float WidthRange = MaxWidth - MinWidth;
	const float DepthRange = MaxDepth - MinDepth;
	const float VelocityRange = MaxVelocity - MinVelocity;

	for (int32 i = 0; i < NumPoints; ++i)
	{
		const float NormalizedT = (NumPoints > 1) ? static_cast<float>(i) / (NumPoints - 1) : 0.0f;

		auto EvalCurve = [](const UCurveFloat* Curve, float InT, float InMin, float InRange) -> float
		{
			if (!Curve)
			{
				return InT;
			}
			const float RawValue = Curve->GetFloatValue(InT);
			return !FMath::IsNearlyZero(InRange) ? (RawValue - InMin) / InRange : 1.0f;
		};

		const float WidthMultiplier = EvalCurve(WidthCurve, NormalizedT, MinWidth, WidthRange);
		const float DepthMultiplier = EvalCurve(DepthCurve, NormalizedT, MinDepth, DepthRange);
		const float VelocityMultiplier = EvalCurve(VelocityCurve, NormalizedT, MinVelocity, VelocityRange);

		const float DesiredWidth = (Preset->RiverSettings.RiverWidthBaseValue * WidthMultiplier + Preset->RiverSettings.RiverWidthMin) * Preset->LandscapeSettings.LandscapeScale;
		const float DesiredDepth = Preset->RiverSettings.RiverDepthBaseValue * DepthMultiplier + Preset->RiverSettings.RiverDepthMin;
		const float DesiredVelocity = Preset->RiverSettings.RiverVelocityBaseValue * VelocityMultiplier + Preset->RiverSettings.RiverVelocityMin;

		if (SplineMetadata->RiverWidth.Points.IsValidIndex(i))
		{
			SplineMetadata->RiverWidth.Points[i].OutVal = DesiredWidth;
			SplineMetadata->Depth.Points[i].OutVal = DesiredDepth;
			SplineMetadata->WaterVelocityScalar.Points[i].OutVal = DesiredVelocity;
		}

		if (SplineComp->SplineCurves.Scale.Points.IsValidIndex(i))
		{
			SplineComp->SetScaleAtSplinePoint(i, FVector(DesiredWidth, DesiredDepth, 1.0f), ESplineCoordinateSpace::Local);
		}
	}

	SplineComp->UpdateSpline();

	FOnWaterBodyChangedParams Params;
	Params.bShapeOrPositionChanged = true;
	Params.bUserTriggered = true;
	RiverComp->OnWaterBodyChanged(Params);
}

void UOCGHydrologySubsystem::CacheRiverStartPoints(
	const TArray<uint16>& HeightMapData,
	const UMapPreset* Preset,
	float CurMinHeight,
	float CurMaxHeight
)
{
	CachedRiverStartPoints.Empty();

	const float ThresholdMultiplier = FMath::Clamp(Preset->RiverSettings.RiverSourceElevationRatio, 0.0f, 1.0f);
	SeaHeight = CurMinHeight + (CurMaxHeight - CurMinHeight) * Preset->HeightSettings.SeaLevel - 5.0f;
	const float HighThreshold = SeaHeight + (CurMaxHeight - SeaHeight) * ThresholdMultiplier;

	UOCGLandscapeGenSubsystem* LandscapeSubsystem = GEditor->GetEditorSubsystem<UOCGLandscapeGenSubsystem>();

	for (int32 y = 0; y < Preset->LandscapeSettings.MapResolution.Y; ++y)
	{
		for (int32 x = 0; x < Preset->LandscapeSettings.MapResolution.X; ++x)
		{
			const FVector WorldPos = LandscapeSubsystem
				? LandscapeSubsystem->GetLandscapePointWorldPosition(FIntPoint(x, y), Preset, &HeightMapData)
				: FVector::ZeroVector;

			if (WorldPos.Z >= HighThreshold)
			{
				CachedRiverStartPoints.Add(FIntPoint(x, y));
			}
		}
	}
}

FIntPoint UOCGHydrologySubsystem::GetRandomStartPoint(int32 RiverIndex, const UMapPreset* Preset) const
{
	FRandomStream Stream(Preset->RiverSettings.RiverSeed + RiverIndex * 9973);

	if (CachedRiverStartPoints.Num() > 0)
	{
		return CachedRiverStartPoints[Stream.RandRange(0, CachedRiverStartPoints.Num() - 1)];
	}
	return FIntPoint(Preset->LandscapeSettings.MapResolution.X / 2, Preset->LandscapeSettings.MapResolution.Y - 1);
}

void UOCGHydrologySubsystem::SimplifyPathRDP(const TArray<FVector>& InPoints, TArray<FVector>& OutPoints, float Epsilon)
{
	if (InPoints.Num() < 3)
	{
		OutPoints = InPoints;
		return;
	}

	const int32 LastIndex = InPoints.Num() - 1;
	float MaxDist = 0.0f;
	int32 SplitIndex = 0;

	for (int32 i = 1; i < LastIndex; ++i)
	{
		const float PointDist = FMath::PointDistToSegment(InPoints[i], InPoints[0], InPoints[LastIndex]);
		if (PointDist > MaxDist)
		{
			SplitIndex = i;
			MaxDist = PointDist;
		}
	}

	if (MaxDist > Epsilon)
	{
		TArray<FVector> LeftResult, RightResult;
		SimplifyPathRDP(TArray<FVector>(InPoints.GetData(), SplitIndex + 1), LeftResult, Epsilon);
		SimplifyPathRDP(TArray<FVector>(InPoints.GetData() + SplitIndex, InPoints.Num() - SplitIndex), RightResult, Epsilon);
		OutPoints.Append(LeftResult.GetData(), LeftResult.Num() - 1);
		OutPoints.Append(RightResult);
	}
	else
	{
		OutPoints.Add(InPoints[0]);
		OutPoints.Add(InPoints[LastIndex]);
	}
}
