// Copyright (c) 2025 Code1133. All rights reserved.
// [DEPRECATED v2] Custom MapPreset editor. Replaced by default property editor. Scheduled for removal.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"


class FMapPresetEditorCommands : public TCommands<FMapPresetEditorCommands>
{
public:
	FMapPresetEditorCommands();

	virtual void RegisterCommands() override;

public:
	TSharedPtr<FUICommandInfo> PreviewMapAction;
	TSharedPtr<FUICommandInfo> GenerateAction;
	TSharedPtr<FUICommandInfo> ExportToLevelAction;
	TSharedPtr<FUICommandInfo> RegenerateRiverAction;
	TSharedPtr<FUICommandInfo> ForceGeneratePCGAction;
};
