#include "Editor/MapPresetApplicationMode.h"
#include "Editor/MapPresetEditorToolkit.h" // 툴킷 헤더

extern const FName GMapPresetEditor_ViewportTabId;
extern const FName GMapPresetEditor_DetailsTabId;
extern const FName GMapPresetEditor_MaterialDetailsTabId;
extern const FName GMapPresetEditor_EnvLightMixerTabId;

FMapPresetApplicationMode::FMapPresetApplicationMode(TSharedPtr<FMapPresetEditorToolkit> InEditorToolkit)
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
				->AddTab(GMapPresetEditor_ViewportTabId, ETabState::OpenedTab)
				->SetHideTabWell(true)
			)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.3f)
				->AddTab(GMapPresetEditor_DetailsTabId, ETabState::OpenedTab)
				->AddTab(GMapPresetEditor_MaterialDetailsTabId, ETabState::OpenedTab)
			)
		);
}

void FMapPresetApplicationMode::RegisterTabFactories(TSharedPtr<FTabManager> InTabManager)
{
	// 툴킷의 유효성을 확인합니다.
	if (TSharedPtr<FMapPresetEditorToolkit> Toolkit = MyToolkit.Pin())
	{
		// 이 모드에서 사용할 탭 팩토리들을 TabManager에 등록합니다.
		Toolkit->RegisterTabSpawners(InTabManager.ToSharedRef());
	}
	FApplicationMode::RegisterTabFactories(InTabManager);
}

