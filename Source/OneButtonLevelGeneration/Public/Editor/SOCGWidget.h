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

	FString GetMapPresetPath() const;
	void OnMapPresetChanged(const FAssetData& AssetData);
	
	void OnActorSelectionChanged(UObject* NewSelection);
	void OnLevelActorDeleted(AActor* InActor);

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

	void CheckForExistingLevelGenerator();
	FText GetGeneratorButtonText() const;
	FReply OnGeneratorButtonClicked();
	bool IsGeneratorButtonEnabled() const;
	
	TWeakObjectPtr<class AOCGLevelGenerator> LevelGeneratorActor;
	TSharedPtr<IDetailsView> MapPresetDetailsView;
	TSharedPtr<SBox> DetailsContainer;
	FDelegateHandle OnLevelActorDeletedDelegateHandle;

	bool bLevelGeneratorExistsInLevel = false;
};