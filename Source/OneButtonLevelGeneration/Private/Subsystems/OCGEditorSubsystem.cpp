// Copyright (c) 2025-2026 Code1133. All rights reserved.
#include "Subsystems/OCGEditorSubsystem.h"

#include "OCGDeveloperSettings.h"
#include "OCGLog.h"
#include "Data/MapPreset.h"
#include "Data/OCGBiomeSettings.h"
#include "PCG/OCGLandscapeVolume.h"
#include "Subsystems/OCGDataGenerationSubsystem.h"
#include "Subsystems/OCGHydrologySubsystem.h"
#include "Subsystems/OCGLandscapeGenSubsystem.h"
#include "Subsystems/OCGPopulationSubsystem.h"
#include "UI/SOCGWindow.h"
#include "Utils/OCGUtils.h"

#include "Editor.h"
#include "Framework/Docking/TabManager.h"
#include "PCGComponent.h"
#include "PCGGraph.h"
#include "ToolMenus.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/MessageDialog.h"
#include "Styling/AppStyle.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "OCGEditorSubsystem"

static const TCHAR* OCGConfigSection = TEXT("OCG");
static const TCHAR* OCGLastPresetKey = TEXT("LastUsedPreset");

static const FName OCGToolbarMenuName    = TEXT("LevelEditor.LevelEditorToolBar.AssetsToolBar");
static const FName OCGToolbarSectionName = TEXT("OCG");
static const FName OCGMenuGroupName      = TEXT("OCGTools");
static const FName OCGWindowTabName      = TEXT("OCGWindow");

void UOCGEditorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// MapPreset 프로퍼티 변경 구독
	// DataAsset이 직접 월드 액터를 조작하지 않도록 에디터 레이어가 대신 처리합니다.
	UMapPreset::OnPropertyChanged.AddUObject(this, &UOCGEditorSubsystem::OnMapPresetPropertyChanged);

	// OCG Window 탭 등록
	MenuGroup = WorkspaceMenu::GetMenuStructure().GetLevelEditorCategory()->AddGroup(
		OCGMenuGroupName,
		LOCTEXT("OCGMenuGroup", "OCG Tools"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "LandscapeEditor.NewLandscape")
	);

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		OCGWindowTabName,
		FOnSpawnTab::CreateUObject(this, &UOCGEditorSubsystem::SpawnOCGWindowTab)
	)
	.SetDisplayName(LOCTEXT("OCGWindowTitle", "OCG"))
	.SetTooltipText(LOCTEXT("OCGWindowTooltip", "Open the OCG Level Generator window"))
	.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LandscapeEditor.NewLandscape"))
	.SetGroup(MenuGroup.ToSharedRef());

	RestoreLastUsedPreset();
	RegisterToolbarEntry();
	ScheduleSettingsValidation();
}

void UOCGEditorSubsystem::ScheduleSettingsValidation()
{
	IAssetRegistry& AssetRegistry =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

	// 에셋이 로딩 중이라면, 완료되고 나서 검증
	if (AssetRegistry.IsLoadingAssets())
	{
		OnFilesLoadedHandle = AssetRegistry.OnFilesLoaded().AddWeakLambda(this, []
		{
			GetDefault<UOCGDeveloperSettings>()->ValidateConfiguredAssets();
		});
	}
	else
	{
		GetDefault<UOCGDeveloperSettings>()->ValidateConfiguredAssets();
	}
}

void UOCGEditorSubsystem::Deinitialize()
{
	UMapPreset::OnPropertyChanged.RemoveAll(this);

	if (OnFilesLoadedHandle.IsValid())
	{
		if (const FAssetRegistryModule* Module = FModuleManager::GetModulePtr<FAssetRegistryModule>(TEXT("AssetRegistry")))
		{
			Module->Get().OnFilesLoaded().Remove(OnFilesLoadedHandle);
		}
		OnFilesLoadedHandle.Reset();
	}

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(OCGWindowTabName);

	// AddGroup은 같은 이름이어도 항목을 새로 만들기 때문에 직접 제거해야 중복이 쌓이지 않음
	if (MenuGroup.IsValid())
	{
		WorkspaceMenu::GetMenuStructure().GetLevelEditorCategory()->RemoveItem(MenuGroup.ToSharedRef());
		MenuGroup.Reset();
	}

	UnregisterToolbarEntry();

	Super::Deinitialize();
}

