#include "Editor/MapPresetEditorCommands.h"

FMapPresetEditorCommands::FMapPresetEditorCommands()
	:TCommands<FMapPresetEditorCommands>(
			TEXT("MapPresetEditor"),
			FText::FromString(TEXT("Map Preset Editor")),
			NAME_None,
			FEditorStyle::GetStyleSetName()
		)
{
}

#define LOCTEXT_NAMESPACE "MapPresetEditorCommands"

void FMapPresetEditorCommands::RegisterCommands()
{
	UI_COMMAND(GenerateAction, "Generate", "Generate a new level based on the preset settings.",
		EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE

