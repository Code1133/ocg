// Copyright (c) 2025 Code1133. All rights reserved.


#include "Utils/OCGLandscapeUtil.h"

#include <OCGLog.h>

#include "OCGLevelGenerator.h"
#include "PCGComponent.h"
#include "Component/OCGLandscapeGenerateComponent.h"
#include "Component/OCGRiverGeneratorComponent.h"
#include "Data/MapData.h"
#include "Data/MapPreset.h"
#include "PCG/OCGLandscapeVolume.h"
#include "Utils/OCGMaterialEditTool.h"
#include "Utils/OCGUtils.h"

#if WITH_EDITOR
#include "Landscape.h"
#include "LandscapeEdit.h"
#include "LandscapeInfo.h"
#include "LandscapeSettings.h"

#include "LandscapeSubsystem.h"
#include "Builders/CubeBuilder.h"
#include "LocationVolume.h"

#include "LandscapeStreamingProxy.h"

#include "ActorFactories/ActorFactory.h"
#include "ActorPartition/ActorPartitionSubsystem.h"
#include "WorldPartition/WorldPartition.h"

#include "ObjectTools.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Utils/OCGFileUtils.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 5
#include "LandscapeEditLayer.h"
#endif
#endif

FString OCGLandscapeUtil::LayerInfoSavePath = TEXT("/Game/Landscape/LayerInfos");

static int32 NumLandscapeRegions(const ULandscapeInfo* InLandscapeInfo)
{
#if WITH_EDITOR
	int32 NumRegions = 0;
	TArray<AActor*> Children;
	InLandscapeInfo->LandscapeActor->GetAttachedActors(Children);
	TArray<ALocationVolume*> LandscapeRegions;

	for (AActor* Child : Children)
	{
		NumRegions += Child->IsA<ALocationVolume>() ? 1 : 0;
	}

	return NumRegions;
#endif
}

OCGLandscapeUtil::OCGLandscapeUtil()
{
}

OCGLandscapeUtil::~OCGLandscapeUtil()
{
}

void OCGLandscapeUtil::ExtractHeightMap(ALandscape* InLandscape, const FGuid InGuid, int32& OutWidth, int32& OutHeight,
                                        TArray<uint16>& OutHeightMap)
{
#if WITH_EDITOR
	if (InLandscape)
	{
		ULandscapeInfo* Info = InLandscape->GetLandscapeInfo();
		if (!Info)
		{
			return;
		}

		FScopedSetLandscapeEditingLayer Scope(InLandscape, InGuid);

		FIntRect ExportRegion;
		if (Info->GetLandscapeExtent(ExportRegion))
		{
			FLandscapeEditDataInterface LandscapeEdit(Info);

			OutWidth = ExportRegion.Width() + 1;
			OutHeight = ExportRegion.Height() + 1;

			OutHeightMap.AddZeroed(OutWidth * OutHeight);
			LandscapeEdit.GetHeightDataFast(ExportRegion.Min.X, ExportRegion.Min.Y, ExportRegion.Max.X, ExportRegion.Max.Y, OutHeightMap.GetData(), 0);
		}
	}
#endif

}

void OCGLandscapeUtil::AddWeightMap(ALandscape* InLandscape, const int32 InTargetLayerIndex, const TArray<uint8>& InWeightMap)
{
	if (InLandscape)
	{
		ULandscapeInfo* LandscapeInfo = InLandscape->GetLandscapeInfo();
		if (!LandscapeInfo) return;

		ULandscapeLayerInfoObject* LayerInfo = LandscapeInfo->Layers[InTargetLayerIndex].LayerInfoObj;
		
		AddWeightMap(InLandscape, LayerInfo, InWeightMap);
	}
}

void OCGLandscapeUtil::AddWeightMap(ALandscape* InLandscape, ULandscapeLayerInfoObject* InLayerInfo,
	const TArray<uint8>& InWeightMap)
{
#if WITH_EDITOR
	if (InWeightMap.IsEmpty())
		return;

	if (InLayerInfo == nullptr)
		return;
	
	if (InLandscape)
	{
		FGuid CurrentLayerGuid = GetLandscapeLayerGuid(InLandscape, TEXT("Layer"));
		ULandscapeInfo* LandscapeInfo = InLandscape->GetLandscapeInfo();
		if (!LandscapeInfo) return;

		FIntRect Region;
		if (LandscapeInfo->GetLandscapeExtent(Region))
		{
			FScopedSetLandscapeEditingLayer Scope(InLandscape, CurrentLayerGuid, [InLandscape]
			{
				check(InLandscape);
				InLandscape->RequestLayersContentUpdate(ELandscapeLayerUpdateMode::Update_Weightmap_All);
			});

			FAlphamapAccessor<false, false> AlphamapAccessor(LandscapeInfo, InLayerInfo);
			int32 RegionWidth = Region.Width() + 1;
			int32 RegionHeight = Region.Height() + 1;
			int32 NumPixels = RegionWidth * RegionHeight;

			TArray<uint8> OriginWeightMap;
			GetWeightMap(InLandscape, InLayerInfo, OriginWeightMap);
			TArray<uint8> TargetWeightData;
			TargetWeightData.AddZeroed(RegionWidth * RegionHeight);

			for (int32 i = 0; i < NumPixels; ++i)
			{
				float Origin = OriginWeightMap[i] / 255.f;
				float New = 0;

				if (i < InWeightMap.Num())
				{
					New = FMath::Clamp(InWeightMap[i] / 255.f, 0.f, 1.f);
				}

				float Final = FMath::Clamp(Origin + New, 0.f, 1.f);    // 더하기 방식 블렌드
				TargetWeightData[i] = static_cast<uint8>(Final * 255.f);
			}

			AlphamapAccessor.SetData(Region.Min.X, Region.Min.Y, Region.Max.X, Region.Max.Y,
									 TargetWeightData.GetData(), ELandscapeLayerPaintingRestriction::None);

			LandscapeInfo->ForceLayersFullUpdate();

			InLandscape->ReregisterAllComponents();
		}
	}
#endif
}

void OCGLandscapeUtil::ApplyWeightMap(ALandscape* InLandscape, const int32 InTargetLayerIndex, const TArray<uint8>& InWeightMap)
{
#if WITH_EDITOR
	if (InWeightMap.IsEmpty())
		return;
	
	if (InLandscape)
	{
		ULandscapeInfo* LandscapeInfo = InLandscape->GetLandscapeInfo();
		if (!LandscapeInfo) return;

		ULandscapeLayerInfoObject* LayerInfo = LandscapeInfo->Layers[InTargetLayerIndex].LayerInfoObj;

		ApplyWeightMap(InLandscape, LayerInfo, InWeightMap);
	}
#endif
}

void OCGLandscapeUtil::ApplyWeightMap(ALandscape* InLandscape, ULandscapeLayerInfoObject* InLayerInfo,
	const TArray<uint8>& InWeightMap)
{
#if WITH_EDITOR
	if (InWeightMap.IsEmpty())
		return;

	if (InLayerInfo == nullptr)
		return;

	if (InLandscape)
	{
		FGuid CurrentLayerGuid = GetLandscapeLayerGuid(InLandscape, TEXT("Layer"));
		ULandscapeInfo* LandscapeInfo = InLandscape->GetLandscapeInfo();
		if (!LandscapeInfo) return;

		FIntRect Region;
		if (LandscapeInfo->GetLandscapeExtent(Region))
		{
			Region.Max.X += 1;
			Region.Max.Y += 1;

			FScopedSetLandscapeEditingLayer Scope(InLandscape, CurrentLayerGuid, [InLandscape]
			{
				check(InLandscape);
				InLandscape->RequestLayersContentUpdate(ELandscapeLayerUpdateMode::Update_Weightmap_All);
			});

			FAlphamapAccessor<false, false> AlphamapAccessor(LandscapeInfo, InLayerInfo);
			AlphamapAccessor.SetData(Region.Min.X, Region.Min.Y, Region.Max.X - 1, Region.Max.Y - 1, InWeightMap.GetData(), ELandscapeLayerPaintingRestriction::None);
			LandscapeInfo->ForceLayersFullUpdate();

			InLandscape->ReregisterAllComponents();

			OCGMapDataUtils::ExportMap(InWeightMap, FIntPoint(Region.Max.X - 1, Region.Max.Y - 1), TEXT("OriginWeightMap.png"));
		}
	}
#endif
}

