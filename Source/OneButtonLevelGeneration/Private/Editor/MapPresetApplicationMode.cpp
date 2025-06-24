#include "Editor/MapPresetApplicationMode.h"
#include "Editor/MapPresetEditorToolkit.h" // 툴킷 헤더

extern const FName GMapPresetEditor_ViewportTabId;
extern const FName GMapPresetEditor_DetailsTabId;

FMapPresetApplicationMode::FMapPresetApplicationMode(TSharedPtr<FMapPresetEditorToolkit> InEditorToolkit)
	:FApplicationMode(TEXT("DefaultMode")), MyToolkit(InEditorToolkit)
{
	TSharedPtr<FMapPresetEditorToolkit> ToolkitPin = MyToolkit.Pin();

	if (ToolkitPin.IsValid())
	{
		// 이 모드에섯 사용할 탭 팩토리들을 등록
		TabFactories.RegisterFactory(MakeShared<FWorkflowTabFactory>(GMapPresetEditor_ViewportTabId, MyToolkit.Pin()));
		TabFactories.RegisterFactory(MakeShared<FWorkflowTabFactory>(GMapPresetEditor_DetailsTabId, MyToolkit.Pin()));
	}

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
	if (TSharedPtr<FMapPresetEditorToolkit> Toolkit = MyToolkit.Pin())
	{
		// 툴킷의 탭 스패너들을 TabManager에 등록합니다.
		Toolkit->RegisterTabSpawners(InTabManager.ToSharedRef());
	}
}
