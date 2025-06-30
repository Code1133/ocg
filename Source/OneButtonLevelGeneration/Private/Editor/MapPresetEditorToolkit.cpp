#include "Editor/MapPresetEditorToolkit.h"

#include "OCGLevelGenerator.h"
#include "Data/MapPreset.h"
#include "Editor/MapPresetApplicationMode.h"
#include "Editor/MapPresetEditorCommands.h"
#include "Editor/SMapPresetViewport.h"
#include "Editor/MapPresetViewportClient.h"
#include "Editor/SMapPresetEnvironmentLightingViewer.h"
#include "MaterialEditor/MaterialEditorInstanceConstant.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Framework/Docking/TabManager.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

class ADirectionalLight;

const FName GMapPresetEditor_ViewportTabId(TEXT("MapPresetEditor_Viewport"));
const FName GMapPresetEditor_DetailsTabId(TEXT("MapPresetEditor_Details"));
const FName GMapPresetEditor_MaterialDetailsTabId(TEXT("MapPresetEditor_MaterialDetails"));
const FName GMapPresetEditor_EnvLightMixerTabId(TEXT("MapPresetEditor_EnvLightMixer"));


void FMapPresetEditorToolkit::InitEditor(const EToolkitMode::Type Mode,
                                         const TSharedPtr<class IToolkitHost>& InitToolkitHost, UMapPreset* MapPreset)
{
	// 커맨드 리스트 및 액션 바인드
	FMapPresetEditorCommands::Register();

	// 툴바 확장 기능 생성
	TSharedPtr<FExtender> ToolbarExtender = MakeShared<FExtender>();
	ToolbarExtender->AddToolBarExtension(
			"Asset",
			EExtensionHook::After,
			ToolkitCommands,
			FToolBarExtensionDelegate::CreateSP(this, &FMapPresetEditorToolkit::FillToolbar)
		);

	// 맵 프리셋
	EditingPreset = MapPreset;
	EditingPreset->EditorToolkit = SharedThis(this);

	MapPresetEditorWorld = UWorld::CreateWorld(
		EWorldType::Editor,
		true,
		TEXT("MapPresetEditorWorld"),
		GetTransientPackage(),
		true
		);

	if (MapPresetEditorWorld)
	{
		FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Editor);
		Context.SetCurrentWorld(MapPresetEditorWorld.Get());
		// 월드 바운드 체크 비활성화
		MapPresetEditorWorld->GetWorldSettings()->bEnableWorldBoundsChecks = false;

		EditingPreset->OwnerWorld = MapPresetEditorWorld.Get();
	}

	
	// 뷰포트 위젯 생성
	ViewportWidget = SNew(SMapPresetViewport)
		.MapPresetEditorToolkit(SharedThis(this))
		.World(MapPresetEditorWorld.Get()); // .World 인자로 EditorWorld 전달

	// 환경 라이팅 믹서 생성
	EnvironmentLightingViewer = SNew(SMapPresetEnvironmentLightingViewer)
		.World(MapPresetEditorWorld.Get());
	
	// 에디터 모드 추가
	AddApplicationMode(
		TEXT("DefaultMode"),
		MakeShared<FMapPresetApplicationMode>(SharedThis(this))
	);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	InitAssetEditor(
		Mode,
		InitToolkitHost,
		FName("MapPresetEditor"),
		FTabManager::FLayout::NullLayout,
		bCreateDefaultStandaloneMenu,
		bCreateDefaultToolbar,
		EditingPreset.Get()
		);

	// 확장 기능을 에디터에 추가
	AddToolbarExtender(ToolbarExtender);
	// 메뉴 및 툴바를 다시 생성하여 변경사항 적용
	//RegenerateMenusAndToolbars();
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
	OnGenerateButtonClicked.Clear();
	OnExportToLevelButtonClicked.Clear();
	
	if (EditingPreset.Get())
	{
		EditingPreset->EditorToolkit = nullptr;
		EditingPreset = nullptr;
	}
	
	if (MapPresetEditorWorld.Get())
	{
		GEngine->DestroyWorldContext(MapPresetEditorWorld.Get());
		for (AActor* Actor : MapPresetEditorWorld->GetCurrentLevel()->Actors)
		{
			if (Actor && Actor->IsA<AOCGLevelGenerator>())
			{
				AOCGLevelGenerator* LevelGenerator = Cast<AOCGLevelGenerator>(Actor);
				LevelGenerator->SetMapPreset(nullptr);
				MapPresetEditorWorld->DestroyActor(Actor);
			}
		}
		
		MapPresetEditorWorld->DestroyWorld(true);
		MapPresetEditorWorld->MarkAsGarbage();
		MapPresetEditorWorld->SetFlags(RF_Transient);
		MapPresetEditorWorld->Rename(nullptr, GetTransientPackage(), REN_NonTransactional | REN_DontCreateRedirectors);

		MapPresetEditorWorld = nullptr;

		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS, true);
	}
}

