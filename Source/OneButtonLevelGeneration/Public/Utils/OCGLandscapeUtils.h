// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "FileHelpers.h"
#include "Landscape.h"
#include "ActorPartition/ActorPartitionSubsystem.h"

struct FLandscapeSetting;
class UMapPreset;
class ALocationVolume;
class ULandscapeInfo;
class ULandscapeSubsystem;
class ALandscapeProxy;
class ULandscapeLayerInfoObject;
struct FLandscapeImportLayerInfo;
class ALandscape;

/**
 * 랜드스케이프(Landscape) 조작 유틸리티 모음
 * 하이트맵/웨이트맵 입출력, 타깃 레이어 관리, World Partition 리전 처리,
 * LayerInfo 에셋 생성, PCG 생성 등 에디터 전용 랜드스케이프 작업을 담당합니다.
 */
struct ONEBUTTONLEVELGENERATION_API FOCGLandscapeUtils
{
	FOCGLandscapeUtils() = delete;

	/** 랜드스케이프의 특정 편집 레이어(InGuid)에서 하이트맵(uint16)과 해상도를 추출합니다. */
	static void ExtractHeightMap(ALandscape* InLandscape, const FGuid InGuid, int32& OutWidth, int32& OutHeight, TArray<uint16>& OutHeightMap);

	/** 지정한 타깃 레이어 인덱스에 웨이트맵을 누적 추가합니다. */
	static void AddWeightMap(ALandscape* InLandscape, int32 InTargetLayerIndex, const TArray<uint8>& InWeightMap);

	/** 지정한 LayerInfo 객체에 해당하는 레이어에 웨이트맵을 누적 추가합니다. */
	static void AddWeightMap(ALandscape* InLandscape, ULandscapeLayerInfoObject* InLayerInfo, const TArray<uint8>& InWeightMap);

	/** 지정한 타깃 레이어 인덱스의 웨이트맵을 주어진 값으로 덮어씁니다. */
	static void ApplyWeightMap(ALandscape* InLandscape, int32 InTargetLayerIndex, const TArray<uint8>& InWeightMap);

	/** 지정한 LayerInfo 객체에 해당하는 레이어의 웨이트맵을 주어진 값으로 덮어씁니다. */
	static void ApplyWeightMap(ALandscape* InLandscape, ULandscapeLayerInfoObject* InLayerInfo, const TArray<uint8>& InWeightMap);

	/** InMaskedWeightMap이 0이 아닌 픽셀은 그 값으로 덮어쓰고, 0인 픽셀은 OriginWeightMap 값을 유지하여 합성한 결과를 적용합니다. */
	static void ApplyMaskedWeightMap(ALandscape* InLandscape, ULandscapeLayerInfoObject* InLayerInfo, const TArray<uint8>& OriginWeightMap, const TArray<uint8>& InMaskedWeightMap);

	/** 지정한 타깃 레이어 인덱스의 현재 웨이트맵을 읽어옵니다. */
	static void GetWeightMap(ALandscape* InLandscape, int32 InTargetLayerIndex, TArray<uint8>& OutOriginWeightMap);

	/** 지정한 LayerInfo 객체에 해당하는 레이어의 현재 웨이트맵을 읽어옵니다. */
	static void GetWeightMap(ALandscape* InLandscape, ULandscapeLayerInfoObject* InLayerInfo, TArray<uint8>& OutOriginWeightMap);

	/** 지정한 타깃 레이어 인덱스의 웨이트맵을 읽어 Mask로 마스킹한 결과를 반환합니다. */
	static void GetMaskedWeightMap(ALandscape* InLandscape, int32 InTargetLayerIndex, const TArray<uint8>& Mask, TArray<uint8>& OutWeightMap);

	/** 지정한 LayerInfo의 웨이트맵을 읽어 Mask로 마스킹한 결과를 반환합니다. */
	static void GetMaskedWeightMap(ALandscape* InLandscape, ULandscapeLayerInfoObject* InLayerInfo, const TArray<uint8>& Mask, TArray<uint8>& OutWeightMap);

	/** 인덱스 1번 이상의 모든 레이어 웨이트맵을 읽어 그대로 다시 적용(재커밋)해 정규화합니다. (값을 0으로 초기화하지 않음) */
	static void CleanUpWeightMap(ALandscape* InLandscape);

	/** 높이 차이(HeightDiff)가 MinDiffThreshold를 초과하는 픽셀만 255, 나머지는 0인 이진(binary) 웨이트맵을 생성합니다. */
	static void MakeWeightMapFromHeightDiff(const TArray<uint16>& HeightDiff, TArray<uint8>& OutWeight, uint16 MinDiffThreshold = 0);

	/** 웨이트맵에 블러를 적용해 경계를 부드럽게 만듭니다. */
	static void BlurWeightMap(const TArray<uint8>& InWeight, TArray<uint8>& OutWeight, int32 Width, int32 Height);

	/** 랜드스케이프의 모든 타깃 레이어를 제거합니다. */
	static void ClearTargetLayers(const ALandscape* InLandscape);

	/** 레이어 정보 맵을 갱신한 뒤, 임포트 데이터의 각 레이어를 타깃 레이어로 추가 또는 갱신합니다(LayerInfo가 없으면 생성). 추가로 PCG 소거용 'ErasePCG_Layer'를 등록합니다. */
	static void UpdateTargetLayers(ALandscape* InLandscape, const TMap<FGuid, TArray<FLandscapeImportLayerInfo>>& MaterialLayerDataPerLayers);