void OCGLandscapeUtil::ApplyMaskedWeightMap(ALandscape* InLandscape, ULandscapeLayerInfoObject* InLayerInfo, const TArray<uint8>& OriginWeightMap,
	const TArray<uint8>& InMaskedWeightMap)
{
#if WITH_EDITOR
	if (InMaskedWeightMap.IsEmpty())
		return;

	if (InLayerInfo == nullptr)
		return;
	
	if (InLandscape)
	{
		FGuid CurrentLayerGuid = GetLandscapeLayerGuid(InLandscape, FName(TEXT("Layer")));
		
		ULandscapeInfo* LandscapeInfo = InLandscape->GetLandscapeInfo();
		if (!LandscapeInfo) return;
		
		FIntRect Region;
		if (LandscapeInfo->GetLandscapeExtent(Region))
		{
			FScopedSetLandscapeEditingLayer Scope(InLandscape, CurrentLayerGuid, [InLandscape]
			{
				check(InLandscape);
				InLandscape->RequestLayersContentUpdate(ELandscapeLayerUpdateMode::Update_Weightmap_All);
			});

			FAlphamapAccessor<false, false> AlphamapAccessor(LandscapeInfo, InLayerInfo);
			int32 RegionWidth = Region.Width() + 1;
			int32 RegionHeight = Region.Height() + 1;
			int32 NumPixels = RegionWidth * RegionHeight;

			if (InMaskedWeightMap.Num() != NumPixels)
			{
				UE_LOG(LogOCGModule, Warning, TEXT("Mask Resolution != %d"), NumPixels);
				return;
			}

			TArray<uint8> FinalWeightMap;
			FinalWeightMap.SetNum(NumPixels);

			const uint8* MaskData   = InMaskedWeightMap.GetData();
			const uint8* OriginData = OriginWeightMap.GetData();
			uint8*       FinalData  = FinalWeightMap.GetData();
			
			for (int32 i = 0; i < NumPixels; ++i)
			{
				FinalData[i] = (MaskData[i] != 0) ? MaskData[i] : OriginData[i];
			}

			AlphamapAccessor.SetData(Region.Min.X, Region.Min.Y, Region.Max.X, Region.Max.Y, FinalWeightMap.GetData(), ELandscapeLayerPaintingRestriction::None);
			
			LandscapeInfo->ForceLayersFullUpdate();

			InLandscape->ReregisterAllComponents();
		}
	}
#endif
}

void OCGLandscapeUtil::GetWeightMap(ALandscape* InLandscape, const int32 InTargetLayerIndex, TArray<uint8>& OutOriginWeightMap)
{
#if	WITH_EDITOR
	if (InLandscape)
	{
		ULandscapeInfo* LandscapeInfo = InLandscape->GetLandscapeInfo();
		if (!LandscapeInfo) return;

		ULandscapeLayerInfoObject* LayerInfo = LandscapeInfo->Layers[InTargetLayerIndex].LayerInfoObj;
		
		if (!LayerInfo) return;

		GetWeightMap(InLandscape, LayerInfo, OutOriginWeightMap);
	}
#endif
}

void OCGLandscapeUtil::GetWeightMap(ALandscape* InLandscape, ULandscapeLayerInfoObject* InLayerInfo,
	TArray<uint8>& OutOriginWeightMap)
{
#if	WITH_EDITOR
	if (InLandscape)
	{
		FGuid CurrentLayerGuid = GetLandscapeLayerGuid(InLandscape, TEXT("Layer"));
		ULandscapeInfo* LandscapeInfo = InLandscape->GetLandscapeInfo();
		if (!LandscapeInfo) return;
		
		if (!InLayerInfo) return;

		FIntRect Region;
		if (LandscapeInfo->GetLandscapeExtent(Region))
		{
			FScopedSetLandscapeEditingLayer Scope(InLandscape, CurrentLayerGuid, [InLandscape]
			{
				check(InLandscape);
				InLandscape->RequestLayersContentUpdate(ELandscapeLayerUpdateMode::Update_Weightmap_All);
			});

			FAlphamapAccessor<false, false> AlphamapAccessor(LandscapeInfo, InLayerInfo);
			int32 RegionWidth = Region.Width() + 1; 
			int32 RegionHeight = Region.Height() + 1;
			if (!OutOriginWeightMap.IsEmpty())
				OutOriginWeightMap.Empty();

			OutOriginWeightMap.AddZeroed(RegionWidth * RegionHeight);

			AlphamapAccessor.GetDataFast(Region.Min.X, Region.Min.Y, Region.Max.X, Region.Max.Y, OutOriginWeightMap.GetData());
		}
	}
#endif
}

void OCGLandscapeUtil::GetMaskedWeightMap(ALandscape* InLandscape, const int32 InTargetLayerIndex, const TArray<uint8>& Mask, TArray<uint8>& OutWeightMap)
{
#if WITH_EDITOR
	if (InLandscape)
	{
		ULandscapeInfo* LandscapeInfo = InLandscape->GetLandscapeInfo();
		if (!LandscapeInfo) return;

		ULandscapeLayerInfoObject* LayerInfo = LandscapeInfo->Layers[InTargetLayerIndex].LayerInfoObj;
		
		if (!LayerInfo) return;

		GetMaskedWeightMap(InLandscape, LayerInfo, Mask, OutWeightMap);
	}
#endif
}

void OCGLandscapeUtil::GetMaskedWeightMap(ALandscape* InLandscape, ULandscapeLayerInfoObject* InLayerInfo,
	const TArray<uint8>& Mask, TArray<uint8>& OutWeightMap)
{
#if WITH_EDITOR
	GetWeightMap(InLandscape, InLayerInfo, OutWeightMap);

	if (OutWeightMap.IsEmpty())
	{
		UE_LOG(LogOCGModule, Warning, TEXT("GetMaskedWeightMap : %s's WeightMap is empty"), *InLayerInfo->LayerName.ToString());
		return;
	}

	if (Mask.IsEmpty())
	{
		UE_LOG(LogOCGModule, Warning, TEXT("Mask is empty"));
		return;
	}

	if (OutWeightMap.Num() != Mask.Num())
	{
		UE_LOG(LogOCGModule, Warning, TEXT("%s: WeightMap size (%d) != Mask size (%d)"),
		       *InLayerInfo->LayerName.ToString(), OutWeightMap.Num(), Mask.Num());
		return;
	}

	uint8*       OutData  = OutWeightMap.GetData();
	const uint8* MaskData = Mask.GetData();
	const int32  Num      = Mask.Num();
	
	for (int32 i = 0; i < Num; ++i)
	{
		OutData[i] = OutData[i] * static_cast<uint8>(MaskData[i] != 0);
	}

#endif
}

void OCGLandscapeUtil::CleanUpWeightMap(ALandscape* InLandscape)
{
#if	WITH_EDITOR
	if (InLandscape)
	{
		ULandscapeInfo* LandscapeInfo = InLandscape->GetLandscapeInfo();
		if (!LandscapeInfo) return;

		for (int32 LayerIndex = 1; LayerIndex < LandscapeInfo->Layers.Num(); ++LayerIndex)
		{
			TArray<uint8> LayerWeightMap;

			GetWeightMap(InLandscape, LayerIndex, LayerWeightMap);
			ApplyWeightMap(InLandscape, LayerIndex, LayerWeightMap);
		}
	}
#endif
}

