// Copyright (c) 2025 Code1133. All rights reserved.

#include "Editor/MapPresetApplicationMode.h"
#include "Editor/MapPresetEditorToolkit.h"

FMapPresetApplicationMode::FMapPresetApplicationMode(const TSharedPtr<FMapPresetEditorToolkit>& InEditorToolkit)
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
				->AddTab(FMapPresetEditorConstants::ViewportTabId, ETabState::OpenedTab)
				->SetHideTabWell(true)
			)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.3f)
				->AddTab(FMapPresetEditorConstants::DetailsTabId, ETabState::OpenedTab)
			)
		);
}

void FMapPresetApplicationMode::RegisterTabFactories(TSharedPtr<FTabManager> InTabManager)
{
	// Validate the toolkit pointer before proceeding
	if (const TSharedPtr<FMapPresetEditorToolkit> Toolkit = MyToolkit.Pin())
	{
		const TSharedRef<FTabManager> TabManagerRef = InTabManager.ToSharedRef();

		// Register the tab spawners with the toolkit
		Toolkit->RegisterTabSpawners(TabManagerRef);

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 8
		RegisterTabFactoriesWithAppAndManager(Toolkit.Get(), TabManagerRef);
#endif
	}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 8
	FApplicationMode::RegisterTabFactories(InTabManager);
#endif
}
