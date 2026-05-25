// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "Data/OCGWorldDataContainer.h"
#include "OCGDataGenerationSubsystem.generated.h"

class UMapPreset;
class UOCGHeightmapStrategyBase;

/**
 * 높이맵, 온도맵, 습도맵 생성과 바이옴 배정, 평탄화(Smoothing) 및 침식 데이터 처리를 담당하는 에디터 서브시스템
 */
UCLASS()
class ONEBUTTONLEVELGENERATION_API UOCGDataGenerationSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Preset을 기반으로 전체 맵 데이터를 생성합니다.
	 * 호출 시 DataContainer가 초기화된 후 각 전략(Strategy) 인터페이스가 순서대로 실행됩니다.
	 */
	void GenerateData(const UMapPreset* Preset);

	/** 생성된 맵 데이터 컨테이너에 대한 읽기 전용 참조를 반환합니다. */
	FORCEINLINE const FOCGWorldDataContainer& GetDataContainer() const { return DataContainer; }

private:
	/** 생성 파이프라인의 중간 결과물을 보관하는 런타임 전용 컨테이너. */
	FOCGWorldDataContainer DataContainer;

	/** 높이맵 생성 전략 구현체. GC 대상이 되지 않도록 UPROPERTY로 소유합니다. */
	UPROPERTY()
	TObjectPtr<UOCGHeightmapStrategyBase> HeightmapStrategy;
};
