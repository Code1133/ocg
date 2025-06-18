#pragma once


#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "WorkflowOrientedApp/WorkflowCentricApplication.h"

class UMapPreset;

class FMapPresetEditorToolkit : public FWorkflowCentricApplication
{
public:
	/**
	 * 에디터를 초기화합니다. 에셋 타입 액션에서 호출됩니다.
	 * @param Mode 에디터 모드 (Standalone 또는 WorldCentric)
	 * @param InitToolkitHost 툴킷을 호스팅하는 상위 툴킷
	 * @param InMapPreset 편집할 UMapPreset 에셋
	 */
	void InitEditor(const EToolkitMode::Type Mode, const TSharedPtr<class IToolkitHost>& InitToolkitHost, UMapPreset* MapPreset);

	// IToolkit 인터페이스
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;

	// UMapPreset 에셋에 접근하기 위한 함수
	UMapPreset* GetMapPreset() const { return EditingPreset; }

	virtual ~FMapPresetEditorToolkit();
	UWorld* GetPreviewWorld() const;
	
	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager) override;
protected:
	// 탭 스패너 등록
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager) override;

	// 뷰포트 탭 생성 함수
	TSharedRef<SDockTab> SpawnTab_Viewport(const FSpawnTabArgs& Args);
	// 디테일 탭 생성 함수
	TSharedRef<SDockTab> SpawnTab_Details(const FSpawnTabArgs& Args);
	
private:
	// 편집 중인 UMapPreset 에셋
	UMapPreset* EditingPreset = nullptr;

	// 독립적인 레벨을 담을 프리뷰 씬
	TSharedPtr<class FAdvancedPreviewScene> PreviewScene;
	// 뷰포트 위젯
	TSharedPtr<class SMapPresetViewport> ViewportWidget;

	// 탭 식별을 위한 고유 FName
	static const FName ViewportTabId;
	static const FName DetailsTabId;
};
