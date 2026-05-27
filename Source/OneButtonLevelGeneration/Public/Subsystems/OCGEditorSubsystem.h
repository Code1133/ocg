// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "OCGEditorSubsystem.generated.h"

class UMapPreset;
struct IConsoleCommand;

/**
 * OCG 생성 파이프라인의 단일 진입점 에디터 서브시스템
 * 툴바 버튼, 콘솔 커맨드 등록을 담당하며 4단계 파이프라인 실행을 조율합니다.
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
	 * 마지막으로 사용된 Preset으로 생성을 재실행합니다.
	 * 저장된 Preset이 없으면 경고 로그를 남기고 반환합니다.
	 */
	void RegenerateLast();

	/**
	 * 마지막으로 사용된 Preset을 반환합니다.
	 * @return 아직 로드되지 않은 경우 nullptr
	 */
	const UMapPreset* GetLastUsedPreset() const;

private:
	void RegisterToolbarEntry();
	void UnregisterToolbarEntry();
	void RegisterConsoleCommand();
	void UnregisterConsoleCommand();

	/** 툴바 버튼 클릭 시 MapPreset 선택 다이얼로그를 열고 생성을 실행합니다. */
	void OnGenerateClicked();

	/** 콘솔 커맨드 "OCG.Generate [/Game/Path/To/Preset]" 핸들러. */
	void OnConsoleGenerate(const TArray<FString>& Args);

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

private:
	UPROPERTY()
	TSoftObjectPtr<UMapPreset> LastUsedPresetAsset;

	IConsoleCommand* GenerateConsoleCommand = nullptr;
};
