// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "OCGPopulationSubsystem.generated.h"

class UMapPreset;
class AOCGLandscapeVolume;

/**
 * 바이옴 데이터를 기반으로 폴리지와 오브젝트 배치를 담당하는 에디터 서브시스템
 */
UCLASS()
class ONEBUTTONLEVELGENERATION_API UOCGPopulationSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * PCG Volume을 월드에 배치하고 PCG 그래프를 실행합니다.
	 * @param Preset 생성 설정 에셋
	 */
	void ApplyPopulation(const UMapPreset* Preset);

private:
	UPROPERTY()
	TSoftObjectPtr<AOCGLandscapeVolume> CachedVolumeAsset;
};