void OCGLandscapeUtil::MakeWeightMapFromHeightDiff(const TArray<uint16>& HeightDiff, TArray<uint8>& OutWeight, const uint16 MinDiffThreshold)
{
#if WITH_EDITOR
	const int32 Num       = HeightDiff.Num();
	OutWeight.SetNumUninitialized(Num);

	const uint16* HeightData = HeightDiff.GetData();
	uint8*       OutData     = OutWeight.GetData();
	
	for (int32 i = 0; i < Num; ++i)
	{
		// MinDiffThreshold 초과분만 255, 그 외는 0
		OutData[i] = static_cast<uint8>((HeightData[i] > MinDiffThreshold) * 255);
	}
#endif
}

void OCGLandscapeUtil::BlurWeightMap(const TArray<uint8>& InWeight, TArray<uint8>& OutWeight, const int32 Width, const int32 Height)
{
	const int32 Num = Width * Height;
	OutWeight.SetNumUninitialized(Num);
	
	TArray<float> TempAccum;
	TempAccum.SetNumZeroed(Num);
	
	const uint8* InData   = InWeight.GetData();
	float*       TempData = TempAccum.GetData();
	
	for (int32 y = 0; y < Height; ++y)
	{
		const int32 Base = y * Width;
		for (int32 x = 0; x < Width; ++x)
		{
			float Sum = InData[Base + x];
			if (x > 0)         Sum += InData[Base + x - 1];
			if (x < Width - 1) Sum += InData[Base + x + 1];
			TempData[Base + x] = Sum / 3.0f;
		}
	}

	for (int32 x = 0; x < Width; ++x)
	{
		for (int32 y = 0; y < Height; ++y)
		{
			const int32 Idx = y * Width + x;
			float Sum = TempData[Idx];
			if (y > 0)          Sum += TempData[Idx - Width];
			if (y < Height - 1) Sum += TempData[Idx + Width];
			const int32 Val = FMath::RoundToInt(Sum / 3.0f);
			OutWeight[Idx] = static_cast<uint8>(FMath::Clamp(Val, 0, 255));
		}
	}
}

void OCGLandscapeUtil::ClearTargetLayers(const ALandscape* InLandscape)
{
	if (InLandscape == nullptr) return;
	ULandscapeInfo* LandscapeInfo = InLandscape->GetLandscapeInfo();

	TArray<FName> LayerNames;
	InLandscape->GetTargetLayers().GetKeys(LayerNames);

	for (const FName& LayerName : LayerNames)
	{
		if (LandscapeInfo)
		{
			if (InLandscape->GetTargetLayers().Contains(LayerName))
			{
				LandscapeInfo->DeleteLayer(InLandscape->GetTargetLayers().FindRef(LayerName).LayerInfoObj, LayerName);
			}
		}
	}

}

void OCGLandscapeUtil::UpdateTargetLayers(ALandscape* InLandscape,
	const TMap<FGuid, TArray<FLandscapeImportLayerInfo>>& MaterialLayerDataPerLayers)
{
#if WITH_EDITOR
	if (!InLandscape) return;
	ULandscapeInfo* LandscapeInfo = InLandscape->GetLandscapeInfo();
	if (LandscapeInfo)
	{
		LandscapeInfo->UpdateLayerInfoMap(InLandscape);
	}

	// Assume single entry in map
	const TArray<FLandscapeImportLayerInfo>& ImportInfos = MaterialLayerDataPerLayers.begin()->Value;
	const ULandscapeSettings* Settings = GetDefault<ULandscapeSettings>();
	const ULandscapeLayerInfoObject* DefaultLayerInfo = Settings->GetDefaultLayerInfoObject().LoadSynchronous();

	for (const FLandscapeImportLayerInfo& ImportInfo : ImportInfos)
	{
		ULandscapeLayerInfoObject* LayerInfoObj = ImportInfo.LayerInfo;
		const FName LayerName = ImportInfo.LayerName;
		if (!LayerInfoObj)
		{
			// TODO: CreateLayerInfo API 변경필요 (5.6에서 UE::Landscape::CreateTargetLayerInfo을 대신 사용)
			LayerInfoObj = InLandscape->CreateLayerInfo(*LayerName.ToString(), DefaultLayerInfo);
			if (LayerInfoObj)
			{
				LayerInfoObj->LayerUsageDebugColor = LayerInfoObj->GenerateLayerUsageDebugColor();
				(void)LayerInfoObj->MarkPackageDirty();
			}
		}

		if (LayerInfoObj)
		{
			if (!InLandscape->GetTargetLayers().Contains(LayerName))
			{
				InLandscape->AddTargetLayer(LayerName, FLandscapeTargetLayerSettings(LayerInfoObj));
				if (LandscapeInfo)
				{
					const int32 Index = LandscapeInfo->GetLayerInfoIndex(LayerName);
					if (Index != INDEX_NONE)
					{
						FLandscapeInfoLayerSettings& LayerSettings = LandscapeInfo->Layers[Index];
						LayerSettings.LayerInfoObj = LayerInfoObj;
					}
				}
			}
			else
			{
				InLandscape->UpdateTargetLayer(LayerName, FLandscapeTargetLayerSettings(LayerInfoObj));
				if (LandscapeInfo)
				{
					const int32 Index = LandscapeInfo->GetLayerInfoIndex(LayerName);
					if (Index != INDEX_NONE)
					{
						FLandscapeInfoLayerSettings& LayerSettings = LandscapeInfo->Layers[Index];
						LayerSettings.LayerInfoObj = LayerInfoObj;
					}
				}
			}
		}
		else
		{
			InLandscape->AddTargetLayer(LayerName, FLandscapeTargetLayerSettings());
		}
	}
#endif
}

void OCGLandscapeUtil::AddTargetLayers(ALandscape* InLandscape,
	const TMap<FGuid, TArray<FLandscapeImportLayerInfo>>& MaterialLayerDataPerLayers)
{
#if WITH_EDITOR
	if (!InLandscape) return;
	ULandscapeInfo* LandscapeInfo = InLandscape->GetLandscapeInfo();
	if (LandscapeInfo)
	{
		LandscapeInfo->UpdateLayerInfoMap(InLandscape);
	}

	// Assume single entry in map
	const TArray<FLandscapeImportLayerInfo>& ImportInfos = MaterialLayerDataPerLayers.begin()->Value;
	const ULandscapeSettings* Settings = GetDefault<ULandscapeSettings>();
	const ULandscapeLayerInfoObject* DefaultLayerInfo = Settings->GetDefaultLayerInfoObject().LoadSynchronous();

	for (const FLandscapeImportLayerInfo& ImportInfo : ImportInfos)
	{
		ULandscapeLayerInfoObject* LayerInfoObj = ImportInfo.LayerInfo;
		const FName LayerName = ImportInfo.LayerName;
		if (!LayerInfoObj)
		{
			// TODO: CreateLayerInfo API 변경필요 (5.6에서 UE::Landscape::CreateTargetLayerInfo을 대신 사용)
			LayerInfoObj = InLandscape->CreateLayerInfo(*LayerName.ToString(), DefaultLayerInfo);
			if (LayerInfoObj)
			{
				LayerInfoObj->LayerUsageDebugColor = LayerInfoObj->GenerateLayerUsageDebugColor();
				(void)LayerInfoObj->MarkPackageDirty();
			}
		}

		if (LayerInfoObj)
		{
			InLandscape->AddTargetLayer(LayerName, FLandscapeTargetLayerSettings(LayerInfoObj));
			if (LandscapeInfo)
			{
				const int32 Index = LandscapeInfo->GetLayerInfoIndex(LayerName);
				if (Index != INDEX_NONE)
				{
					FLandscapeInfoLayerSettings& LayerSettings = LandscapeInfo->Layers[Index];
					LayerSettings.LayerInfoObj = LayerInfoObj;
				}
			}
		}
		else
		{
			// Add a target layer with no layer info asset
			InLandscape->AddTargetLayer(LayerName, FLandscapeTargetLayerSettings());
		}
	}
#endif
}

