// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once


#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "WorkflowOrientedApp/WorkflowCentricApplication.h"

class FMapPresetViewportClient;
class UMaterialEditorInstanceConstant;
class UMapPreset;

DECLARE_MULTICAST_DELEGATE(FOnGenerateButtonClicked);
DECLARE_MULTICAST_DELEGATE(FOnExportToLevelButtonClicked);

struct FMapPresetEditorConstants
{
	inline static const FName ViewportTabId = TEXT("MapPresetEditor_Viewport");
	inline static const FName DetailsTabId = TEXT("MapPresetEditor_Details");
	inline static const FName MaterialDetailsTabId = TEXT("MapPresetEditor_MaterialDetails");
	inline static const FName EnvLightMixerTabId = TEXT("MapPresetEditor_EnvLightMixer");
	inline static const FName LandscapeDetailsTabId = TEXT("MapPresetEditor_LandscapeDetails");
};

class FMapPresetEditorToolkit : public FWorkflowCentricApplication, public FNotifyHook
{
	friend class FMapPresetApplicationMode; 

public:
	/** Initialize the Map Preset Editor Toolkit */
	void InitEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UMapPreset* MapPreset);

	/** IToolkit Interface */ 
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;

	/** Getter for the MapPreset */
	UMapPreset* GetMapPreset() const { return EditingPreset.Get(); }

	virtual ~FMapPresetEditorToolkit() override;

protected:
	/** Register TabSpawners */
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	/** Unregister TabSpawners */
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;

	/** Tab Spawner Functions */
	TSharedRef<SDockTab> SpawnTab_Viewport(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Details(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_EnvLightMixerTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_LandscapeTab(const FSpawnTabArgs& Args);

	TSharedRef<SWidget> CreateMapPresetTabBody();
	TSharedRef<SWidget> CreateLandscapeTabBody();
	
private:
	TWeakObjectPtr<UMapPreset> EditingPreset = nullptr;
	/** Viewport Widget */
	TSharedPtr<class SMapPresetViewport> ViewportWidget;
	/** Environment Lighting Viewer Widget */
	TSharedPtr<class SMapPresetEnvironmentLightingViewer> EnvironmentLightingViewer;
	/** Landscape Details View */
	TSharedPtr<IDetailsView> LandscapeDetailsView;
	
private:
	void FillToolbar(FToolBarBuilder& ToolbarBuilder);
	FReply OnGenerateClicked();
	FReply OnExportToLevelClicked();
	FReply OnRegenerateRiverClicked();
	FReply OnForceGeneratePCGClicked();

	UWorld* CreateEditorWorld();
	void SetupDefaultActors();
	void ExportPreviewSceneToLevel();
	void Generate() const;

private:
	TObjectPtr<UWorld> MapPresetEditorWorld; 
	TWeakObjectPtr<class AOCGLevelGenerator> LevelGenerator; 
};
