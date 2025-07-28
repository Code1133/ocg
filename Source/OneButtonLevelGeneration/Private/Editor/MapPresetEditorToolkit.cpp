// Copyright (c) 2025 Code1133. All rights reserved.

#include "Editor/MapPresetEditorToolkit.h"

#include "FileHelpers.h"
#include "Landscape.h"
#include "OCGLevelGenerator.h"
#include "OCGLog.h"
#include "WaterBodyActor.h"
#include "Data/MapPreset.h"
#include "Editor/MapPresetApplicationMode.h"
#include "Editor/MapPresetEditorCommands.h"
#include "Editor/SMapPresetViewport.h"
#include "Editor/SMapPresetEnvironmentLightingViewer.h"
#include "Framework/Docking/TabManager.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Component/OCGMapGenerateComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ModelComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Editor/MapPresetAssetTypeActions.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyLight.h"
#include "Factories/WorldFactory.h"
#include "Utils/OCGLandscapeUtil.h"
#include "WorldPartition/DataLayer/DataLayerType.h"

class ADirectionalLight;

void FMapPresetEditorToolkit::InitEditor(const EToolkitMode::Type Mode,
                                         const TSharedPtr<IToolkitHost>& InitToolkitHost, UMapPreset* MapPreset)
{
	// Bind command lists and actions
	FMapPresetEditorCommands::Register();

	// create ToolbarExtender
	const TSharedPtr<FExtender> ToolbarExtender = MakeShared<FExtender>();
	ToolbarExtender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		nullptr,
		FToolBarExtensionDelegate::CreateSP(this, &FMapPresetEditorToolkit::FillToolbar)
	);

	EditingPreset = MapPreset;
	
	MapPresetEditorWorld = CreateEditorWorld();

	if (MapPresetEditorWorld)
	{
		FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Editor);
		Context.SetCurrentWorld(MapPresetEditorWorld.Get());
		
		MapPresetEditorWorld->GetWorldSettings()->bEnableWorldBoundsChecks = false;

		// Add LevelGenerator
		LevelGenerator = MapPresetEditorWorld->SpawnActor<AOCGLevelGenerator>();
		LevelGenerator->SetMapPreset(EditingPreset.Get());
		SetupDefaultActors();
	}

	
	// Create Viewport Widget
	ViewportWidget = SNew(SMapPresetViewport)
		.MapPresetEditorToolkit(SharedThis(this))
		.World(MapPresetEditorWorld.Get());
	
	// Create Environment Lighting Viewer
	EnvironmentLightingViewer = SNew(SMapPresetEnvironmentLightingViewer)
		.World(MapPresetEditorWorld.Get());
	
	// Add Application Mode
	AddApplicationMode(
		TEXT("DefaultMode"),
		MakeShared<FMapPresetApplicationMode>(SharedThis(this))
	);

	constexpr bool bCreateDefaultStandaloneMenu = true;
	constexpr bool bCreateDefaultToolbar = true;
	InitAssetEditor(
		Mode,
		InitToolkitHost,
		FName("MapPresetEditor"),
		FTabManager::FLayout::NullLayout,
		bCreateDefaultStandaloneMenu,
		bCreateDefaultToolbar,
		EditingPreset.Get()
		);

	AddToolbarExtender(ToolbarExtender);
	
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
	if (EditingPreset.Get())
	{
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

	FMapPresetAssetTypeActions::OpenedEditorInstance = nullptr;
}

void FMapPresetEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	// Create Menu Group
	const IWorkspaceMenuStructure& MenuStructure = WorkspaceMenu::GetMenuStructure();
	const TSharedRef<FWorkspaceItem> MenuRoot = MenuStructure.GetLevelEditorCategory()->AddGroup(
		FName(TEXT("MapPresetEditor")),
		FText::FromString(TEXT("Map Preset Editor"))
	);

	// Add Tabs
	InTabManager->RegisterTabSpawner(
		FMapPresetEditorConstants::ViewportTabId,
		FOnSpawnTab::CreateSP(this, &FMapPresetEditorToolkit::SpawnTab_Viewport)
	)
	.SetDisplayName(FText::FromString(TEXT("Viewport"))).SetGroup(MenuRoot);

	InTabManager->RegisterTabSpawner(
		FMapPresetEditorConstants::DetailsTabId,
		FOnSpawnTab::CreateSP(this, &FMapPresetEditorToolkit::SpawnTab_Details)
	)
	.SetDisplayName(FText::FromString(TEXT("Details"))).SetGroup(MenuRoot);

	InTabManager->RegisterTabSpawner(
		FMapPresetEditorConstants::EnvLightMixerTabId,
		FOnSpawnTab::CreateSP(this, &FMapPresetEditorToolkit::SpawnTab_EnvLightMixerTab)
	)
	.SetDisplayName(FText::FromString(TEXT("Environment Light Mixer"))).SetGroup(MenuRoot);

	InTabManager->RegisterTabSpawner(
		FMapPresetEditorConstants::LandscapeDetailsTabId,
		FOnSpawnTab::CreateSP(this, &FMapPresetEditorToolkit::SpawnTab_LandscapeTab)
	)
	.SetDisplayName(FText::FromString(TEXT("Landscape Details"))).SetGroup(MenuRoot);
}

void FMapPresetEditorToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FWorkflowCentricApplication::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterAllTabSpawners();
}

TSharedRef<SDockTab> FMapPresetEditorToolkit::SpawnTab_Viewport(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == FMapPresetEditorConstants::ViewportTabId);

	return SNew(SDockTab)
		.Label(FText::FromString(TEXT("Viewport")))
		[
			ViewportWidget.ToSharedRef()
		];
}

TSharedRef<SDockTab> FMapPresetEditorToolkit::SpawnTab_Details(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == FMapPresetEditorConstants::DetailsTabId);

	return SNew(SDockTab)
		.Label(FText::FromString(TEXT("Details")))
		[
			CreateMapPresetTabBody()
		];
}

TSharedRef<SWidget> FMapPresetEditorToolkit::CreateMapPresetTabBody()
{
	// Load PropertyEditorModule
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	// Set Args
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bHideSelectionTip = true;

	const TSharedRef<IDetailsView> DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);

	DetailsView->SetObject(GetMapPreset());

	// Create and return the vertical box containing the DetailsView
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			DetailsView
		];
}

TSharedRef<SWidget> FMapPresetEditorToolkit::CreateLandscapeTabBody()
{
	// Load PropertyEditorModule
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	// Set Args
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bHideSelectionTip = true;

	LandscapeDetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	
	ALandscape* SpawnedLandscape = LevelGenerator.IsValid()
		? LevelGenerator->GetLandscape()
		: nullptr;
	
	LandscapeDetailsView->SetObject(SpawnedLandscape);

	// Create and return the vertical box containing the DetailsView
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			LandscapeDetailsView.ToSharedRef()
		];
}

TSharedRef<SDockTab> FMapPresetEditorToolkit::SpawnTab_EnvLightMixerTab([[maybe_unused]] const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
	.Label(FText::FromString(TEXT("Env.Light Mixer")))
	[
		EnvironmentLightingViewer.ToSharedRef()
	];
}

TSharedRef<SDockTab> FMapPresetEditorToolkit::SpawnTab_LandscapeTab([[maybe_unused]] const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(FText::FromString(TEXT("Landscape Details")))
		[
			CreateLandscapeTabBody()
		];
}