void OCGLandscapeUtil::ManageLandscapeRegions(UWorld* World, const ALandscape* Landscape, UMapPreset* InMapPreset,
	const FLandscapeSetting& InLandscapeSetting)
{
	#if WITH_EDITOR
    ULandscapeInfo* LandscapeInfo = Landscape->GetLandscapeInfo();
    ALandscapeProxy* LandscapeProxy = nullptr;
	if (InMapPreset == nullptr)
	{
		return;
	}

    ULandscapeSubsystem* LandscapeSubsystem = World->GetSubsystem<ULandscapeSubsystem>();
    ChangeGridSize(World, LandscapeInfo, InLandscapeSetting.WorldPartitionGridSize);

    if (LandscapeInfo)
    {
        LandscapeProxy = LandscapeInfo->GetLandscapeProxy();
    }

	bool bIsWorldPartition = false;
	if (World && World->GetSubsystem<ULandscapeSubsystem>())
	{
		bIsWorldPartition = World->GetSubsystem<ULandscapeSubsystem>()->IsGridBased();
	}

	bool bLandscapeLargerThanRegion = false;

	if (InMapPreset)
	{
		bLandscapeLargerThanRegion = static_cast<int32>(InLandscapeSetting.WorldPartitionRegionSize) < InMapPreset->Landscape_ComponentCount.X || static_cast<int32>(InLandscapeSetting.WorldPartitionRegionSize) < InMapPreset->Landscape_ComponentCount.Y;
	}
	const bool bNeedsLandscapeRegions = bIsWorldPartition && bLandscapeLargerThanRegion;

    if (bNeedsLandscapeRegions)
    {
    	TArray<FIntPoint> NewComponents;
    	NewComponents.Empty(InLandscapeSetting.ComponentCountX * InLandscapeSetting.ComponentCountY);
    	for (int32 Y = 0; Y < InLandscapeSetting.ComponentCountY; Y++)
    	{
    		for (int32 X = 0; X < InLandscapeSetting.ComponentCountX; X++)
    		{
    			NewComponents.Add(FIntPoint(X, Y));
    		}
    	}

    	TArray<ALocationVolume*> RegionVolumes;
    	FBox LandscapeBounds;
    	auto AddComponentsToRegion = [World, LandscapeProxy, LandscapeInfo, LandscapeSubsystem, &RegionVolumes, &LandscapeBounds, InMapPreset, InLandscapeSetting](const FIntPoint& RegionCoordinate, const TArray<FIntPoint>& NewComponents)
    	{
    		TRACE_CPUPROFILER_EVENT_SCOPE(AddComponentsToRegion);

    		const int32 RegionSizeTexels = InLandscapeSetting.QuadsPerSection * InMapPreset->WorldPartitionRegionSize * InMapPreset->Landscape_SectionsPerComponent;
    		const double RegionSizeX = RegionSizeTexels * LandscapeProxy->GetActorScale3D().X;

		    if (ALocationVolume* RegionVolume = CreateLandscapeRegionVolume(World, LandscapeProxy, RegionCoordinate, RegionSizeX))
    		{
    			RegionVolumes.Add(RegionVolume);
    		}

    		TArray<ALandscapeProxy*> CreatedStreamingProxies;
    		AddLandscapeComponent(LandscapeInfo, LandscapeSubsystem, NewComponents, CreatedStreamingProxies);

    		// ensures all the final height textures have been updated.
    		LandscapeInfo->ForceLayersFullUpdate();

    		SaveLandscapeProxies(World, MakeArrayView(CreatedStreamingProxies));
    		LandscapeBounds += LandscapeInfo->GetCompleteBounds();

    		return true;
    	};

    	SaveObjects(MakeArrayView(MakeArrayView(TArrayView<ALandscape*>(TArray<ALandscape*> { LandscapeInfo->LandscapeActor.Get() }))));

    	ForEachComponentByRegion(InMapPreset->WorldPartitionRegionSize, NewComponents, AddComponentsToRegion);

    	for (ALocationVolume* RegionVolume : RegionVolumes)
    	{
    		const FVector Scale = RegionVolume->GetActorScale();
    		const FVector NewScale{ Scale.X, Scale.Y, 1000000.0f}; //ZScale * 10.0
    		RegionVolume->SetActorScale3D(NewScale);
    	}

    	SaveObjects(MakeArrayView(RegionVolumes));
    	TArray<ALandscapeProxy*> AllProxies;

    	// save the initial region & unload it
    	LandscapeInfo->ForEachLandscapeProxy([&AllProxies](ALandscapeProxy* Proxy) {
			if (Proxy->IsA<ALandscapeStreamingProxy>())
			{
				AllProxies.Add(Proxy);

			}
			return true;
		});

    	SaveLandscapeProxies(World, MakeArrayView(AllProxies));
    }
#endif
}

void OCGLandscapeUtil::ImportMapDatas(UWorld* World, ALandscape* InLandscape, TArray<uint16> ImportHeightMap,
                                      TArray<FLandscapeImportLayerInfo> ImportLayers)
{
	#if WITH_EDITOR
	if (World == nullptr)
		return;

	if (InLandscape == nullptr)
		return;

	if (ULandscapeInfo* LandscapeInfo = InLandscape->GetLandscapeInfo())
	{
		FIntRect LandscapeExtent;
		LandscapeInfo->GetLandscapeExtent(LandscapeExtent);

		LandscapeExtent.Max.X += 1;
		LandscapeExtent.Max.Y += 1;

		FIntRect ImportRegion = LandscapeExtent;

		FGuid CurrentLayerGuid = GetLandscapeLayerGuid(InLandscape, TEXT("Layer"));
		constexpr ELandscapeLayerPaintingRestriction PaintRestriction = ELandscapeLayerPaintingRestriction::None;
		const bool bIsWorldPartition = World->GetSubsystem<ULandscapeSubsystem>()->IsGridBased();
		if (bIsWorldPartition && NumLandscapeRegions(LandscapeInfo) > 0)
		{
			TArray<ALocationVolume*> LandscapeRegions;
			TArray<AActor*> Children;
			LandscapeInfo->LandscapeActor->GetAttachedActors(Children);
			for (AActor* Child : Children)
			{
				if (Child->IsA<ALocationVolume>())
				{
					LandscapeRegions.Add(Cast<ALocationVolume>(Child));
				}
			}

			int32 NumRegions = LandscapeRegions.Num();

			FScopedSlowTask Progress(static_cast<float>(NumRegions), NSLOCTEXT("ONEBUTTONLEVELGENERATION_API", "Importing Landscape Regions", "Importing Landscape Regions"));
			Progress.MakeDialog(/*bShowCancelButton = */ false);

			auto RegionImporter = [&ImportHeightMap, &ImportLayers, &Progress, LandscapeInfo, CurrentLayerGuid, PaintRestriction](const FBox& RegionBounds, const TArray<ALandscapeProxy*>& Proxies)
			{
				FIntRect LandscapeLoadedExtent;
				LandscapeInfo->GetLandscapeExtent(LandscapeLoadedExtent);
				LandscapeLoadedExtent.Max.X += 1;
				LandscapeLoadedExtent.Max.Y += 1;

				Progress.EnterProgressFrame(1.0f, NSLOCTEXT("ONEBUTTONLEVELGENERATION_API", "Importing Landscape Regions", "Importing Landscape Regions"));
				{
					ALandscape* Landscape = LandscapeInfo->LandscapeActor.Get();
					FScopedSetLandscapeEditingLayer Scope(Landscape, CurrentLayerGuid, [&] { check(Landscape); Landscape->RequestLayersContentUpdate(ELandscapeLayerUpdateMode::Update_Heightmap_All); });
					FHeightmapAccessor<false> HeightmapAccessor(LandscapeInfo);
					HeightmapAccessor.SetData(LandscapeLoadedExtent.Min.X, LandscapeLoadedExtent.Min.Y, LandscapeLoadedExtent.Max.X - 1, LandscapeLoadedExtent.Max.Y - 1, ImportHeightMap.GetData());
				}

				for (const FLandscapeImportLayerInfo& ImportLayer : ImportLayers)
				{
					ALandscape* Landscape = LandscapeInfo->LandscapeActor.Get();
					FScopedSetLandscapeEditingLayer Scope(Landscape, CurrentLayerGuid, [&] { check(Landscape); Landscape->RequestLayersContentUpdate(ELandscapeLayerUpdateMode::Update_Weightmap_All); });
					FAlphamapAccessor<false, false> AlphamapAccessor(LandscapeInfo, ImportLayer.LayerInfo);
					AlphamapAccessor.SetData(LandscapeLoadedExtent.Min.X, LandscapeLoadedExtent.Min.Y, LandscapeLoadedExtent.Max.X - 1, LandscapeLoadedExtent.Max.Y - 1, ImportLayer.LayerData.GetData(), PaintRestriction);
				}

				return !Progress.ShouldCancel();
			};

			ForEachRegion_LoadProcessUnload(LandscapeInfo, ImportRegion, World, RegionImporter);
		}
		else
		{
			FScopedSlowTask Progress(static_cast<float>(1 + ImportLayers.Num()), NSLOCTEXT("ONEBUTTONLEVELGENERATION_API", "ImportingLandscape", "Importing Landscape"));
			Progress.MakeDialog(/*bShowCancelButton = */ false);

			{
				Progress.EnterProgressFrame(1.0f, NSLOCTEXT("ONEBUTTONLEVELGENERATION_API", "ImportingLandscapeHeight", "Importing Landscape Height"));
				ALandscape* Landscape = LandscapeInfo->LandscapeActor.Get();
				FScopedSetLandscapeEditingLayer Scope(Landscape, CurrentLayerGuid, [&] { check(Landscape); Landscape->RequestLayersContentUpdate(ELandscapeLayerUpdateMode::Update_Heightmap_All); });
				FHeightmapAccessor<false> HeightmapAccessor(LandscapeInfo);
				HeightmapAccessor.SetData(ImportRegion.Min.X, ImportRegion.Min.Y, ImportRegion.Max.X - 1, ImportRegion.Max.Y - 1, ImportHeightMap.GetData());
			}

			for (const FLandscapeImportLayerInfo& ImportLayer : ImportLayers)
			{
				Progress.EnterProgressFrame(1.0f, NSLOCTEXT("ONEBUTTONLEVELGENERATION_API", "ImportingLandscapeWeight", "Importing Landscape Weight"));

				ALandscape* Landscape = LandscapeInfo->LandscapeActor.Get();
				FScopedSetLandscapeEditingLayer Scope(Landscape, CurrentLayerGuid, [&] { check(Landscape); Landscape->RequestLayersContentUpdate(ELandscapeLayerUpdateMode::Update_Weightmap_All); });
				FAlphamapAccessor<false, false> AlphamapAccessor(LandscapeInfo, ImportLayer.LayerInfo);
				AlphamapAccessor.SetData(ImportRegion.Min.X, ImportRegion.Min.Y, ImportRegion.Max.X - 1, ImportRegion.Max.Y - 1, ImportLayer.LayerData.GetData(), PaintRestriction);
			}

			LandscapeInfo->ForceLayersFullUpdate();
		}
	}
#endif
}

