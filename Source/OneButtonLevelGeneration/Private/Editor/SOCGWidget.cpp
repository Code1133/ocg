// Copyright (c) 2025 Code1133. All rights reserved.

#include "Editor/SOCGWidget.h"

#include "AssetToolsModule.h"
#include "OCGLevelGenerator.h"
#include "PropertyCustomizationHelpers.h"
#include "Selection.h"
#include "Data/MapPreset.h"
#include "Editor/MapPresetFactory.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Utils/OCGUtils.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

void SOCGWidget::Construct([[maybe_unused]] const FArguments& InArgs)
{
    RegisterDelegates();
    CheckForExistingLevelGenerator();

    ChildSlot
    [
        SNew(SVerticalBox)

        // Create LevelGenerator Button
        + SVerticalBox::Slot()
        .AutoHeight().Padding(5)
        [
            SNew(SButton)
            .Text(this, &SOCGWidget::GetGeneratorButtonText)
            .OnClicked(this, &SOCGWidget::OnGeneratorButtonClicked)
            .IsEnabled(this, &SOCGWidget::IsGeneratorButtonEnabled)
        ]
        
        // Generate Level Button
        + SVerticalBox::Slot()
        .AutoHeight().Padding(5)
        [
            SNew(SButton)
            .Text(FText::FromString(TEXT("Generate")))
            .OnClicked(this, &SOCGWidget::OnGenerateLevelClicked)
            .IsEnabled(this, &SOCGWidget::IsGenerateEnabled)
        ]
        
        // MapPreset Asset Selection and Creation UI
        + SVerticalBox::Slot()
        .AutoHeight().Padding(5)
        [
            SNew(SHorizontalBox)

            // Existing Asset Selection Dropdown (Asset Picker)
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            [
                SNew(SObjectPropertyEntryBox)
                .AllowedClass(UMapPreset::StaticClass())
                .ObjectPath(this, &SOCGWidget::GetMapPresetPath)
                .OnObjectChanged(this, &SOCGWidget::OnMapPresetChanged)
                .IsEnabled_Lambda([this] { return LevelGeneratorActor.IsValid(); })
            ]

            // New Asset Creation '+' Button
            + SHorizontalBox::Slot()
            .AutoWidth().Padding(2, 0, 0, 0).VAlign(VAlign_Center)
            [
                SNew(SButton)
                .Text(FText::FromString(TEXT("+")))
                .OnClicked(this, &SOCGWidget::OnCreateNewMapPresetClicked)
                .IsEnabled_Lambda([this] { return LevelGeneratorActor.IsValid(); })
            ]
        ]

        // MapPreset Details Container
        + SVerticalBox::Slot()
        .FillHeight(1.0f).Padding(5)
        [
            SAssignNew(DetailsContainer, SBox)
        ]
    ];

    // Set initial UI state
    UpdateSelectedActor();
}

SOCGWidget::~SOCGWidget()
{
    UnregisterDelegates();
}

FReply SOCGWidget::OnCreateLevelGeneratorClicked()
{
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (World)
    {
        if (LevelGeneratorActor.IsValid())
        {
            World->DestroyActor(LevelGeneratorActor.Get());
        }

        AOCGLevelGenerator* SpawnedActor = World->SpawnActor<AOCGLevelGenerator>(FVector::ZeroVector, FRotator::ZeroRotator);
        
        // Select the newly spawned actor in the editor
        GEditor->SelectActor(SpawnedActor, true, true);
        SetSelectedActor(SpawnedActor); // Manually set the actor for the UI
        bLevelGeneratorExistsInLevel = true;
    }
    return FReply::Handled();
}

FReply SOCGWidget::OnGenerateLevelClicked()
{
    if (LevelGeneratorActor.IsValid() && LevelGeneratorActor->GetMapPreset())
    {
        // Assuming OnClickGenerate is a valid function in your AOCGLevelGenerator
        
        LevelGeneratorActor->OnClickGenerate(GEditor->GetEditorWorldContext().World());
    }
    return FReply::Handled();
}

FReply SOCGWidget::OnCreateNewMapPresetClicked()
{
    const FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
    UObject* NewAsset = AssetToolsModule.Get().CreateAssetWithDialog(UMapPreset::StaticClass(), NewObject<UMapPresetFactory>());

    if (NewAsset && LevelGeneratorActor.IsValid())
    {
        UMapPreset* MapPreset = Cast<UMapPreset>(NewAsset);
        
        LevelGeneratorActor->SetMapPreset(MapPreset);
        RefreshDetailsView();
    }
    
    return FReply::Handled();
}

FString SOCGWidget::GetMapPresetPath() const
{
    if (LevelGeneratorActor.IsValid() && LevelGeneratorActor->GetMapPreset())
    {
        return LevelGeneratorActor->GetMapPreset()->GetPathName();
    }
    return FString("");
}

void SOCGWidget::OnMapPresetChanged(const FAssetData& AssetData)
{
    if (LevelGeneratorActor.IsValid())
    {
        LevelGeneratorActor->SetMapPreset(Cast<UMapPreset>(AssetData.GetAsset()));
        RefreshDetailsView();
    }
}

