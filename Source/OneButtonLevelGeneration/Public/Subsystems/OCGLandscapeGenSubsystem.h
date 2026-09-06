// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "Data/OCGLandscapeTypes.h"
#include "OCGLandscapeGenSubsystem.generated.h"

class UMapPreset;
class ALandscape;
class URuntimeVirtualTexture;
class ARuntimeVirtualTextureVolume;
struct FOCGWorldDataContainer;

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
	ALandscape* GetLandscape() { return ResolveLandscape(); }

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

	/** 캐시된 Landscape Actor를 가져옵니다. (없으면 TargetLandscapeAsset으로 Load) */
	[[nodiscard]] ALandscape* ResolveLandscape();

private:
	TWeakObjectPtr<ALandscape> TargetLandscape;

	UPROPERTY()
	TSoftObjectPtr<ALandscape> TargetLandscapeAsset;

	/** InitializeLandscapeSetting에서 계산된 값. ShouldCreateNewLandscape의 이전 값 비교에 사용. */
	FLandscapeSetting LandscapeSetting;

	FVector VolumeExtent = FVector::ZeroVector;
	FVector VolumeOrigin = FVector::ZeroVector;

	TArray<TWeakObjectPtr<ARuntimeVirtualTextureVolume>> CachedRuntimeVirtualTextureVolumes;

	UPROPERTY()
	TArray<TSoftObjectPtr<ARuntimeVirtualTextureVolume>> CachedRuntimeVirtualTextureVolumeAssets;

	UPROPERTY()
	TObjectPtr<URuntimeVirtualTexture> ColorRVT;

	UPROPERTY()
	TObjectPtr<URuntimeVirtualTexture> HeightRVT;

	UPROPERTY()
	TObjectPtr<URuntimeVirtualTexture> DisplacementRVT;
};
