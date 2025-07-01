#include "Editor/MapPresetEditorToolkit.h"

#include "FileHelpers.h"
#include "OCGLevelGenerator.h"
#include "Data/MapPreset.h"
#include "Editor/MapPresetApplicationMode.h"
#include "Editor/MapPresetEditorCommands.h"
#include "Editor/SMapPresetViewport.h"
#include "Editor/SMapPresetEnvironmentLightingViewer.h"
#include "Framework/Docking/TabManager.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ModelComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyLight.h"

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

		// 월드에 레벨 제너레이터 액터 추가
		LevelGenerator = MapPresetEditorWorld->SpawnActor<AOCGLevelGenerator>();
		LevelGenerator->SetMapPreset(EditingPreset.Get());
		SetupDefaultActors();
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
	if (EditingPreset.Get())
	{
		EditingPreset->EditorToolkit = nullptr;
		EditingPreset = nullptr;
	}

	LevelGenerator = nullptr;
	if (MapPresetEditorWorld.Get())
	{
		GEngine->DestroyWorldContext(MapPresetEditorWorld.Get());
		for (AActor* Actor : MapPresetEditorWorld->GetCurrentLevel()->Actors)
		{
			if (Actor && Actor->IsA<AOCGLevelGenerator>())
			{
				MapPresetEditorWorld->DestroyActor(Actor);
			}
		}
		
		MapPresetEditorWorld->DestroyWorld(true);
		MapPresetEditorWorld->MarkAsGarbage();
		MapPresetEditorWorld->SetFlags(RF_Transient);
		MapPresetEditorWorld->Rename(nullptr, GetTransientPackage(), REN_NonTransactional | REN_DontCreateRedirectors);

		MapPresetEditorWorld = nullptr;
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

	Generate();
	
	return FReply::Handled();
}

FReply FMapPresetEditorToolkit::OnExportToLevelClicked()
{
	ExportPreviewSceneToLevel();
	
	return FReply::Handled();
}

void FMapPresetEditorToolkit::SetupDefaultActors()
{
	if (!MapPresetEditorWorld)
		return;

	const FTransform Transform(FVector(0.0f, 0.0f, 0.0f));
	ASkyLight* SkyLight = Cast<ASkyLight>(GEditor->AddActor(MapPresetEditorWorld->GetCurrentLevel(), ASkyLight::StaticClass(), Transform));
	SkyLight->GetLightComponent()->SetMobility(EComponentMobility::Movable);
	SkyLight->GetLightComponent()->SetRealTimeCaptureEnabled(true);

	ADirectionalLight* DirectionalLight = Cast<ADirectionalLight>(GEditor->AddActor(MapPresetEditorWorld->GetCurrentLevel(), ADirectionalLight::StaticClass(), Transform));
	DirectionalLight->GetComponent()->bAtmosphereSunLight = 1;
	DirectionalLight->GetComponent()->AtmosphereSunLightIndex = 0;
	DirectionalLight->MarkComponentsRenderStateDirty();

	GEditor->AddActor(MapPresetEditorWorld->GetCurrentLevel(), ASkyAtmosphere::StaticClass(), Transform);
	GEditor->AddActor(MapPresetEditorWorld->GetCurrentLevel(), AVolumetricCloud::StaticClass(), Transform);
	GEditor->AddActor(MapPresetEditorWorld->GetCurrentLevel(), AExponentialHeightFog::StaticClass(), Transform);
}

void FMapPresetEditorToolkit::ExportPreviewSceneToLevel()
{
	if (!MapPresetEditorWorld)
	{
		UE_LOG(LogTemp, Warning, TEXT("Toolkit or its EditorWorld is null, cannot export."));
		return;
	}
	
	UWorld* SourceWorld = MapPresetEditorWorld;
	
	// 월드 복제
	UPackage* DestWorldPackage = CreatePackage(TEXT("/Temp/MapPresetEditor/World"));
	FObjectDuplicationParameters Parameters(SourceWorld, DestWorldPackage);
	Parameters.DestName = SourceWorld->GetFName();
	Parameters.DestClass = SourceWorld->GetClass();
	Parameters.DuplicateMode = EDuplicateMode::World;
	Parameters.PortFlags = PPF_Duplicate;

	UWorld* DuplicatedWorld = CastChecked<UWorld>(StaticDuplicateObjectEx(Parameters));
	DuplicatedWorld->SetFeatureLevel(SourceWorld->GetFeatureLevel());

	ULevel* SourceLevel = SourceWorld->PersistentLevel;
	ULevel* DuplicatedLevel = DuplicatedWorld->PersistentLevel;

	// AOCGLevelGenerator 액터를 찾아서 제거합니다.
	for (AActor* Actor : DuplicatedLevel->Actors)
	{
		if (Actor && Actor->IsA<AOCGLevelGenerator>())
		{
			DuplicatedWorld->DestroyActor(Actor);
		}
	}

	if (DuplicatedLevel->Model != NULL
		&& DuplicatedLevel->Model == SourceLevel->Model
		&& DuplicatedLevel->ModelComponents.Num() == SourceLevel->ModelComponents.Num())
	{
		DuplicatedLevel->Model->ClearLocalMaterialIndexBuffersData();
		for (int32 ComponentIndex = 0; ComponentIndex < DuplicatedLevel->ModelComponents.Num(); ++ComponentIndex)
		{
			UModelComponent* SrcComponent = SourceLevel->ModelComponents[ComponentIndex];
			UModelComponent* DstComponent = DuplicatedLevel->ModelComponents[ComponentIndex];
			DstComponent->CopyElementsFrom(SrcComponent);
		}
	}
	
	bool bSuccess = FEditorFileUtils::SaveLevelAs(DuplicatedWorld->GetCurrentLevel());
	if (!bSuccess)
	{
		DestWorldPackage->ClearFlags(RF_Standalone);
		DestWorldPackage->MarkAsGarbage();
	
		GEngine->DestroyWorldContext(DuplicatedWorld);
		DuplicatedWorld->DestroyWorld(true);
		DuplicatedWorld->MarkAsGarbage();
		DuplicatedWorld->SetFlags(RF_Transient);
		DuplicatedWorld->Rename(nullptr, GetTransientPackage(), REN_NonTransactional | REN_DontCreateRedirectors);

		CollectGarbage(RF_NoFlags);
	}
}

void FMapPresetEditorToolkit::Generate() const
{
	if (LevelGenerator.IsValid() && MapPresetEditorWorld)
	{
		LevelGenerator->OnClickGenerate(MapPresetEditorWorld);
	}
}