void UOCGEditorSubsystem::ExecuteGeneration(const UMapPreset* Preset)
{
	if (!ValidatePreset(Preset))
	{
		return;
	}

	UOCGDataGenerationSubsystem* DataSub = GEditor->GetEditorSubsystem<UOCGDataGenerationSubsystem>();
	UOCGLandscapeGenSubsystem* LandscapeSub = GEditor->GetEditorSubsystem<UOCGLandscapeGenSubsystem>();
	UOCGPopulationSubsystem* PopulationSub = GEditor->GetEditorSubsystem<UOCGPopulationSubsystem>();
	UOCGHydrologySubsystem* HydrologySub = GEditor->GetEditorSubsystem<UOCGHydrologySubsystem>();

	if (!DataSub || !LandscapeSub || !PopulationSub || !HydrologySub)
	{
		UE_LOG(LogOCGModule, Error, TEXT("ExecuteGeneration: One or more OCG subsystems are unavailable."));
		return;
	}

	UE_LOG(LogOCGModule, Log, TEXT("ExecuteGeneration: Starting pipeline with preset '%s'."), *Preset->GetName());

	DataSub->GenerateData(Preset);
	LandscapeSub->ApplyLandscape(Preset, DataSub->GetDataContainer());
	PopulationSub->ApplyPopulation(Preset);
	HydrologySub->ApplyHydrology(Preset, DataSub->GetDataContainer());

	PersistLastUsedPreset(Preset);
	LastUsedPresetAsset = TSoftObjectPtr<UMapPreset>(const_cast<UMapPreset*>(Preset));

	UE_LOG(LogOCGModule, Log, TEXT("ExecuteGeneration: Pipeline complete."));
}

void UOCGEditorSubsystem::OpenOCGWindow()
{
	FGlobalTabmanager::Get()->TryInvokeTab(FTabId(OCGWindowTabName));
}

void UOCGEditorSubsystem::RegenerateRiverOnly(const UMapPreset* Preset)
{
	if (!ValidatePreset(Preset))
	{
		return;
	}

	UOCGDataGenerationSubsystem* DataSub = GEditor->GetEditorSubsystem<UOCGDataGenerationSubsystem>();
	UOCGHydrologySubsystem* HydrologySub = GEditor->GetEditorSubsystem<UOCGHydrologySubsystem>();

	if (!DataSub || !HydrologySub)
	{
		UE_LOG(LogOCGModule, Error, TEXT("RegenerateRiverOnly: Required subsystems unavailable."));
		return;
	}

	UE_LOG(LogOCGModule, Log, TEXT("RegenerateRiverOnly: Running Hydrology step with preset '%s'."), *Preset->GetName());

	HydrologySub->ApplyHydrology(Preset, DataSub->GetDataContainer());

	PersistLastUsedPreset(Preset);
	LastUsedPresetAsset = TSoftObjectPtr<UMapPreset>(const_cast<UMapPreset*>(Preset));

	UE_LOG(LogOCGModule, Log, TEXT("RegenerateRiverOnly: Complete."));
}

void UOCGEditorSubsystem::ForcePCGRegenerate()
{
	if (!GEditor)
	{
		return;
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		UE_LOG(LogOCGModule, Warning, TEXT("ForcePCGRegenerate: No editor world available."));
		return;
	}

	TArray<AOCGLandscapeVolume*> Volumes = FOCGUtils::GetAllActorsOfClass<AOCGLandscapeVolume>(World);
	int32 TriggeredCount = 0;

	for (AOCGLandscapeVolume* Volume : Volumes)
	{
		if (!Volume)
		{
			continue;
		}
		if (UPCGComponent* PCGComp = Volume->GetPCGComponent())
		{
			PCGComp->GenerateLocal(/*bForce=*/ true);
			++TriggeredCount;
		}
	}

	UE_LOG(LogOCGModule, Log, TEXT("ForcePCGRegenerate: Triggered PCG on %d volume(s)."), TriggeredCount);
}

TSharedRef<SDockTab> UOCGEditorSubsystem::SpawnOCGWindowTab(const FSpawnTabArgs& /*Args*/)
{
	TSharedRef<SOCGWindow> WindowWidget = SNew(SOCGWindow);

	// 마지막으로 사용된 Preset이 유효하면 즉시 표시
	if (UMapPreset* LastPreset = LastUsedPresetAsset.IsValid()
		? LastUsedPresetAsset.Get()
		: LastUsedPresetAsset.LoadSynchronous())
	{
		WindowWidget->SetPreset(LastPreset);
	}

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("OCGWindowTitle", "OCG"))
		[
			WindowWidget
		];
}

