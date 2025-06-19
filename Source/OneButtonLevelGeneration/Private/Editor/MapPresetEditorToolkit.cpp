#include "Editor/MapPresetEditorToolkit.h"

#include "AdvancedPreviewScene.h"
#include "SEditorViewport.h"
#include "Data/MapPreset.h"
#include "Editor/MapPresetApplicationMode.h"
#include "Editor/MapPresetEditorCommands.h"
#include "Editor/MapPresetViewport.h"

const FName FMapPresetEditorToolkit::ViewportTabId(TEXT("MapPresetEditor_Viewport"));
const FName FMapPresetEditorToolkit::DetailsTabId(TEXT("MapPresetEditor_Details"));

const FName GMapPresetEditor_ViewportTabId(TEXT("MapPresetEditor_Viewport"));
const FName GMapPresetEditor_DetailsTabId(TEXT("MapPresetEditor_Details"));

void FMapPresetEditorToolkit::InitEditor(const EToolkitMode::Type Mode,
                                         const TSharedPtr<class IToolkitHost>& InitToolkitHost, UMapPreset* MapPreset)
{
	// 커맨드 리스트 및 액션 바인드
	FMapPresetEditorCommands::Register();
	// ToolkitCommands = MakeShareable(new FUICommandList);
	//
	// ToolkitCommands->MapAction(
	// 		FMapPresetEditorCommands::Get().GenerateAction,
	// 		FExecuteAction::CreateSP(this, &FMapPresetEditorToolkit::OnGenerateClicked)
	// 	);

	// 툴바 확장 기능 생성
	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
	ToolbarExtender->AddToolBarExtension(
			"Asset",
			EExtensionHook::After,
			ToolkitCommands,
			FToolBarExtensionDelegate::CreateSP(this, &FMapPresetEditorToolkit::FillToolbar)
		);
	
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

	// 에디터 모드 추가
	AddApplicationMode(
		TEXT("DefaultMode"),
		MakeShareable(new FMapPresetApplicationMode(SharedThis(this)))
	);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;

	InitAssetEditor(Mode, InitToolkitHost, FName("MapPresetEditor"), FTabManager::FLayout::NullLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, EditingPreset);

	// 확장 기능을 에디터에 추가
	AddToolbarExtender(ToolbarExtender);
	// 메뉴 및 툴바를 다시 생성하여 변경사항 적용
	RegenerateMenusAndToolbars();
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
			CreateTabBody()
		];
}

TSharedRef<SWidget> FMapPresetEditorToolkit::CreateTabBody()
{
	// 프로퍼티 에디터 모듈 로드
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	// DetailsView 생성 시 필요한 인수 설정
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bHideSelectionTip = true;

	// DetailsView 위젯 생성
	TSharedRef<IDetailsView> DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);

	// DetailsView가 표시할 에셋 객체를 설정
	DetailsView->SetObject(GetMapPreset());

	// Slate 위젯을 생성하여 반환
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			DetailsView
		];
}

void FMapPresetEditorToolkit::FillToolbar(FToolBarBuilder& ToolbarBuilder)
{
	// 버튼 파란색
	FButtonStyle* GenerateButtonStyle = new FButtonStyle(FAppStyle::Get().GetWidgetStyle<FButtonStyle>("Button"));
	//GenerateButtonStyle->SetNormal(FSlateColorBrush(FLinearColor(0.05f, 0.2f, 0.8f)));

	// 툴바 박스 추가
	TSharedRef<SHorizontalBox> CustomToolbarBox = SNew(SHorizontalBox);

	// 툴바 박스에 스페이서 추가
	CustomToolbarBox->AddSlot()
	.FillWidth(1.0f)
	[
		SNew(SSpacer)
	];
	
	// 커스텀 버튼 위젯 생성
	CustomToolbarBox->AddSlot()
	.AutoWidth()
	.HAlign(HAlign_Right)
	.Padding(2.0f)
	[
		SNew(SButton)
		.ButtonStyle(GenerateButtonStyle)
		.HAlign(HAlign_Center)
		.OnClicked(FOnClicked::CreateSP(this, &FMapPresetEditorToolkit::OnGenerateClicked))
		.ToolTipText(FMapPresetEditorCommands::Get().GenerateAction->GetDescription())
		[
			SNew(STextBlock)
			.Text(FMapPresetEditorCommands::Get().GenerateAction->GetLabel())
			.Justification(ETextJustify::Center)
		]
	];

	ToolbarBuilder.AddWidget(CustomToolbarBox);
}

FReply FMapPresetEditorToolkit::OnGenerateClicked()
{
	// !TODO : EditorViewportClient의 액터 찾아서 호출
	OnGenerateButtonClicked.Broadcast();

	return FReply::Handled();
}