bool OCGLandscapeUtil::ChangeGridSize(const UWorld* InWorld, ULandscapeInfo* InLandscapeInfo,
                                      uint32 InNewGridSizeInComponents)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UOCGLandscapeGenerateComponent::ChangeGridSize);

	if (InWorld == nullptr)
		return false;

	ULandscapeSubsystem* LandscapeSubsystem = InWorld->GetSubsystem<ULandscapeSubsystem>();
	if (LandscapeSubsystem == nullptr || !LandscapeSubsystem->IsGridBased())
	{
		return false;
	}

	const uint32 GridSize = InLandscapeInfo->GetGridSize(InNewGridSizeInComponents);

	InLandscapeInfo->LandscapeActor->Modify();
	InLandscapeInfo->LandscapeActor->SetGridSize(GridSize);
	InLandscapeInfo->LandscapeActor->InitializeLandscapeLayersWeightmapUsage();

	InLandscapeInfo->LandscapeActor->bIncludeGridSizeInNameForLandscapeActors = true;

	FIntRect Extent;
	InLandscapeInfo->GetLandscapeExtent(Extent.Min.X, Extent.Min.Y, Extent.Max.X, Extent.Max.Y);

	const UWorld* World = InLandscapeInfo->LandscapeActor->GetWorld();
	UActorPartitionSubsystem* ActorPartitionSubsystem = World->GetSubsystem<UActorPartitionSubsystem>();

	TArray<ULandscapeComponent*> LandscapeComponents;
	LandscapeComponents.Reserve(InLandscapeInfo->XYtoComponentMap.Num());
	InLandscapeInfo->ForAllLandscapeComponents([&LandscapeComponents](ULandscapeComponent* LandscapeComponent)
	{
		LandscapeComponents.Add(LandscapeComponent);
	});

	{
		const uint32 ActorPartitionSubsystemGridSize = GridSize > 0 ? GridSize : ALandscapeStreamingProxy::StaticClass()->GetDefaultObject<APartitionActor>()->GetDefaultGridSize(World->PersistentLevel->GetWorld());
		const UActorPartitionSubsystem::FCellCoord MinCellCoords = UActorPartitionSubsystem::FCellCoord::GetCellCoord(Extent.Min, World->PersistentLevel, ActorPartitionSubsystemGridSize);
		const UActorPartitionSubsystem::FCellCoord MaxCellCoords = UActorPartitionSubsystem::FCellCoord::GetCellCoord(Extent.Max, World->PersistentLevel, ActorPartitionSubsystemGridSize);

		// 전체 스텝 수 계산
		const int32 NumX = MaxCellCoords.X - MinCellCoords.X + 1;
		const int32 NumY = MaxCellCoords.Y - MinCellCoords.Y + 1;
		const int32 TotalSteps = NumX * NumY;

		// FScopedSlowTask 생성
		FScopedSlowTask SlowTask(TotalSteps, NSLOCTEXT("ONEBUTTONLEVELGENERATION_API", "Create LandscapeStreamingProxy", "Creating LandscapeStreamingProxies..."));
		SlowTask.MakeDialog(/*bShowCancelButton=*/ false);

		FActorPartitionGridHelper::ForEachIntersectingCell(ALandscapeStreamingProxy::StaticClass(), Extent, World->PersistentLevel, [&SlowTask, TotalSteps, ActorPartitionSubsystem, InLandscapeInfo, InNewGridSizeInComponents, &LandscapeComponents](const UActorPartitionSubsystem::FCellCoord& CellCoord, const FIntRect& CellBounds)
		{
			// // 진행도 1 증가
			SlowTask.EnterProgressFrame(1.0f, FText::Format(
				NSLOCTEXT("Landscape", "ProcessingCell", "Processing Landscape cell {0}/{1}..."),
				FText::AsNumber(SlowTask.CompletedWork),
				FText::AsNumber(TotalSteps)
			));

			TMap<ULandscapeComponent*, UMaterialInterface*> ComponentMaterials;

			TArray<ULandscapeComponent*> ComponentsToMove;
			const int32 MaxComponents = static_cast<int32>(InNewGridSizeInComponents * InNewGridSizeInComponents);
			ComponentsToMove.Reserve(MaxComponents);
			for (int32 i = 0; i < LandscapeComponents.Num();)
			{
				if (ULandscapeComponent* LandscapeComponent = LandscapeComponents[i]; CellBounds.Contains(LandscapeComponent->GetSectionBase()))
				{
					ComponentMaterials.FindOrAdd(LandscapeComponent, LandscapeComponent->GetLandscapeMaterial());
					ComponentsToMove.Add(LandscapeComponent);
					LandscapeComponents.RemoveAtSwap(i);
				}
				else
				{
					i++;
				}
			}

			check(ComponentsToMove.Num() <= MaxComponents);
			if (ComponentsToMove.Num())
			{
				ALandscapeProxy* LandscapeProxy = FindOrAddLandscapeStreamingProxy(ActorPartitionSubsystem, InLandscapeInfo, CellCoord);
				check(LandscapeProxy);
				InLandscapeInfo->MoveComponentsToProxy(ComponentsToMove, LandscapeProxy);

				for (ULandscapeComponent* MovedComponent : ComponentsToMove)
				{
					UMaterialInterface* PreviousLandscapeMaterial = ComponentMaterials.FindChecked(MovedComponent);

					MovedComponent->OverrideMaterial = nullptr;
					if (PreviousLandscapeMaterial != nullptr && PreviousLandscapeMaterial != MovedComponent->GetLandscapeMaterial())
					{
						if(LandscapeProxy->GetLandscapeMaterial() == LandscapeProxy->GetLandscapeActor()->GetLandscapeMaterial())
						{
							LandscapeProxy->LandscapeMaterial = PreviousLandscapeMaterial;
						}
						else
						{
							MovedComponent->OverrideMaterial = PreviousLandscapeMaterial;
						}
					}
				}
			}

			return true;
		}, GridSize);
	}

	if (InLandscapeInfo->CanHaveLayersContent())
	{
		InLandscapeInfo->ForceLayersFullUpdate();
	}

	return true;
}