void UOCGEditorSubsystem::RegisterToolbarEntry()
{
	if (!UToolMenus::IsToolMenuUIEnabled())
	{
		return;
	}

	UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu(OCGToolbarMenuName);
	FToolMenuSection& Section = ToolbarMenu->FindOrAddSection(OCGToolbarSectionName);
	Section.AddEntry(FToolMenuEntry::InitToolBarButton(
		TEXT("OCGGenerate"),
		FUIAction(FExecuteAction::CreateUObject(this, &UOCGEditorSubsystem::OnGenerateClicked)),
		LOCTEXT("OCGGenerateLabel", "OCG"),
		LOCTEXT("OCGGenerateTooltip", "Open OCG Generator window"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "LandscapeEditor.NewLandscape")
	));
}

void UOCGEditorSubsystem::UnregisterToolbarEntry()
{
	if (!UToolMenus::IsToolMenuUIEnabled())
	{
		return;
	}

	UToolMenus::Get()->RemoveSection(OCGToolbarMenuName, OCGToolbarSectionName);
}

void UOCGEditorSubsystem::OnGenerateClicked()
{
	// v2: 에셋 피커 다이얼로그 대신 OCG Window 탭을 엽니다.
	// Preset 선택과 Generate 실행은 OCG Window 내에서 처리합니다.
	OpenOCGWindow();
}

bool UOCGEditorSubsystem::ValidatePreset(const UMapPreset* Preset) const
{
	if (!Preset)
	{
		UE_LOG(LogOCGModule, Error, TEXT("ExecuteGeneration: Preset is null."));
		return false;
	}

	if (Preset->Biomes.IsEmpty())
	{
		FMessageDialog::Open(
			EAppMsgType::Ok,
			NSLOCTEXT("OCG.Errors", "EmptyBiomes", "At least one biome must be defined in the preset before generating the level."),
			NSLOCTEXT("OCG", "ErrorTitle", "Error")
		);
		return false;
	}

	for (const FOCGBiomeSettings& Biome : Preset->Biomes)
	{
		if (Biome.BiomeName == NAME_None)
		{
			FMessageDialog::Open(
				EAppMsgType::Ok,
				NSLOCTEXT("OCG.Errors", "InvalidBiomeName", "Invalid Biome Name. Please set a valid name for each biome."),
				NSLOCTEXT("OCG", "ErrorTitle", "Error")
			);
			return false;
		}
	}

	return true;
}

void UOCGEditorSubsystem::PersistLastUsedPreset(const UMapPreset* Preset)
{
	if (!Preset)
	{
		return;
	}

	GConfig->SetString(OCGConfigSection, OCGLastPresetKey, *Preset->GetPathName(), GEditorPerProjectIni);
}

void UOCGEditorSubsystem::RestoreLastUsedPreset()
{
	FString SavedPath;
	if (GConfig->GetString(OCGConfigSection, OCGLastPresetKey, SavedPath, GEditorPerProjectIni) && !SavedPath.IsEmpty())
	{
		LastUsedPresetAsset = TSoftObjectPtr<UMapPreset>(FSoftObjectPath(SavedPath));
	}
}

void UOCGEditorSubsystem::OnMapPresetPropertyChanged(const UMapPreset* Preset, FName PropertyName)
{
	if (!Preset || !GEditor)
	{
		return;
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return;
	}

	// 에디터 월드의 모든 OCGLandscapeVolume 액터에 변경사항을 반영합니다.
	TArray<AOCGLandscapeVolume*> Actors = FOCGUtils::GetAllActorsOfClass<AOCGLandscapeVolume>(World);
	for (AOCGLandscapeVolume* VolumeActor : Actors)
	{
		if (!VolumeActor)
		{
			continue;
		}

		if (PropertyName == GET_MEMBER_NAME_CHECKED(UMapPreset, PCGGraph))
		{
			if (UPCGComponent* PCGComponent = VolumeActor->GetPCGComponent())
			{
				PCGComponent->SetGraph(Preset->PCGGraph);
			}
		}
		else if (PropertyName == GET_MEMBER_NAME_CHECKED(UMapPreset, bAutoGenerate))
		{
			VolumeActor->SetEditorAutoGenerate(Preset->bAutoGenerate);
		}
	}
}

#undef LOCTEXT_NAMESPACE
