// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "Component/OCGLandscapeGenerateComponent.h"
#include "OCGLandscapeGenSubsystem.generated.h"

class UMapPreset;
class ALandscape;
class URuntimeVirtualTexture;
class ARuntimeVirtualTextureVolume;
struct FOCGWorldDataContainer;
struct FOCGBiomeSettings;

/**
 * 생성된 맵 데이터를 랜드스케이프(Landscape) 액터에 적용하고 바이옴 레이어 마무리 및 RVT 볼륨 생성을 담당하는 에디터 서브시스템
 */
UCLASS()
class ONEBUTTONLEVELGENERATION_API UOCGLandscapeGenSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * DataContainer의 높이맵과 바이옴 레이어를 Landscape 액터에 적용합니다.
	 * DataContainer.HeightMapData를 바이옴 최소 높이 기준으로 In-place 수정합니다.
	 *
	 * @param Preset 생성 설정 에셋
	 * @param DataContainer 높이맵, 바이옴 레이어 등 런타임 결과를 담는 컨테이너
	 */
	void ApplyLandscape(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer);

	/**
	 * 스폰된 Landscape 액터를 반환합니다.
	 * @return 현재 생성된 Landscape 액터. 아직 생성되지 않았으면 nullptr
	 */
	FORCEINLINE ALandscape* GetLandscape() const { return TargetLandscape; }

	/**
	 * RVT 볼륨의 월드 원점을 반환합니다.
	 * @return RVT 볼륨 중심의 월드 좌표
	 */
	FORCEINLINE FVector GetVolumeOrigin() const { return VolumeOrigin; }

	/**
	 * RVT 볼륨의 반 크기(Half-extent)를 반환합니다.
	 *
	 * @return RVT 볼륨의 각 축 방향 반 크기 벡터
	 */
	FORCEINLINE FVector GetVolumeExtent() const { return VolumeExtent; }

	/**
	 * MapPoint 픽셀 좌표를 Landscape 위 실제 월드 좌표로 변환합니다.
	 * InHeightMapData가 제공되면 GetHeightAtLocation 실패 시 높이맵 기반 fallback을 사용합니다.
	 *
	 * @param MapPoint 변환할 맵 픽셀 좌표
	 * @param Preset 생성 설정 에셋 (해상도, 스케일 참조용)
	 * @param InHeightMapData fallback 높이 계산에 사용할 높이맵 데이터. nullptr이면 fallback 미사용
	 * @return 해당 픽셀 위치의 월드 좌표
	 */
	FVector GetLandscapePointWorldPosition(const FIntPoint& MapPoint, const UMapPreset* Preset, const TArray<uint16>* InHeightMapData = nullptr) const;

private:
	void InitializeLandscapeSetting(const UMapPreset* Preset);
	bool ShouldCreateNewLandscape(const UMapPreset* Preset);
	static bool IsLandscapeSettingChanged(const FLandscapeSetting& Prev, const FLandscapeSetting& Curr);
	bool CreateRuntimeVirtualTextureVolume(ALandscape* InLandscape);

	/**
	 * 바이옴 설정에 따라 높이맵을 수정합니다.
	 * @note 원본: UOCGMapGenerateComponent::ModifyLandscapeWithBiome
	 *
	 * @param Preset 생성 설정 에셋
	 * @param DataContainer 높이맵과 바이옴 레이어가 담긴 컨테이너 (HeightMapData In-place 수정됨)
	 * @param ZScale Landscape Z 스케일
	 * @param ZOffset Landscape Z 오프셋
	 */
	void ModifyLandscapeWithBiome(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer, float ZScale, float ZOffset);

	/**
	 * 각 바이옴 연결 영역의 최소 높이를 계산합니다.
	 * @note 원본: UOCGMapGenerateComponent::CalculateBiomeMinHeights
	 *
	 * @param Preset 맵 해상도 등 설정 참조용
	 * @param InHeightMap uint16 높이맵 데이터
	 * @param InBiomeMap 픽셀별 바이옴 포인터 배열
	 * @param OutMinHeights 픽셀별 바이옴 영역 최소 높이 출력 배열
	 * @param ZScale Landscape Z 스케일
	 * @param ZOffset Landscape Z 오프셋
	 */
	void CalculateBiomeMinHeights(const UMapPreset* Preset, const TArray<uint16>& InHeightMap, const TArray<const FOCGBiomeSettings*>& InBiomeMap, TArray<float>& OutMinHeights, float ZScale, float ZOffset);

	/**
	 * 바이옴 최소 높이 배열에 슬라이딩 윈도우 박스 블러를 적용합니다.
	 * @note 원본: UOCGMapGenerateComponent::BlurBiomeMinHeights
	 *
	 * @param Preset BiomeHeightBlendRadius, 맵 해상도 참조용
	 * @param InMinHeights 입력 최소 높이 배열
	 * @param OutMinHeights 블러 처리된 최소 높이 출력 배열
	 */
	static void BlurBiomeMinHeights(const UMapPreset* Preset, const TArray<float>& InMinHeights, TArray<float>& OutMinHeights);

	/**
	 * BFS로 동일 바이옴 연결 영역을 탐색해 최소 높이를 구합니다.
	 * @note 원본: UOCGMapGenerateComponent::GetBiomeStats
	 *
	 * @param MapSize 맵 해상도
	 * @param X 탐색 시작 픽셀 X 좌표
	 * @param Y 탐색 시작 픽셀 Y 좌표
	 * @param RegionID 이번 탐색에 할당할 영역 식별자
	 * @param OutMinHeight 탐색된 영역의 최소 월드 높이 출력값
	 * @param RegionIDMap 픽셀별 영역 ID 기록 배열 (In-place 갱신)
	 * @param InHeightMap uint16 높이맵 데이터
	 * @param InBiomeMap 픽셀별 바이옴 포인터 배열
	 * @param ZScale Landscape Z 스케일
	 * @param ZOffset Landscape Z 오프셋
	 */
	static void GetBiomeStats(FIntPoint MapSize, int32 X, int32 Y, int32 RegionID, float& OutMinHeight, TArray<int32>& RegionIDMap, const TArray<uint16>& InHeightMap, const TArray<const FOCGBiomeSettings*>& InBiomeMap, float ZScale, float ZOffset);

private:
	UPROPERTY()
	TObjectPtr<ALandscape> TargetLandscape;

	UPROPERTY()
	TSoftObjectPtr<ALandscape> TargetLandscapeAsset;

	/** InitializeLandscapeSetting에서 계산된 값. ShouldCreateNewLandscape의 이전 값 비교에 사용. */
	FLandscapeSetting LandscapeSetting;

	FVector VolumeExtent = FVector::ZeroVector;
	FVector VolumeOrigin = FVector::ZeroVector;

	UPROPERTY()
	TArray<TObjectPtr<ARuntimeVirtualTextureVolume>> CachedRuntimeVirtualTextureVolumes;

	UPROPERTY()
	TArray<TSoftObjectPtr<ARuntimeVirtualTextureVolume>> CachedRuntimeVirtualTextureVolumeAssets;

	UPROPERTY()
	TObjectPtr<URuntimeVirtualTexture> ColorRVT;

	UPROPERTY()
	TObjectPtr<URuntimeVirtualTexture> HeightRVT;

	UPROPERTY()
	TObjectPtr<URuntimeVirtualTexture> DisplacementRVT;
};