void OCGLandscapeUtil::AddLandscapeComponent(ULandscapeInfo* InLandscapeInfo, ULandscapeSubsystem* InLandscapeSubsystem,
                                             const TArray<FIntPoint>& InComponentCoordinates, TArray<ALandscapeProxy*>& OutCreatedStreamingProxies)
{
	TArray<ULandscapeComponent*> NewComponents;
	InLandscapeInfo->Modify();

	for (const FIntPoint& ComponentCoordinate : InComponentCoordinates)
	{
		ULandscapeComponent* LandscapeComponent = InLandscapeInfo->XYtoComponentMap.FindRef(ComponentCoordinate);
		if (LandscapeComponent)
		{
			continue;
		}

		// Add New component...
		FIntPoint ComponentBase = ComponentCoordinate * InLandscapeInfo->ComponentSizeQuads;

		ALandscapeProxy* LandscapeProxy = InLandscapeSubsystem->FindOrAddLandscapeProxy(InLandscapeInfo, ComponentBase);
		if (!LandscapeProxy)
		{
			continue;
		}

		OutCreatedStreamingProxies.Add(LandscapeProxy);

		LandscapeComponent = NewObject<ULandscapeComponent>(LandscapeProxy, NAME_None, RF_Transactional);
		NewComponents.Add(LandscapeComponent);
		LandscapeComponent->Init(
			ComponentBase.X, ComponentBase.Y,
			LandscapeProxy->ComponentSizeQuads,
			LandscapeProxy->NumSubsections,
			LandscapeProxy->SubsectionSizeQuads
		);

		TArray<FColor> HeightData;
		const int32 ComponentVerts = (LandscapeComponent->SubsectionSizeQuads + 1) * LandscapeComponent->NumSubsections;
		const FColor PackedMidpoint = LandscapeDataAccess::PackHeight(LandscapeDataAccess::GetTexHeight(0.0f));
		HeightData.Init(PackedMidpoint, FMath::Square(ComponentVerts));

		LandscapeComponent->InitHeightmapData(HeightData, true);
		LandscapeComponent->UpdateMaterialInstances();

		InLandscapeInfo->XYtoComponentMap.Add(ComponentCoordinate, LandscapeComponent);
		InLandscapeInfo->XYtoAddCollisionMap.Remove(ComponentCoordinate);
	}

	for (int32 Idx = 0; Idx < NewComponents.Num(); Idx++)
	{
		NewComponents[Idx]->RegisterComponent();
	}

	ALandscape* Landscape = InLandscapeInfo->LandscapeActor.Get();

	bool bHasLandscapeLayersContent = Landscape && Landscape->HasLayersContent();

	for (ULandscapeComponent* NewComponent : NewComponents)
	{
		if (bHasLandscapeLayersContent)
		{
			TArray<ULandscapeComponent*> ComponentsUsingHeightmap;
			ComponentsUsingHeightmap.Add(NewComponent);

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 5
			for (const ULandscapeEditLayerBase* EditLayer : Landscape->GetEditLayersConst())
			{
				TMap<UTexture2D*, UTexture2D*> CreatedHeightmapTextures;
				NewComponent->AddDefaultLayerData(EditLayer->GetGuid(), ComponentsUsingHeightmap, CreatedHeightmapTextures);
			}
#else
			for (const FLandscapeLayer& EditLayer : Landscape->GetLayers())
			{
				TMap<UTexture2D*, UTexture2D*> CreatedHeightmapTextures;
				NewComponent->AddDefaultLayerData(EditLayer.Guid, ComponentsUsingHeightmap, CreatedHeightmapTextures);
			}
#endif
		}

		// Update Collision
		NewComponent->UpdateCachedBounds();
		NewComponent->UpdateBounds();
		NewComponent->MarkRenderStateDirty();

		if (!bHasLandscapeLayersContent)
		{
			if (ULandscapeHeightfieldCollisionComponent* CollisionComp = NewComponent->GetCollisionComponent())
			{
				CollisionComp->MarkRenderStateDirty();
				CollisionComp->RecreateCollision();
			}
		}
	}

	if (Landscape)
	{
		GEngine->BroadcastOnActorMoved(Landscape);
	}
}

ALocationVolume* OCGLandscapeUtil::CreateLandscapeRegionVolume(UWorld* InWorld, ALandscapeProxy* InParentLandscapeActor,
	const FIntPoint& InRegionCoordinate, const double InRegionSize)
{
#if WITH_EDITOR
	constexpr double Shrink = 0.95;

	const FVector ParentLocation = InParentLandscapeActor->GetActorLocation();
	const FVector Location = ParentLocation + FVector(InRegionCoordinate.X * InRegionSize, InRegionCoordinate.Y * InRegionSize, 0.0) + FVector(InRegionSize / 2.0, InRegionSize / 2.0, 0.0);
	FRotator Rotation;
	FActorSpawnParameters SpawnParameters;

	const FString Label = FString::Printf(TEXT("LandscapeRegion_%i_%i"), InRegionCoordinate.X, InRegionCoordinate.Y);
	SpawnParameters.Name = FName(*Label);
	SpawnParameters.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;

	ALocationVolume* LocationVolume = InWorld->SpawnActor<ALocationVolume>(Location, Rotation, SpawnParameters);
	if (LocationVolume == nullptr)
	{
		return nullptr;
	}
	LocationVolume->SetActorLabel(Label);

	LocationVolume->AttachToActor(InParentLandscapeActor, FAttachmentTransformRules::KeepWorldTransform);
	const FVector Scale{ InRegionSize * Shrink,  InRegionSize * Shrink, InRegionSize * 0.5f };
	LocationVolume->SetActorScale3D(Scale);

	UCubeBuilder* Builder = NewObject<UCubeBuilder>();
	Builder->X = 1.0f;
	Builder->Y = 1.0f;
	Builder->Z = 1.0f;
	UActorFactory::CreateBrushForVolumeActor(LocationVolume, Builder);

	LocationVolume->Tags.Add(FName("LandscapeRegion"));
	return LocationVolume;
#endif
}

