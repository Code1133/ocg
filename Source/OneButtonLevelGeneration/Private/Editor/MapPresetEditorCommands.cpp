// Copyright (c) 2025 Code1133. All rights reserved.

#include "Editor/MapPresetEditorCommands.h"

FMapPresetEditorCommands::FMapPresetEditorCommands()
	: TCommands(
		TEXT("MapPresetEditor"),
		FText::FromString(TEXT("Map Preset Editor")),
		NAME_None,
		FAppStyle::GetAppStyleSetName()
	)
{
}

#define LOCTEXT_NAMESPACE "MapPresetEditorCommands"

void FMapPresetEditorCommands::RegisterCommands()
{
	UI_COMMAND(GenerateAction, "Generate", "Generate a new level based on the preset settings.",
		EUserInterfaceActionType::Button, FInputChord());

	UI_COMMAND(ExportToLevelAction, "Export to Level", "Export current preset to a new level.",
		EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE

