// Copyright (c) 2025-2026 Code1133. All rights reserved.
#include "Subsystems/OCGEditorSubsystem.h"

#include "OCGLog.h"
#include "Data/MapPreset.h"
#include "Data/OCGBiomeSettings.h"
#include "PCG/OCGLandscapeVolume.h"
#include "Subsystems/OCGDataGenerationSubsystem.h"
#include "Subsystems/OCGHydrologySubsystem.h"
#include "Subsystems/OCGLandscapeGenSubsystem.h"
#include "Subsystems/OCGPopulationSubsystem.h"
#include "Utils/OCGUtils.h"

#include "ContentBrowserModule.h"
#include "Editor.h"
#include "IContentBrowserSingleton.h"
#include "PCGComponent.h"
#include "PCGGraph.h"
#include "ToolMenus.h"
#include "AssetRegistry/AssetData.h"
#include "HAL/IConsoleManager.h"
#include "Misc/MessageDialog.h"
#include "Styling/AppStyle.h"
#include "Widgets/SWindow.h"

#define LOCTEXT_NAMESPACE "OCGEditorSubsystem"

static const TCHAR* OCGConfigSection = TEXT("OCG");
static const TCHAR* OCGLastPresetKey = TEXT("LastUsedPreset");

static const FName OCGToolbarMenuName = TEXT("LevelEditor.LevelEditorToolBar.AssetsToolBar");
static const FName OCGToolbarSectionName = TEXT("OCG");

void UOCGEditorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// MapPreset 프로퍼티 변경 구독
	// DataAsset이 직접 월드 액터를 조작하지 않도록 에디터 레이어가 대신 처리합니다.
	UMapPreset::OnPropertyChanged.AddUObject(this, &UOCGEditorSubsystem::OnMapPresetPropertyChanged);

	RestoreLastUsedPreset();
	RegisterToolbarEntry();
	RegisterConsoleCommand();
}

void UOCGEditorSubsystem::Deinitialize()
{
	UMapPreset::OnPropertyChanged.RemoveAll(this);

	UnregisterConsoleCommand();
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

void UOCGEditorSubsystem::RegenerateLast()
{
	if (!LastUsedPresetAsset.ToSoftObjectPath().IsValid())
	{
		UE_LOG(LogOCGModule, Warning, TEXT("RegenerateLast: No last used preset saved."));
		return;
	}

	UMapPreset* Preset = LastUsedPresetAsset.IsValid()
		? LastUsedPresetAsset.Get()
		: LastUsedPresetAsset.LoadSynchronous();

	if (!Preset)
	{
		UE_LOG(LogOCGModule, Warning, TEXT("RegenerateLast: Failed to load last preset '%s'."), *LastUsedPresetAsset.ToSoftObjectPath().ToString());
		return;
	}

	ExecuteGeneration(Preset);
}

const UMapPreset* UOCGEditorSubsystem::GetLastUsedPreset() const
{
	return LastUsedPresetAsset.Get();
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
		LOCTEXT("OCGGenerateLabel", "Generate"),
		LOCTEXT("OCGGenerateTooltip", "Select a Map Preset and run the OCG generation pipeline"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Settings")
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

void UOCGEditorSubsystem::RegisterConsoleCommand()
{
	GenerateConsoleCommand = IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("OCG.Generate"),
		TEXT("Run the OCG generation pipeline. Usage: OCG.Generate /Game/Path/To/Preset  (omit path to regenerate last)"),
		FConsoleCommandWithArgsDelegate::CreateUObject(this, &UOCGEditorSubsystem::OnConsoleGenerate),
		ECVF_Default
	);
}

void UOCGEditorSubsystem::UnregisterConsoleCommand()
{
	if (GenerateConsoleCommand)
	{
		IConsoleManager::Get().UnregisterConsoleObject(GenerateConsoleCommand);
		GenerateConsoleCommand = nullptr;
	}
}

void UOCGEditorSubsystem::OnGenerateClicked()
{
	TSharedRef<SWindow> PickerWindow = SNew(SWindow)
		.Title(LOCTEXT("SelectPreset", "Select Map Preset"))
		.SizingRule(ESizingRule::UserSized)
		.ClientSize(FVector2D(640.f, 480.f))
		.IsTopmostWindow(true);

	UMapPreset* SelectedPreset = nullptr;

	FAssetPickerConfig PickerConfig;
	PickerConfig.Filter.ClassPaths.Add(UMapPreset::StaticClass()->GetClassPathName());
	PickerConfig.bAllowNullSelection = false;
	PickerConfig.SelectionMode = ESelectionMode::Single;
	PickerConfig.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateLambda([&SelectedPreset, &PickerWindow](const FAssetData& AssetData)
	{
		SelectedPreset = Cast<UMapPreset>(AssetData.GetAsset());
		PickerWindow->RequestDestroyWindow();
	});
	PickerConfig.OnAssetEnterPressed = FOnAssetEnterPressed::CreateLambda([&SelectedPreset, &PickerWindow](const TArray<FAssetData>& Assets)
	{
		if (!Assets.IsEmpty())
		{
			SelectedPreset = Cast<UMapPreset>(Assets[0].GetAsset());
			PickerWindow->RequestDestroyWindow();
		}
	});

	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	PickerWindow->SetContent(ContentBrowserModule.Get().CreateAssetPicker(PickerConfig));

	GEditor->EditorAddModalWindow(PickerWindow);

	if (SelectedPreset)
	{
		ExecuteGeneration(SelectedPreset);
	}
}

void UOCGEditorSubsystem::OnConsoleGenerate(const TArray<FString>& Args)
{
	if (Args.IsEmpty())
	{
		RegenerateLast();
		return;
	}

	FString AssetPath = Args[0];
	// 패키지 경로만 전달된 경우 (예: /Game/MyPreset) 에셋 오브젝트 이름을 자동 보완
	if (!AssetPath.Contains(TEXT(".")))
	{
		FString AssetName;
		AssetPath.Split(TEXT("/"), nullptr, &AssetName, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		AssetPath = AssetPath + TEXT(".") + AssetName;
	}

	UMapPreset* Preset = Cast<UMapPreset>(StaticLoadObject(UMapPreset::StaticClass(), nullptr, *AssetPath));
	if (!Preset)
	{
		UE_LOG(LogOCGModule, Warning, TEXT("OCG.Generate: Failed to load preset '%s'"), *AssetPath);
		return;
	}

	ExecuteGeneration(Preset);
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
