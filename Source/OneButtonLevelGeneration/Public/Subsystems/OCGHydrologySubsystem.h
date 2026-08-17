// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "OCGHydrologySubsystem.generated.h"

class UMapPreset;
class ALandscape;
class AWaterBodyRiver;
class AWaterBodyOcean;
struct FOCGWorldDataContainer;

/**
 * 강(River)의 경로 탐색/배치와 해양(Ocean) 워터 바디 생성을 담당하는 에디터 서브시스템
 */
UCLASS()
class ONEBUTTONLEVELGENERATION_API UOCGHydrologySubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * DataContainer의 높이맵과 Preset의 River/Ocean 설정을 바탕으로 워터 바디를 생성합니다.
	 *
	 * @param Preset 생성 설정 에셋
	 * @param DataContainer 높이맵 및 현재 높이 범위(CurMinHeight / CurMaxHeight)가 담긴 컨테이너
	 */
	void ApplyHydrology(const UMapPreset* Preset, const FOCGWorldDataContainer& DataContainer);

private:
	void GenerateRivers(UWorld* World, ALandscape* InLandscape, const UMapPreset* Preset, const FOCGWorldDataContainer& DataContainer);
	void CreateOcean(UWorld* World, ALandscape* InLandscape, const UMapPreset* Preset, const FOCGWorldDataContainer& DataContainer, const FVector& VolumeOrigin, const FVector& VolumeExtent);
	void ApplyWaterWeight(ALandscape* InLandscape, const UMapPreset* Preset);
	void ExportWaterEditLayerHeightMap(ALandscape* InLandscape, const UMapPreset* Preset, uint16 MinDiffThreshold = 1);
	void ClearAllRivers(ALandscape* InLandscape);

	void SetDefaultRiverProperties(AWaterBodyRiver* InRiverActor, const TArray<FVector>& InRiverPath, const UMapPreset* Preset);
	void AddRiverProperties(AWaterBodyRiver* InRiverActor, const TArray<FVector>& InRiverPath, const UMapPreset* Preset) const;

	static void SimplifyPathRDP(const TArray<FVector>& InPoints, TArray<FVector>& OutPoints, float Epsilon);

	/**
	 * 높이 기준을 초과하는 픽셀을 강 발원 후보로 캐싱합니다.
	 *
	 * @param HeightMapData uint16 높이맵 데이터
	 * @param Preset 생성 설정 에셋 (MapResolution, RiverSourceElevationRatio, SeaLevel 참조)
	 * @param CurMinHeight 현재 파이프라인에서 계산된 높이맵 최솟값 (월드 높이, cm)
	 * @param CurMaxHeight 현재 파이프라인에서 계산된 높이맵 최댓값 (월드 높이, cm)
	 */
	void CacheRiverStartPoints(
		const TArray<uint16>& HeightMapData,
		const UMapPreset* Preset,
		float CurMinHeight,
		float CurMaxHeight
	);

	FIntPoint GetRandomStartPoint(int32 RiverIndex, const UMapPreset* Preset) const;

private:
	UPROPERTY()
	TObjectPtr<AWaterBodyOcean> CachedOcean;

	UPROPERTY()
	TSoftObjectPtr<AWaterBodyOcean> CachedOceanAsset;

	UPROPERTY()
	TArray<TObjectPtr<AWaterBodyRiver>> GeneratedRivers;

	UPROPERTY()
	TArray<TSoftObjectPtr<AWaterBodyRiver>> CachedRivers;

	/** 강 경로 탐색 시작 후보 픽셀 좌표 캐시 */
	TArray<FIntPoint> CachedRiverStartPoints;

	/** 워터 레이어 높이 차이 맵 (ApplyWaterWeight용) */
	TArray<uint16> CachedWaterHeightMap;
	int32 WaterHeightMapWidth = 0;
	int32 WaterHeightMapHeight = 0;

	/** 이전 생성 시 마스크된 레이어 웨이트맵 (재생성 시 복원용) */
	TMap<FName, TArray<uint8>> PrevRiverMaskedWeights;

	float SeaHeight = 0.0f;
	int32 CurrentRiverSeed = INDEX_NONE;
	bool bIsRiverExists = false;
};
