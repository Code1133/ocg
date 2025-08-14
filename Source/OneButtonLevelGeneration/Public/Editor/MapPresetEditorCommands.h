// Copyright (c) 2025 Code1133. All rights reserved.

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