void FMapPresetEditorToolkit::FillToolbar(FToolBarBuilder& ToolbarBuilder)
{
	const FButtonStyle* GenerateButtonStyle = new FButtonStyle(FAppStyle::Get().GetWidgetStyle<FButtonStyle>("Button"));
	const TSharedRef<SHorizontalBox> CustomToolbarBox = SNew(SHorizontalBox);

	// Add Spacer 
	CustomToolbarBox->AddSlot()
	.FillWidth(1.0f)
	[
		SNew(SSpacer)
	];

	CustomToolbarBox->AddSlot()
	.AutoWidth()
	.HAlign(HAlign_Right)
	.Padding(2.0f)
	[
		SNew(SButton)
		.ButtonStyle(GenerateButtonStyle)
		.HAlign(HAlign_Center)
		.OnClicked(FOnClicked::CreateSP(this, &FMapPresetEditorToolkit::OnPreviewMapClicked))
		.ToolTipText(FMapPresetEditorCommands::Get().PreviewMapAction->GetDescription())
		[
			SNew(STextBlock)
			.Text(FMapPresetEditorCommands::Get().PreviewMapAction->GetLabel())
			.Justification(ETextJustify::Center)
		]
	];
	
	// Create buttons for Generate and Export to Level actions
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

	CustomToolbarBox->AddSlot()
		.AutoWidth()
		.HAlign(HAlign_Right)
		.Padding(2.0f)
		[
			SNew(SButton)
			.ButtonStyle(GenerateButtonStyle)
			.HAlign(HAlign_Center)
			.OnClicked(FOnClicked::CreateSP(this, &FMapPresetEditorToolkit::OnForceGeneratePCGClicked))
			.ToolTipText(FMapPresetEditorCommands::Get().ForceGeneratePCGAction->GetDescription())
			[
				SNew(STextBlock)
				.Text(FMapPresetEditorCommands::Get().ForceGeneratePCGAction->GetLabel())
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
		.OnClicked(FOnClicked::CreateSP(this, &FMapPresetEditorToolkit::OnRegenerateRiverClicked))
		.ToolTipText(FMapPresetEditorCommands::Get().RegenerateRiverAction->GetDescription())
		[
			SNew(STextBlock)
			.Text(FMapPresetEditorCommands::Get().RegenerateRiverAction->GetLabel())
			.Justification(ETextJustify::Center)
		]
	];

	ToolbarBuilder.AddWidget(CustomToolbarBox);
}

FReply FMapPresetEditorToolkit::OnPreviewMapClicked()
{
	if (LevelGenerator.IsValid())
	{
		LevelGenerator->PreviewMaps();
	}
	
	return FReply::Handled();
}

FReply FMapPresetEditorToolkit::OnGenerateClicked()
{
	if (!EditingPreset.IsValid() || EditingPreset->Biomes.IsEmpty())
	{
		// Error message
		const FText DialogTitle = FText::FromString(TEXT("Error"));
		const FText DialogText = FText::FromString(TEXT("At Least one biome must be defined in the preset before generating the level."));

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

FReply FMapPresetEditorToolkit::OnRegenerateRiverClicked()
{
	if (LevelGenerator.IsValid())
	{
		if (LevelGenerator->GetLandscape())
			OCGLandscapeUtil::RegenerateRiver(MapPresetEditorWorld, LevelGenerator.Get());
	}
	return FReply::Handled();
}

FReply FMapPresetEditorToolkit::OnForceGeneratePCGClicked()
{
	if (LevelGenerator.IsValid())
	{    
		if (LevelGenerator->GetLandscape())
			OCGLandscapeUtil::ForceGeneratePCG(MapPresetEditorWorld);
	}
	return FReply::Handled();
}

UWorld* FMapPresetEditorToolkit::CreateEditorWorld()
{
	UWorldFactory* Factory = NewObject<UWorldFactory>();
	Factory->WorldType = EWorldType::Editor;
	Factory->bCreateWorldPartition = false;
	Factory->bInformEngineOfWorld = true;
	Factory->FeatureLevel = GEditor->DefaultWorldFeatureLevel;
	UPackage* Pkg = GetTransientPackage();
	constexpr EObjectFlags Flags = RF_Public | RF_Standalone;

	UWorld* NewWorld = CastChecked<UWorld>(Factory->FactoryCreateNew(UWorld::StaticClass(), Pkg, TEXT("MapPresetTransientWorld"), Flags, nullptr, GWarn));
	NewWorld->AddToRoot();
	NewWorld->UpdateWorldComponents(true, true);

	return NewWorld;
}

void FMapPresetEditorToolkit::SetupDefaultActors()
{
	if (!MapPresetEditorWorld)
	{
		return;
	}

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
		UE_LOG(LogOCGModule, Warning, TEXT("Toolkit or its EditorWorld is null, cannot export."));
		return;
	}
	
	UWorld* SourceWorld = MapPresetEditorWorld;
	
	// Duplicate World
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
	
	TArray<AActor*> ActorsToDestroy;
	
	for (AActor* Actor : DuplicatedLevel->Actors)
	{
		if (Actor && Actor->IsA<AWaterBody>())
		{
			AWaterBody* WaterBodyActor = Cast<AWaterBody>(Actor);
			WaterBodyActor->GetWaterBodyComponent()->UpdateWaterZones();
		}
	}

	for (AActor* Actor : ActorsToDestroy)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}

	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS, true);

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
	
	// Enable World Bounds Checks
	DuplicatedWorld->GetWorldSettings()->bEnableWorldBoundsChecks = true;
	
	bool bSuccess = FEditorFileUtils::SaveLevelAs(DuplicatedWorld->GetCurrentLevel());
	if (bSuccess)
	{
		// Show success message
		FText DialogTitle = FText::FromString(TEXT("Export Success"));
		FText DialogText = FText::FromString(TEXT("The level has been successfully exported."));
		FMessageDialog::Open(EAppMsgType::Ok, DialogText, DialogTitle);
	}
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

		// Show error message
		FText DialogTitle = FText::FromString(TEXT("Export Failed"));
		FText DialogText = FText::FromString(TEXT("Failed to export the level. Please check the log for details."));
		FMessageDialog::Open(EAppMsgType::Ok, DialogText, DialogTitle);
	}
}

void FMapPresetEditorToolkit::Generate() const
{
	if (LevelGenerator.IsValid() && MapPresetEditorWorld)
	{
		LevelGenerator->OnClickGenerate(MapPresetEditorWorld);
		if (LandscapeDetailsView)
		{
			LandscapeDetailsView->SetObject(LevelGenerator->GetLandscape());
			LandscapeDetailsView->ForceRefresh();
		}
	}
}

