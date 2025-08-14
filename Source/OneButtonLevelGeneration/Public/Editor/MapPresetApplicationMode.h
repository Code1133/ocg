// Copyright (c) 2025 Code1133. All rights reserved.

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
