// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "OCGEditorSubsystem.generated.h"

class UMapPreset;
class FSpawnTabArgs;
class SDockTab;

/**
 * OCG 생성 파이프라인의 단일 진입점 에디터 서브시스템
 * 툴바 버튼 등록을 담당하며 4단계 파이프라인 실행을 조율합니다.
 */
UCLASS()
class ONEBUTTONLEVELGENERATION_API UOCGEditorSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Preset을 검증한 뒤 DataGeneration -> LandscapeGen -> Population -> Hydrology 순서로 파이프라인을 실행합니다.
	 * @param Preset 생성 설정 에셋
	 */
	void ExecuteGeneration(const UMapPreset* Preset);

	/**
	 * OCG Window 탭을 열거나 이미 열려 있으면 포커스를 줍니다.
	 */
	void OpenOCGWindow();

	/**
	 * Hydrology(강) 단계만 재실행합니다.
	 * 이전 DataGeneration 결과(캐시된 DataContainer)를 사용하므로
	 * Generate All보다 빠릅니다.
	 *
	 * @param Preset 생성 설정 에셋
	 */
	void RegenerateRiverOnly(const UMapPreset* Preset);

	/**
	 * 현재 월드의 모든 AOCGLandscapeVolume에서 PCG 그래프를 강제 재실행합니다.
	 */
	void ForcePCGRegenerate();

private:
	void RegisterToolbarEntry();
	void UnregisterToolbarEntry();

	/** 툴바 버튼 클릭 시 OCG Window 탭을 엽니다. */
	void OnGenerateClicked();

	/** SDockTab 스포너: OCG Window Slate 위젯을 담은 탭을 생성합니다. */
	TSharedRef<SDockTab> SpawnOCGWindowTab(const FSpawnTabArgs& Args);

	/**
	 * Preset 유효성을 검사합니다. 실패 시 다이얼로그를 표시하고 false를 반환합니다.
	 * @param Preset 검증할 설정 에셋
	 * @return 유효하면 true
	 */
	bool ValidatePreset(const UMapPreset* Preset) const;

	/** 마지막 Preset 경로를 EditorPerProjectIni에 저장합니다. */
	void PersistLastUsedPreset(const UMapPreset* Preset);

	/** EditorPerProjectIni에서 마지막 Preset 경로를 복원합니다. */
	void RestoreLastUsedPreset();

	/**
	 * UMapPreset::OnPropertyChanged 델리게이트 핸들러
	 * DataAsset 대신 에디터 레이어에서 월드 액터를 업데이트합니다.
	 *
	 * @param Preset 변경된 프리셋
	 * @param PropertyName 변경된 프로퍼티 이름
	 */
	void OnMapPresetPropertyChanged(const UMapPreset* Preset, FName PropertyName);

	/** 설정된 에셋 경로 검증을 예약합니다. */
	void ScheduleSettingsValidation();

private:
	// 최근 사용된 프리셋. ForcePCGRegenerate가 대상 볼륨을 찾을 때 사용합니다.
	UPROPERTY()
	TSoftObjectPtr<UMapPreset> LastUsedPresetAsset;

	// 에셋 로딩 완료 후 트리거용 핸들
	FDelegateHandle OnFilesLoadedHandle;
};