void FMapPresetEditorToolkit::RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	// 메뉴 그룹 생성
	const IWorkspaceMenuStructure& MenuStructure = WorkspaceMenu::GetMenuStructure();
	TSharedRef<FWorkspaceItem> MenuRoot = MenuStructure.GetLevelEditorCategory()->AddGroup(
		FName(TEXT("MapPresetEditor")),
		FText::FromString(TEXT("Map Preset Editor"))
	);

	// 각 탭을 TabManager에 직접 등록합니다.
	InTabManager->RegisterTabSpawner(GMapPresetEditor_ViewportTabId, FOnSpawnTab::CreateSP(this, &FMapPresetEditorToolkit::SpawnTab_Viewport))
		.SetDisplayName(FText::FromString(TEXT("Viewport"))).SetGroup(MenuRoot);

	InTabManager->RegisterTabSpawner(GMapPresetEditor_DetailsTabId, FOnSpawnTab::CreateSP(this, &FMapPresetEditorToolkit::SpawnTab_Details))
		.SetDisplayName(FText::FromString(TEXT("Details"))).SetGroup(MenuRoot);

	InTabManager->RegisterTabSpawner(GMapPresetEditor_EnvLightMixerTabId, FOnSpawnTab::CreateSP(this, &FMapPresetEditorToolkit::SpawnTab_EnvLightMixerTab))
		.SetDisplayName(FText::FromString(TEXT("Environment Light Mixer"))).SetGroup(MenuRoot);
}

void FMapPresetEditorToolkit::UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	FWorkflowCentricApplication::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterAllTabSpawners();
}

TSharedRef<SDockTab> FMapPresetEditorToolkit::SpawnTab_Viewport(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == GMapPresetEditor_ViewportTabId);

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
	check(Args.GetTabId() == GMapPresetEditor_DetailsTabId);

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

TSharedRef<SDockTab> FMapPresetEditorToolkit::SpawnTab_EnvLightMixerTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
	.Label(FText::FromString(TEXT("Env.Light Mixer")))
	[
		// 생성해 둔 환경 라이팅 믹서 위젯을 탭에 배치
		EnvironmentLightingViewer.ToSharedRef()
	];
}

void FMapPresetEditorToolkit::FillToolbar(FToolBarBuilder& ToolbarBuilder)
{
	// 버튼 스타일
	FButtonStyle* GenerateButtonStyle = new FButtonStyle(FAppStyle::Get().GetWidgetStyle<FButtonStyle>("Button"));

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

	CustomToolbarBox->AddSlot()
	.AutoWidth()
	.HAlign(HAlign_Right)
	.Padding(2.0f)
	[
		SNew(SButton)
		.ButtonStyle(GenerateButtonStyle)
		.HAlign(HAlign_Center)
		.OnClicked(FOnClicked::CreateSP(this, &FMapPresetEditorToolkit::OnExportToLevelClicked))
		.ToolTipText(FMapPresetEditorCommands::Get().ExportToLevelAction->GetDescription())
		[
			SNew(STextBlock)
			.Text(FMapPresetEditorCommands::Get().ExportToLevelAction->GetLabel())
			.Justification(ETextJustify::Center)
		]
	];

	ToolbarBuilder.AddWidget(CustomToolbarBox);
}

FReply FMapPresetEditorToolkit::OnGenerateClicked()
{
	if (!EditingPreset.IsValid() || EditingPreset->Biomes.IsEmpty())
	{
		// 에러 메시지 
		const FText DialogTitle = FText::FromString(TEXT("Error"));
		const FText DialogText = FText::FromString(TEXT("At Least one biome must be defined in the preset before generating the level."));

		// FMessageDialog::Open을 호출하여 다이얼로그를 엽니다.
		FMessageDialog::Open(EAppMsgType::Ok, DialogText, DialogTitle);
        
		return FReply::Handled();
	}
	for (const auto& Biome : EditingPreset->Biomes)
	{
		if (Biome.BiomeName == NAME_None)
		{
			const FText DialogTitle = FText::FromString(TEXT("Error"));
			const FText DialogText = FText::FromString(TEXT("Biome name cannot be empty. Please set a valid name for each biome."));

			FMessageDialog::Open(EAppMsgType::Ok, DialogText, DialogTitle);
			return FReply::Handled();
		}
	}
	OnGenerateButtonClicked.Broadcast();

	return FReply::Handled();
}

FReply FMapPresetEditorToolkit::OnExportToLevelClicked()
{
	OnExportToLevelButtonClicked.Broadcast();
	
	return FReply::Handled();
}

