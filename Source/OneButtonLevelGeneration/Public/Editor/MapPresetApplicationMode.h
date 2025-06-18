#pragma once

#include "CoreMinimal.h"
#include "WorkflowOrientedApp/ApplicationMode.h"
#include "WorkflowOrientedApp/WorkflowTabManager.h"

class FMapPresetEditorToolkit;

class FMapPresetApplicationMode : public FApplicationMode
{
public:
	FMapPresetApplicationMode(TSharedPtr<FMapPresetEditorToolkit> InEditorToolkit);

	virtual void RegisterTabFactories(TSharedPtr<FTabManager> InTabManager) override;

protected:
	// 이 모드에서 사용할 탭 팩토리들의 목록
	FWorkflowAllowedTabSet TabFactories;

	// 이 모드를 소유하는 툴킷
	TWeakPtr<FMapPresetEditorToolkit> MyToolkit; 
};
