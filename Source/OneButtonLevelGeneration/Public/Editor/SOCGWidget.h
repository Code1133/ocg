// Copyright (c) 2025 Code1133. All rights reserved.

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
	virtual ~SOCGWidget() override;

private:
	// Button click event handler
	FReply OnCreateLevelGeneratorClicked();
	FReply OnGenerateLevelClicked();
	FReply OnCreateNewMapPresetClicked();
	void OnMapChanged(uint32 _);

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

	FReply OnRegenerateRiverClicked();
	bool IsRegenerateRiverButtonEnabled() const;

	FReply OnForceGeneratePCGClicked();
	bool IsForceGeneratePCGButtonEnabled() const;

	virtual FReply OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent) override;

	TWeakObjectPtr<class UMapPreset> MapPreset;
	TWeakObjectPtr<AOCGLevelGenerator> LevelGeneratorActor;
	TSharedPtr<IDetailsView> MapPresetDetailsView;
	TSharedPtr<SBox> DetailsContainer;
	FDelegateHandle OnLevelActorDeletedDelegateHandle;

	bool bLevelGeneratorExistsInLevel = false;
};
