// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "FileHelpers.h"
#include "Landscape.h"
#include "ActorPartition/ActorPartitionSubsystem.h"

struct FLandscapeSetting;
class UMapPreset;
class AOCGLevelGenerator;
class ALocationVolume;
class ULandscapeInfo;
class ULandscapeSubsystem;
class ALandscapeProxy;
class ULandscapeLayerInfoObject;
struct FLandscapeImportLayerInfo;
class ALandscape;
/**
 * 
 */
class ONEBUTTONLEVELGENERATION_API OCGLandscapeUtil
{
public:
	OCGLandscapeUtil();
	~OCGLandscapeUtil();

	static void ExtractHeightMap(ALandscape* InLandscape, const FGuid InGuid, int32& OutWidth, int32& OutHeight, TArray<uint16>& OutHeightMap);
	
	static void AddWeightMap(ALandscape* InLandscape, int32 InTargetLayerIndex, const TArray<uint8>& InWeightMap);

	static void AddWeightMap(ALandscape* InLandscape, ULandscapeLayerInfoObject* InLayerInfo, const TArray<uint8>& InWeightMap);

	static void ApplyWeightMap(ALandscape* InLandscape, int32 InTargetLayerIndex, const TArray<uint8>& InWeightMap);
	
	static void ApplyWeightMap(ALandscape* InLandscape, ULandscapeLayerInfoObject* InLayerInfo, const TArray<uint8>& InWeightMap);

	static void ApplyMaskedWeightMap(ALandscape* InLandscape, ULandscapeLayerInfoObject* InLayerInfo, const TArray<uint8>& OriginWeightMap, const TArray<uint8>& InMaskedWeightMap);
	
	static void GetWeightMap(ALandscape* InLandscape, int32 InTargetLayerIndex, TArray<uint8>& OutOriginWeightMap);

	static void GetWeightMap(ALandscape* InLandscape, ULandscapeLayerInfoObject* InLayerInfo, TArray<uint8>& OutOriginWeightMap);
	
	static void GetMaskedWeightMap(ALandscape* InLandscape, int32 InTargetLayerIndex, const TArray<uint8>& Mask, TArray<uint8>& OutWeightMap);

	static void GetMaskedWeightMap(ALandscape* InLandscape, ULandscapeLayerInfoObject* InLayerInfo, const TArray<uint8>& Mask, TArray<uint8>& OutWeightMap);
	
	static void CleanUpWeightMap(ALandscape* InLandscape);

	static void MakeWeightMapFromHeightDiff(const TArray<uint16>& HeightDiff, TArray<uint8>& OutWeight, uint16 MinDiffThreshold = 0);

	static void BlurWeightMap(const TArray<uint8>& InWeight, TArray<uint8>& OutWeight, int32 Width, int32 Height);

	static void ClearTargetLayers(const ALandscape* InLandscape);

	static void UpdateTargetLayers(ALandscape* InLandscape, const TMap<FGuid, TArray<FLandscapeImportLayerInfo>>& MaterialLayerDataPerLayers);

	static void AddTargetLayers(ALandscape* InLandscape, const TMap<FGuid, TArray<FLandscapeImportLayerInfo>>& MaterialLayerDataPerLayers);

	static void ManageLandscapeRegions(UWorld* World, const ALandscape* Landscape, UMapPreset* InMapPreset, const FLandscapeSetting& InLandscapeSetting);

	static void ImportMapDatas(UWorld* World, ALandscape* InLandscape, TArray<uint16> ImportHeightMap, TArray<FLandscapeImportLayerInfo> ImportLayers);

	static TMap<FGuid, TArray<FLandscapeImportLayerInfo>> PrepareLandscapeLayerData(ALandscape* InTargetLandscape, AOCGLevelGenerator* InLevelGenerator, const UMapPreset* InMapPreset);

	static void RegenerateRiver(UWorld* World, AOCGLevelGenerator* LevelGenerator);

	static void ForceGeneratePCG(UWorld* World);
	
	static FGuid GetLandscapeLayerGuid(const ALandscape* Landscape, FName LayerName);
private:
	static FString LayerInfoSavePath;

private:
	static bool ChangeGridSize(const UWorld* InWorld, ULandscapeInfo* InLandscapeInfo, uint32 InNewGridSizeInComponents);

	static void AddLandscapeComponent(ULandscapeInfo* InLandscapeInfo, ULandscapeSubsystem* InLandscapeSubsystem, const TArray<FIntPoint>& InComponentCoordinates, TArray<ALandscapeProxy*>& OutCreatedStreamingProxies);

	static ALocationVolume* CreateLandscapeRegionVolume(UWorld* InWorld, ALandscapeProxy* InParentLandscapeActor, const FIntPoint& InRegionCoordinate, double InRegionSize);

	static ULandscapeLayerInfoObject* CreateLayerInfo(ALandscape* InLandscape, const FString& InPackagePath, const FString& InAssetName, const ULandscapeLayerInfoObject* InTemplate = nullptr);

	static void ForEachComponentByRegion(int32 RegionSize, const TArray<FIntPoint>& ComponentCoordinates, const TFunctionRef<bool(const FIntPoint&, const TArray<FIntPoint>&)>& RegionFn);

	static void ForEachRegion_LoadProcessUnload(ULandscapeInfo* InLandscapeInfo, const FIntRect& InDomain, const UWorld* InWorld, const TFunctionRef<bool(const FBox&, const TArray<ALandscapeProxy*>)>& InRegionFn);

	static void SaveLandscapeProxies(const UWorld* World, TArrayView<ALandscapeProxy*> Proxies);

	template<typename T>
	static void SaveObjects(TArrayView<T*> InObjects)
	{
		TArray<UPackage*> Packages;
		Algo::Transform(InObjects, Packages, [](const UObject* InObject) { return InObject->GetPackage(); });
		UEditorLoadingAndSavingUtils::SavePackages(Packages, /* bOnlyDirty = */ false);
	}

	static ALandscapeProxy* FindOrAddLandscapeStreamingProxy(UActorPartitionSubsystem* InActorPartitionSubsystem, const ULandscapeInfo* InLandscapeInfo, const UActorPartitionSubsystem::FCellCoord& InCellCoord);

	static ULandscapeLayerInfoObject* CreateLayerInfo(const FString& InPackagePath, const FString& InAssetName, const ULandscapeLayerInfoObject* InTemplate = nullptr);

};
