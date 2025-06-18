#include "Editor/MapPresetEditorToolkit.h"

#include "AdvancedPreviewScene.h"
#include "SEditorViewport.h"
#include "Data/MapPreset.h"
#include "Editor/MapPresetApplicationMode.h"
#include "Editor/MapPresetEditorTabFactory.h"
#include "Editor/MapPresetViewport.h"

const FName GMapPresetEditor_ViewportTabId(TEXT("MapPresetEditor_Viewport"));
const FName GMapPresetEditor_DetailsTabId(TEXT("MapPresetEditor_Details"));

void FMapPresetEditorToolkit::InitEditor(const EToolkitMode::Type Mode,
                                         const TSharedPtr<class IToolkitHost>& InitToolkitHost, UMapPreset* MapPreset)
{
	EditingPreset = MapPreset;

	// 프리뷰 씬 생성
	PreviewScene = MakeShareable(new FAdvancedPreviewScene(FPreviewScene::ConstructionValues()));
	// 월드 바운드 체크 비활성화
	if (PreviewScene->GetWorld() && PreviewScene->GetWorld()->GetWorldSettings())
	{
		PreviewScene->GetWorld()->GetWorldSettings()->bEnableWorldBoundsChecks = false;
	}

	// 뷰포트 위젯 생성
	ViewportWidget = SNew(SMapPresetViewport, PreviewScene).MapPresetEditorToolkit(SharedThis(this));
	ViewportWidget->SetRenderDirectlyToWindow(true);

	// 에디터 모드 추가
	AddApplicationMode(
		TEXT("DefaultMode"),
		MakeShareable(new FMapPresetApplicationMode(SharedThis(this)))
	);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;

	InitAssetEditor(Mode, InitToolkitHost, FName("MapPresetEditor"), FTabManager::FLayout::NullLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, EditingPreset);

	SetCurrentMode(TEXT("DefaultMode"));
}

FName FMapPresetEditorToolkit::GetToolkitFName() const
{
	return FName("MapPresetEditor");
}

FText FMapPresetEditorToolkit::GetBaseToolkitName() const
{
	return FText::FromString(TEXT("Map Preset Editor"));
}

FString FMapPresetEditorToolkit::GetWorldCentricTabPrefix() const
{
	return TEXT("MapPresetEditor");
}

FLinearColor FMapPresetEditorToolkit::GetWorldCentricTabColorScale() const
{
	return FLinearColor::White;
}

FMapPresetEditorToolkit::~FMapPresetEditorToolkit()
{
}

UWorld* FMapPresetEditorToolkit::GetPreviewWorld() const
{
	return PreviewScene.IsValid() ? PreviewScene->GetWorld() : nullptr;
}

void FMapPresetEditorToolkit::RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	InTabManager->RegisterTabSpawner(GMapPresetEditor_ViewportTabId, FOnSpawnTab::CreateSP(this, &FMapPresetEditorToolkit::SpawnTab_Viewport))
	.SetDisplayName(FText::FromString(TEXT("Viewport")));

	InTabManager->RegisterTabSpawner(GMapPresetEditor_DetailsTabId, FOnSpawnTab::CreateSP(this, &FMapPresetEditorToolkit::SpawnTab_Details))
		.SetDisplayName(FText::FromString(TEXT("Details")));
}

void FMapPresetEditorToolkit::UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	FWorkflowCentricApplication::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(ViewportTabId);
	InTabManager->UnregisterTabSpawner(DetailsTabId);
}

TSharedRef<SDockTab> FMapPresetEditorToolkit::SpawnTab_Viewport(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == ViewportTabId);

	return SNew(SDockTab)
		.Label(FText::FromString(TEXT("Viewport")))
		[
			// 생성해 둔 뷰포트 위젯을 탭에 배치
			ViewportWidget.ToSharedRef()
		];
}

TSharedRef<SDockTab> FMapPresetEditorToolkit::SpawnTab_Details(const FSpawnTabArgs& Args)
{
	// 탭에 표시할 Slate UI 생성
	check(Args.GetTabId() == DetailsTabId);

	return SNew(SDockTab)
		.Label(FText::FromString(TEXT("Details")))
		[
			// FMapPresetEditorTabFactory를 통해 디테일 뷰를 생성합니다.
			FMapPresetEditorTabFactory::CreateTabBody(SharedThis(this))
		];
}
