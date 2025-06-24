#pragma once


#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "WorkflowOrientedApp/WorkflowCentricApplication.h"

class UMaterialEditorInstanceConstant;
class UMapPreset;

DECLARE_MULTICAST_DELEGATE(FOnGenerateButtonClicked);
DECLARE_MULTICAST_DELEGATE(FOnExportToLevelButtonClicked);

class FMapPresetEditorToolkit : public FWorkflowCentricApplication, public FNotifyHook
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
	
	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager) override;
protected:
	// 탭 스패너 등록
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager) override;

	// 뷰포트 탭 생성 함수
	TSharedRef<SDockTab> SpawnTab_Viewport(const FSpawnTabArgs& Args);
	// 디테일 탭 생성 함수
	TSharedRef<SDockTab> SpawnTab_Details(const FSpawnTabArgs& Args);
	// 머티리얼 탭 생성 함수
	TSharedRef<SDockTab> SpawnTab_MaterialDetails(const FSpawnTabArgs& Args);
	// 탭의 본문 위젯 생성 함수
	TSharedRef<SWidget> CreateTabBody();
	
private:
	// 편집 중인 UMapPreset 에셋
	UMapPreset* EditingPreset = nullptr;

	// 독립적인 레벨을 담을 프리뷰 씬
	TSharedPtr<class FAdvancedPreviewScene> PreviewScene;
	// 뷰포트 위젯
	TSharedPtr<class SMapPresetViewport> ViewportWidget;

	//머티리얼 디테일 띄우기 위한 프로퍼티들
	TSharedPtr<IDetailsView> MaterialInstanceDetails;

	UPROPERTY()
	TObjectPtr<UMaterialEditorInstanceConstant> MaterialEditorInstance;

	void GetShowHiddenParameters(bool& bShowHiddenParameters) const;
	
	/** Whether or not we should be displaying all the material parameters */
	bool bShowAllMaterialParameters = false;

	// 헬퍼 함수
	void CreateOrUpdateMaterialEditorWrapper(UMaterialInstanceConstant* InMaterialInstance);
	void FilterOverriddenProperties();

private:

	void SetupDefaultActors();
	/** 툴바에 버튼을 추가하는 함수 */
	void FillToolbar(FToolBarBuilder& ToolbarBuilder);

	/** "Generate" 버튼 클릭 시 실행될 함수 */
	FReply OnGenerateClicked();

	/** "Export to Level" 버튼 클릭 시 실행될 함수 */
	FReply OnExportToLevelClicked();
	
	/** 툴킷의 UI 커맨드 리스트 */
	TSharedPtr<FUICommandList> ToolkitCommands;

public:
	FOnGenerateButtonClicked OnGenerateButtonClicked;
	FOnExportToLevelButtonClicked OnExportToLevelButtonClicked;

private:
	TObjectPtr<UWorld> MapPresetEditorWorld; // MapPreset 에디터가 참조하는 월드
};