	/** 임포트 데이터의 각 레이어를 타깃 레이어로 추가합니다(LayerInfo가 없으면 생성). 추가로 PCG 소거용 'ErasePCG_Layer'를 등록합니다. */
	static void AddTargetLayers(ALandscape* InLandscape, const TMap<FGuid, TArray<FLandscapeImportLayerInfo>>& MaterialLayerDataPerLayers);

	/** World Partition 기반 랜드스케이프의 리전(스트리밍 프록시/리전 볼륨)을 생성·관리합니다. */
	static void ManageLandscapeRegions(UWorld* World, const ALandscape* Landscape, UMapPreset* InMapPreset, const FLandscapeSetting& InLandscapeSetting);

	/** 하이트맵과 레이어 데이터를 랜드스케이프로 임포트합니다. */
	static void ImportMapDatas(UWorld* World, ALandscape* InLandscape, TArray<uint16> ImportHeightMap, TArray<FLandscapeImportLayerInfo> ImportLayers);

	/** 웨이트 레이어 맵을 임포트용 레이어 데이터(LayerInfo + 웨이트)로 변환합니다. 필요 시 LayerInfo 에셋을 생성합니다. */
	static TMap<FGuid, TArray<FLandscapeImportLayerInfo>> PrepareLandscapeLayerData(ALandscape* InTargetLandscape, const TMap<FName, TArray<uint8>>& InWeightLayers, const UMapPreset* InMapPreset);

	/** 월드의 PCG 컴포넌트 생성을 강제로 실행합니다. */
	static void ForceGeneratePCG(UWorld* World);

	/** 레이어 이름으로 랜드스케이프의 편집 레이어(Edit Layer) Guid를 조회합니다. */
	static FGuid GetLandscapeLayerGuid(const ALandscape* Landscape, FName LayerName);

private:
	/**
	 * LayerInfo 에셋 저장 경로.
	 * @note Project Settings > One-Click Level Generation에서 지정합니다.
	 */
	static FString GetLayerInfoSavePath();

	/** 랜드스케이프 그리드(컴포넌트 단위) 크기를 변경합니다. */
	static bool ChangeGridSize(const UWorld* InWorld, ULandscapeInfo* InLandscapeInfo, uint32 InNewGridSizeInComponents);

	/** 지정한 컴포넌트 좌표들에 랜드스케이프 컴포넌트(스트리밍 프록시)를 추가합니다. */
	static void AddLandscapeComponent(ULandscapeInfo* InLandscapeInfo, ULandscapeSubsystem* InLandscapeSubsystem, const TArray<FIntPoint>& InComponentCoordinates, TArray<ALandscapeProxy*>& OutCreatedStreamingProxies);

	/** 리전 경계를 나타내는 ALocationVolume을 생성합니다. */
	static ALocationVolume* CreateLandscapeRegionVolume(UWorld* InWorld, ALandscapeProxy* InParentLandscapeActor, const FIntPoint& InRegionCoordinate, double InRegionSize);

	/**
	 * LayerInfo 에셋을 생성하고 해당 랜드스케이프에 등록까지 수행하는 오버로드.
	 * 내부적으로 경로 전용 오버로드 CreateLayerInfo(InPackagePath, ...)에 위임합니다.
	 */
	static ULandscapeLayerInfoObject* CreateLayerInfo(ALandscape* InLandscape, const FString& InPackagePath, const FString& InAssetName, const ULandscapeLayerInfoObject* InTemplate = nullptr);

	/** 리전 크기 단위로 컴포넌트 좌표를 묶어 RegionFn을 호출합니다. */
	static void ForEachComponentByRegion(int32 RegionSize, const TArray<FIntPoint>& ComponentCoordinates, const TFunctionRef<bool(const FIntPoint&, const TArray<FIntPoint>&)>& RegionFn);

	/** 도메인 내 각 리전을 로드 → 처리(InRegionFn) → 언로드하는 순회를 수행합니다. */
	static void ForEachRegion_LoadProcessUnload(ULandscapeInfo* InLandscapeInfo, const FIntRect& InDomain, const UWorld* InWorld, const TFunctionRef<bool(const FBox&, const TArray<ALandscapeProxy*>)>& InRegionFn);

	/** 주어진 랜드스케이프 프록시들이 속한 패키지를 저장합니다. */
	static void SaveLandscapeProxies(const UWorld* World, TArrayView<ALandscapeProxy*> Proxies);

	/** 주어진 오브젝트들이 속한 패키지를 일괄 저장하는 템플릿 헬퍼. */
	template<typename T>
	static void SaveObjects(TArrayView<T*> InObjects)
	{
		TArray<UPackage*> Packages;
		Algo::Transform(InObjects, Packages, [](const UObject* InObject) { return InObject->GetPackage(); });
		UEditorLoadingAndSavingUtils::SavePackages(Packages, /* bOnlyDirty = */ false);
	}

	/** 셀 좌표에 해당하는 랜드스케이프 스트리밍 프록시를 찾거나 새로 추가합니다. */
	static ALandscapeProxy* FindOrAddLandscapeStreamingProxy(UActorPartitionSubsystem* InActorPartitionSubsystem, const ULandscapeInfo* InLandscapeInfo, const UActorPartitionSubsystem::FCellCoord& InCellCoord);

	/**
	 * LayerInfo 에셋을 생성하는 경로 전용 오버로드(랜드스케이프 등록 없음).
	 * 동일 경로에 에셋이 이미 있으면 로드해 재사용하고, 없으면 새로 생성합니다.
	 * 위 ALandscape* 포함 오버로드가 이 함수에 위임합니다.
	 */
	static ULandscapeLayerInfoObject* CreateLayerInfo(const FString& InPackagePath, const FString& InAssetName, const ULandscapeLayerInfoObject* InTemplate = nullptr);
};