ULandscapeLayerInfoObject* OCGLandscapeUtil::CreateLayerInfo(ALandscape* InLandscape, const FString& InPackagePath,
                                                             const FString& InAssetName, const ULandscapeLayerInfoObject* InTemplate)
{
#if WITH_EDITOR
	if (!InLandscape)
	{
		UE_LOG(LogOCGModule, Error, TEXT("TargetLandscape is null."));
		return nullptr;
	}

	// 위에서 정의한 함수를 사용하여 지정된 경로에 LayerInfo 애셋을 생성합니다.
	ULandscapeLayerInfoObject* LayerInfo = CreateLayerInfo(InPackagePath, InAssetName, InTemplate);

	if (LayerInfo)
	{
		if (ULandscapeInfo* LandscapeInfo = InLandscape->GetLandscapeInfo())
		{
			// 랜드스케이프 정보에 새로 생성된 LayerInfo를 추가하거나 업데이트합니다.
			// 이 과정은 랜드스케이프가 해당 레이어를 인식하게 만듭니다.
			const int32 Index = LandscapeInfo->GetLayerInfoIndex(LayerInfo->LayerName, InLandscape);
			if (Index == INDEX_NONE)
			{
				LandscapeInfo->Layers.Add(FLandscapeInfoLayerSettings(LayerInfo, InLandscape));
			}
			else
			{
				LandscapeInfo->Layers[Index].LayerInfoObj = LayerInfo;
			}
		}
	}

	return LayerInfo;
#endif
}

void OCGLandscapeUtil::ForEachComponentByRegion(int32 RegionSize, const TArray<FIntPoint>& ComponentCoordinates,
	const TFunctionRef<bool(const FIntPoint&, const TArray<FIntPoint>&)>& RegionFn)
{
	if (RegionSize <= 0)
	{
		return;
	}

	TMap<FIntPoint, TArray< FIntPoint>> Regions;
	for (FIntPoint ComponentCoordinate : ComponentCoordinates)
	{
		FIntPoint RegionCoordinate;
		RegionCoordinate.X = (ComponentCoordinate.X) / RegionSize;
		RegionCoordinate.Y = (ComponentCoordinate.Y) / RegionSize;

		if (TArray<FIntPoint>* ComponentIndices = Regions.Find(RegionCoordinate))
		{
			ComponentIndices->Add(ComponentCoordinate);
		}
		else
		{
			Regions.Add(RegionCoordinate, TArray<FIntPoint> {ComponentCoordinate});
		}
	}

	auto Sorter = [](const FIntPoint& A, const FIntPoint& B)
	{
		if (A.Y == B.Y)
			return A.X < B.X;

		return A.Y < B.Y;
	};

	Regions.KeySort(Sorter);

	for (const TPair< FIntPoint, TArray< FIntPoint>>& RegionIt : Regions)
	{
		if (!RegionFn(RegionIt.Key, RegionIt.Value))
		{
			break;
		}
	}
}

void OCGLandscapeUtil::ForEachRegion_LoadProcessUnload(ULandscapeInfo* InLandscapeInfo, const FIntRect& InDomain,
                                                       const UWorld* InWorld, const TFunctionRef<bool(const FBox&, const TArray<ALandscapeProxy*>)>& InRegionFn)
{
#if WITH_EDITOR
	TArray<AActor*> Children;
	InLandscapeInfo->LandscapeActor->GetAttachedActors(Children);
	TArray<ALocationVolume*> LandscapeRegions;

	for (AActor* Child : Children)
	{
		if (Child->IsA<ALocationVolume>())
		{
			LandscapeRegions.Add(Cast<ALocationVolume>(Child));
		}
	}

	for (ALocationVolume* Region : LandscapeRegions)
	{
		Region->Load();

		FBox RegionBounds = Region->GetComponentsBoundingBox();

		TArray<AActor*> AllActors = InWorld->GetLevel(0)->Actors;
		TArray<ALandscapeProxy*> LandscapeProxies;
		for (AActor* Actor : AllActors)
		{
			if (ALandscapeStreamingProxy* Proxy = Cast<ALandscapeStreamingProxy>(Actor))
			{
				LandscapeProxies.Add(Proxy);
			}
		}

		// Save the actor
		const bool bShouldExit = !InRegionFn(RegionBounds, LandscapeProxies);

		InLandscapeInfo->ForceLayersFullUpdate();

		OCGLandscapeUtil::SaveLandscapeProxies(InWorld, MakeArrayView(LandscapeProxies));

		Region->Unload();

		if (bShouldExit)
		{
			break;
		}
	}
#endif
}

void OCGLandscapeUtil::SaveLandscapeProxies(const UWorld* World, const TArrayView<ALandscapeProxy*> Proxies)
{
#if WITH_EDITOR
	TRACE_CPUPROFILER_EVENT_SCOPE(SaveCreatedActors);
	UWorldPartition::FDisableNonDirtyActorTrackingScope Scope(World->GetWorldPartition(), true);
	SaveObjects(Proxies);
#endif
}

TMap<FGuid, TArray<FLandscapeImportLayerInfo>> OCGLandscapeUtil::PrepareLandscapeLayerData(
	ALandscape* InTargetLandscape, AOCGLevelGenerator* InLevelGenerator, const UMapPreset* InMapPreset)
{
	#if WITH_EDITOR
    TMap<FGuid, TArray<FLandscapeImportLayerInfo>> MaterialLayerDataPerLayer;
    if (!InTargetLandscape || !InLevelGenerator || !InMapPreset)
    {
        return MaterialLayerDataPerLayer; // 유효하지 않은 입력이면 빈 맵 반환
    }

    ULandscapeInfo* LandscapeInfo = InTargetLandscape->GetLandscapeInfo();

    TArray<FLandscapeImportLayerInfo> ImportLayerDataPerLayer;
    const ULandscapeSettings* Settings = GetDefault<ULandscapeSettings>();
    ULandscapeLayerInfoObject* DefaultLayerInfo = Settings->GetDefaultLayerInfoObject().LoadSynchronous();

    // 1. 웨이트맵 데이터와 머티리얼에서 레이어 이름 가져오기
    TMap<FName, TArray<uint8>> WeightLayers = InLevelGenerator->GetWeightLayers();
    TArray<FName> LayerNames;
    if (InMapPreset->LandscapeMaterial && InMapPreset->LandscapeMaterial->Parent)
    {
        LayerNames = OCGMaterialEditTool::ExtractLandscapeLayerName(Cast<UMaterial>(InMapPreset->LandscapeMaterial->Parent));
    }

    // 2. 각 레이어를 순회하며 LayerInfoObject를 찾거나 생성
    for (int32 Index = 0; Index < WeightLayers.Num(); ++Index)
    {
        FLandscapeImportLayerInfo LayerInfo;

        // 임시 이름 생성 (머티리얼에서 이름을 못 찾을 경우 대비)
        FString TempLayerNameStr = FString::Printf(TEXT("Layer%d"), Index);
        FName TempLayerName(TempLayerNameStr);

        LayerInfo.LayerData = WeightLayers.FindChecked(TempLayerName);

        if (LayerNames.IsValidIndex(Index))
        {
            LayerInfo.LayerName = LayerNames[Index];
        }
        else
        {
            LayerInfo.LayerName = TempLayerName;
            UE_LOG(LogOCGModule, Warning, TEXT("Layer %d not found in Material Names, using default name: %s"), Index, *TempLayerName.ToString());
        }

        ULandscapeLayerInfoObject* LayerInfoObject = nullptr;
        if (LandscapeInfo)
        {
            LayerInfoObject = LandscapeInfo->GetLayerInfoByName(LayerInfo.LayerName);
        }

        if (LayerInfoObject == nullptr)
        {
            UE_LOG(LogOCGModule, Log, TEXT("LayerInfo for '%s' not found. Creating a new one."), *LayerInfo.LayerName.ToString());

            // LayerInfoSavePath는 멤버 변수로 가정합니다. 필요시 파라미터로 전달할 수 있습니다.
            LayerInfoObject = OCGLandscapeUtil::CreateLayerInfo(InTargetLandscape, OCGLandscapeUtil::LayerInfoSavePath, LayerInfo.LayerName.ToString(), DefaultLayerInfo);
        }
        else
        {
            UE_LOG(LogOCGModule, Log, TEXT("Found and reused existing LayerInfo for '%s'."), *LayerInfo.LayerName.ToString());
        }
        
        if(LayerInfoObject)
        {
            LayerInfo.LayerInfo = LayerInfoObject;
            ImportLayerDataPerLayer.Add(LayerInfo);
        }
        else
        {
            UE_LOG(LogOCGModule, Error, TEXT("Failed to find or create LayerInfo for '%s'."), *LayerInfo.LayerName.ToString());
        }
    }

    // 3. 최종 데이터 구조에 추가하여 반환
    FGuid LayerGuid = FGuid(); // 기본 레이어 세트의 GUID
    MaterialLayerDataPerLayer.Add(LayerGuid, MoveTemp(ImportLayerDataPerLayer));
    
    return MaterialLayerDataPerLayer;
#endif
}

