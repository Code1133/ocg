#pragma once

#include "CoreMinimal.h"
#include "EditorStyleSet.h"
#include "Framework/Commands/Commands.h"

class FMapPresetEditorCommands : public TCommands<FMapPresetEditorCommands>
{
public:
	FMapPresetEditorCommands();

	virtual void RegisterCommands() override;

public:
	TSharedPtr<FUICommandInfo> GenerateAction;
};