void SOCGWidget::OnActorSelectionChanged(UObject* NewSelection)
{
    // Respond only to actor selection changes
    if (NewSelection == GEditor->GetSelectedActors())
    {
        UpdateSelectedActor();
    }
}

void SOCGWidget::OnLevelActorDeleted(AActor* InActor)
{
    if (InActor && InActor == LevelGeneratorActor.Get())
    {
        LevelGeneratorActor.Reset();
        ClearUI();
        CheckForExistingLevelGenerator();
    }
}

bool SOCGWidget::IsGenerateEnabled() const
{
    // Enable button only if both actor and preset are valid
    return LevelGeneratorActor.IsValid() && LevelGeneratorActor->GetMapPreset() != nullptr;
}

void SOCGWidget::UpdateSelectedActor()
{
    TArray<UObject*> SelectedObjects;
    GEditor->GetSelectedActors()->GetSelectedObjects(SelectedObjects);
    
    AOCGLevelGenerator* FoundActor = nullptr;
    if (SelectedObjects.Num() == 1)
    {
        FoundActor = Cast<AOCGLevelGenerator>(SelectedObjects[0]);
    }
    SetSelectedActor(FoundActor);
}

void SOCGWidget::SetSelectedActor(AOCGLevelGenerator* NewActor)
{
    if (LevelGeneratorActor.Get() == NewActor)
    {
        return; // No change
    }

    LevelGeneratorActor = NewActor;
    RefreshDetailsView();
}

void SOCGWidget::RefreshDetailsView()
{
    if (LevelGeneratorActor.IsValid() && LevelGeneratorActor->GetMapPreset())
    {
        ShowMapPresetDetails();
    }
    else
    {
        ClearUI();
    }
}

void SOCGWidget::ClearUI()
{
    // If no valid actor is selected, clear the details view
    ClearDetailsView();
}

void SOCGWidget::ShowMapPresetDetails()
{
    if (!MapPresetDetailsView.IsValid())
    {
        FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
        FDetailsViewArgs Args;
        Args.bHideSelectionTip = true;
        MapPresetDetailsView = PropertyModule.CreateDetailView(Args);
    }
    
    MapPresetDetailsView->SetObject(LevelGeneratorActor->GetMapPreset());
    if (DetailsContainer.IsValid())
    {
        DetailsContainer->SetContent(MapPresetDetailsView.ToSharedRef());
    }
}

void SOCGWidget::ClearDetailsView()
{
    // Reset the details container to its initial message
    if (DetailsContainer.IsValid())
    {
        DetailsContainer->SetContent(
            SNew(STextBlock)
            .Text(FText::FromString(TEXT("Please select or create a Level Generator Actor.")))
            .Justification(ETextJustify::Center)
        );
    }
    
    if (MapPresetDetailsView.IsValid())
    {
        MapPresetDetailsView->SetObject(nullptr); // Clear the object in the details view
    }
}

void SOCGWidget::RegisterDelegates()
{
    if (GEngine)
    {
        OnLevelActorDeletedDelegateHandle = GEngine->OnLevelActorDeleted().AddRaw(this, &SOCGWidget::OnLevelActorDeleted);
    }
    USelection::SelectionChangedEvent.AddRaw(this, &SOCGWidget::OnActorSelectionChanged);
}

void SOCGWidget::UnregisterDelegates()
{
    if (GEngine && OnLevelActorDeletedDelegateHandle.IsValid())
    {
        GEngine->OnLevelActorDeleted().Remove(OnLevelActorDeletedDelegateHandle);
    }
    USelection::SelectionChangedEvent.RemoveAll(this);
}

void SOCGWidget::FindExistingLevelGenerator()
{
    // Find existing LevelGenerator actor in the current world
    if (const UWorld* World = GEditor->GetEditorWorldContext().World())
    {
        TArray<AOCGLevelGenerator*> Generators = FOCGUtils::GetAllActorsOfClass<AOCGLevelGenerator>(World);
        if (!Generators.IsEmpty())
        {
            AOCGLevelGenerator* Generator = Generators[0];

            GEditor->SelectActor(Generator, true, true);
            SetSelectedActor(Generator); // Manually set the actor for the UI
        }
    }
}

void SOCGWidget::CheckForExistingLevelGenerator()
{
    bLevelGeneratorExistsInLevel = false;
    if (UWorld* World = GEditor->GetEditorWorldContext().World())
    {
        TArray<AOCGLevelGenerator*> Generators = FOCGUtils::GetAllActorsOfClass<AOCGLevelGenerator>(World);
        if (!Generators.IsEmpty())
        {
            bLevelGeneratorExistsInLevel = true;
        }
    }
}

FText SOCGWidget::GetGeneratorButtonText() const
{
    if (bLevelGeneratorExistsInLevel)
    {
        return FText::FromString(TEXT("Select Level Generator"));
    }
    else
    {
        return FText::FromString(TEXT("Create Level Generator"));
    }
}

FReply SOCGWidget::OnGeneratorButtonClicked()
{
    if (bLevelGeneratorExistsInLevel)
    {
        FindExistingLevelGenerator();
    }
    else
    {
        OnCreateLevelGeneratorClicked();
    }
    return FReply::Handled();
}

bool SOCGWidget::IsGeneratorButtonEnabled() const
{
    return !LevelGeneratorActor.IsValid();
}
