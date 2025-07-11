#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class IDetailsView;
class AOCGLevelGenerator;
class SBox;

class SOCGWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SOCGWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SOCGWidget();

private:
	// Button click event handler
	FReply OnCreateLevelGeneratorClicked();
	FReply OnGenerateLevelClicked();
	FReply OnCreateNewMapPresetClicked();

	// 에셋 피커(드롭다운) 관련 함수
	FString GetMapPresetPath() const;
	void OnMapPresetChanged(const FAssetData& AssetData);
	
	// Editor event handler
	void OnActorSelectionChanged(UObject* NewSelection);
	void OnLevelActorDeleted(AActor* InActor);

	// UI 상태 및 업데이트 함수
	bool IsGenerateEnabled() const;

	void UpdateSelectedActor();
	void SetSelectedActor(AOCGLevelGenerator* NewActor);

	void RefreshDetailsView();
	void ClearUI();
	
	void ShowMapPresetDetails();
	void ClearDetailsView();

	void RegisterDelegates();
	void UnregisterDelegates();

	void FindExistingLevelGenerator();

	TWeakObjectPtr<class AOCGLevelGenerator> LevelGeneratorActor;
	TSharedPtr<IDetailsView> MapPresetDetailsView;
	TSharedPtr<SBox> DetailsContainer;
	FDelegateHandle OnLevelActorDeletedDelegateHandle;

	bool bLevelGeneratorExistsInLevel = false;
	void CheckForExistingLevelGenerator();
	FText GetGeneratorButtonText() const;
	FReply OnGeneratorButtonClicked();
	bool IsGeneratorButtonEnabled() const;
};