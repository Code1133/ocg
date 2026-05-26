// Copyright (c) 2025 Code1133. All rights reserved.
// [DEPRECATED v2] Custom MapPreset editor. Replaced by default property editor. Scheduled for removal.

#pragma once

#include "CoreMinimal.h"
#include "WorkflowOrientedApp/ApplicationMode.h"


class FMapPresetEditorToolkit;

class FMapPresetApplicationMode : public FApplicationMode
{
public:
	FMapPresetApplicationMode(const TSharedPtr<FMapPresetEditorToolkit>& InEditorToolkit);

	virtual void RegisterTabFactories(TSharedPtr<FTabManager> InTabManager) override;
protected:
	TWeakPtr<FMapPresetEditorToolkit> MyToolkit; 
};