ALandscapeProxy* OCGLandscapeUtil::FindOrAddLandscapeStreamingProxy(UActorPartitionSubsystem* InActorPartitionSubsystem,
                                                                    const ULandscapeInfo* InLandscapeInfo, const UActorPartitionSubsystem::FCellCoord& InCellCoord)
{
	ALandscape* Landscape = InLandscapeInfo->LandscapeActor.Get();
	check(Landscape);

	auto LandscapeProxyCreated = [InCellCoord, Landscape](APartitionActor* PartitionActor)
	{
		const FIntPoint CellLocation(static_cast<int32>(InCellCoord.X) * Landscape->GetGridSize(), static_cast<int32>(InCellCoord.Y) * Landscape->GetGridSize());

		ALandscapeProxy* LandscapeProxy = CastChecked<ALandscapeProxy>(PartitionActor);
		
		LandscapeProxy->SynchronizeSharedProperties(Landscape);
		const FVector ProxyLocation = Landscape->GetActorLocation() + FVector(CellLocation.X * Landscape->GetActorRelativeScale3D().X, CellLocation.Y * Landscape->GetActorRelativeScale3D().Y, 0.0f);

		LandscapeProxy->CreateLandscapeInfo();
		LandscapeProxy->SetActorLocationAndRotation(ProxyLocation, Landscape->GetActorRotation());
		LandscapeProxy->LandscapeSectionOffset = FIntPoint(CellLocation.X, CellLocation.Y);
		LandscapeProxy->SetIsSpatiallyLoaded(LandscapeProxy->GetLandscapeInfo()->AreNewLandscapeActorsSpatiallyLoaded());
	};

	constexpr bool bCreate = true;
	constexpr bool bBoundsSearch = false;

	ALandscapeProxy* LandscapeProxy = Cast<ALandscapeProxy>(InActorPartitionSubsystem->GetActor(ALandscapeStreamingProxy::StaticClass(), InCellCoord, bCreate, InLandscapeInfo->LandscapeGuid, Landscape->GetGridSize(), bBoundsSearch, LandscapeProxyCreated));
	check(!LandscapeProxy || LandscapeProxy->GetGridSize() == Landscape->GetGridSize());
	return LandscapeProxy;
}

ULandscapeLayerInfoObject* OCGLandscapeUtil::CreateLayerInfo(const FString& InPackagePath, const FString& InAssetName,
                                                             const ULandscapeLayerInfoObject* InTemplate)
{
	#if WITH_EDITOR
    // 1. 경로 유효성 검사
    if (!InPackagePath.StartsWith(TEXT("/Game/")))
    {
        UE_LOG(LogOCGModule, Error, TEXT("PackagePath must start with /Game/. Path was: %s"), *InPackagePath);
        return nullptr;
    }
    
    // 2. 가장 먼저, 입력된 애셋 이름을 안전한 이름으로 변환합니다.
    // 이 정리된 이름이 앞으로 모든 작업의 기준이 됩니다.
    const FString SanitizedAssetName = ObjectTools::SanitizeObjectName(InAssetName);
    if (SanitizedAssetName.IsEmpty())
    {
        UE_LOG(LogOCGModule, Error, TEXT("Asset name '%s' became empty after sanitization."), *InAssetName);
        return nullptr;
    }

    // 3. 안전한 이름을 사용하여 전체 패키지 경로와 오브젝트 경로를 구성합니다.
    const FString FullPackageName = FPaths::Combine(InPackagePath, SanitizedAssetName);
    const FString ObjectPathToLoad = FullPackageName + TEXT(".") + SanitizedAssetName;

    // 4. 안전한 경로를 사용하여 애셋이 이미 존재하는지 로드를 시도합니다.
    ULandscapeLayerInfoObject* FoundLayerInfo = LoadObject<ULandscapeLayerInfoObject>(nullptr, *ObjectPathToLoad);
    
    // 5. 애셋을 찾았다면, 즉시 반환합니다.
    if (FoundLayerInfo)
    {
        UE_LOG(LogOCGModule, Log, TEXT("Found and reused existing LayerInfo: %s"), *ObjectPathToLoad);
        return FoundLayerInfo;
    }

    // 애셋을 찾지 못했으므로, 새로 생성하기 전에 디렉토리 존재를 확인하고 필요시 생성합니다.
    // InPackagePath는 "/Game/MyFolder/MySubFolder" 형태일 것으로 예상됩니다.
    if (!FOCGFileUtils::EnsureContentDirectoryExists(FullPackageName))
    {
        UE_LOG(LogOCGModule, Error, TEXT("Failed to ensure directory exists for package path: %s"), *InPackagePath);
        return nullptr;
    }

    // 6. 애셋을 찾지 못했으므로, 새로 생성하는 로직을 실행합니다.
    UE_LOG(LogOCGModule, Log, TEXT("LayerInfo not found at '%s'. Creating a new one."), *ObjectPathToLoad);

    const FName AssetFName(*SanitizedAssetName);

    UPackage* Package = CreatePackage(*FullPackageName);
    if (!Package)
    {
        UE_LOG(LogOCGModule, Error, TEXT("Failed to create package: %s"), *FullPackageName);
        return nullptr;
    }
    
    ULandscapeLayerInfoObject* NewLayerInfo;
    if (InTemplate)
    {
        NewLayerInfo = DuplicateObject<ULandscapeLayerInfoObject>(InTemplate, Package, AssetFName);
    }
    else
    {
        NewLayerInfo = NewObject<ULandscapeLayerInfoObject>(Package, AssetFName, RF_Public | RF_Standalone | RF_Transactional);
    }

    if (!NewLayerInfo)
    {
        UE_LOG(LogOCGModule, Error, TEXT("Failed to create ULandscapeLayerInfoObject in package: %s"), *FullPackageName);
        return nullptr;
    }
    
    // LayerName 프로퍼티에도 안전한 이름을 할당합니다.
    NewLayerInfo->LayerName = AssetFName;
    
    FAssetRegistryModule::AssetCreated(NewLayerInfo);
    NewLayerInfo->LayerUsageDebugColor = NewLayerInfo->GenerateLayerUsageDebugColor();
    (void)NewLayerInfo->MarkPackageDirty();

    return NewLayerInfo;
#endif
}

FGuid OCGLandscapeUtil::GetLandscapeLayerGuid(const ALandscape* Landscape, FName LayerName)
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 5
	if (const ULandscapeEditLayerBase* BaseLayer = Landscape->GetEditLayerConst(LayerName))
	{
		return BaseLayer->GetGuid();
	}
#else
	if (const FLandscapeLayer* BaseLayer = Landscape->GetLayerConst(LayerName))
	{
		return BaseLayer->Guid;
	}
#endif
	return {};
}

void OCGLandscapeUtil::RegenerateRiver(UWorld* World, AOCGLevelGenerator* LevelGenerator)
{
#if WITH_EDITOR
	if (World && LevelGenerator)
		LevelGenerator->GetRiverGenerateComponent()->GenerateRiver(World, LevelGenerator->GetLandscape(), false);
#endif
}

void OCGLandscapeUtil::ForceGeneratePCG(UWorld* World)
{
	const TArray<AOCGLandscapeVolume*> Actors =
				FOCGUtils::GetAllActorsOfClass<AOCGLandscapeVolume>(World);

	for (const AOCGLandscapeVolume* VolumeActor : Actors)
	{
		VolumeActor->GetPCGComponent()->Generate(true);
	}
}
