// Copyright (c) 2025 Code1133. All rights reserved.

#include "Editor/MapPresetApplicationMode.h"
#include "Editor/MapPresetEditorToolkit.h" 

extern const FName GMapPresetEditor_ViewportTabId;
extern const FName GMapPresetEditor_DetailsTabId;
extern const FName GMapPresetEditor_EnvLightMixerTabId;

FMapPresetApplicationMode::FMapPresetApplicationMode(TSharedPtr<FMapPresetEditorToolkit> InEditorToolkit)
	:FApplicationMode(TEXT("DefaultMode")), MyToolkit(InEditorToolkit)
{
	TabLayout = FTabManager::NewLayout("Standalone_MapPresetEditor_Layout_v2")
		->AddArea
		(
			FTabManager::NewPrimaryArea()->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.7f)
				->AddTab(GMapPresetEditor_ViewportTabId, ETabState::OpenedTab)
				->SetHideTabWell(true)
			)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.3f)
				->AddTab(GMapPresetEditor_DetailsTabId, ETabState::OpenedTab)
			)
		);
}

void FMapPresetApplicationMode::RegisterTabFactories(TSharedPtr<FTabManager> InTabManager)
{
	// Validate the toolkit pointer before proceeding
	if (TSharedPtr<FMapPresetEditorToolkit> Toolkit = MyToolkit.Pin())
	{
		// Register the tab spawners with the toolkit
		Toolkit->RegisterTabSpawners(InTabManager.ToSharedRef());
	}
	FApplicationMode::RegisterTabFactories(InTabManager);
}

