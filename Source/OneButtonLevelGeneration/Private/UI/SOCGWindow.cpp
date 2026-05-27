// Copyright (c) 2025-2026 Code1133. All rights reserved.
#include "UI/SOCGWindow.h"

#include "Data/MapPreset.h"
#include "Subsystems/OCGEditorSubsystem.h"

#include "Editor.h"
#include "IDetailsView.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyEditorModule.h"
#include "Styling/AppStyle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SOCGWindow"

// Layout constants
namespace OCGWindowLayout
{
	static const FMargin SectionPadding(6.0f, 4.0f);
	static const float SeedBoxWidth = 80.0f;
}

void SOCGWindow::Construct(const FArguments& InArgs)
{
	// IDetailsView 생성
	FPropertyEditorModule& PropEdModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch      = true;
	DetailsViewArgs.bHideSelectionTip = true;
	DetailsViewArgs.bShowScrollBar    = true;
	DetailsViewArgs.bShowOptions      = false;
	DetailsViewArgs.NameAreaSettings  = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.NotifyHook        = nullptr;

	DetailsView = PropEdModule.CreateDetailView(DetailsViewArgs);

	ChildSlot
	[
		SNew(SVerticalBox)

		// Preset Bar
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 1.0f)
		[
			BuildPresetBar()
		]

		// Quick Strip (Seeds + Island/Water)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 1.0f)
		[
			BuildQuickStrip()
		]

		// Action Bar
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 2.0f)
		[
			BuildActionBar()
		]

		// FDetailsView: 나머지 공간을 모두 차지
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			DetailsView.ToSharedRef()
		]
	];
}

void SOCGWindow::SetPreset(UMapPreset* InPreset)
{
	CurrentPreset = InPreset;

	if (DetailsView.IsValid())
	{
		DetailsView->SetObject(InPreset);
	}
}

UMapPreset* SOCGWindow::GetCurrentPreset() const
{
	return CurrentPreset.Get();
}

TSharedRef<SWidget> SOCGWindow::BuildPresetBar()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(OCGWindowLayout::SectionPadding)
		[
			SNew(SHorizontalBox)

			// "Preset" 레이블
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 6.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("PresetLabel", "Preset"))
				.Font(FAppStyle::GetFontStyle("SmallFont"))
			]

			// UE 표준 에셋 참조 피커 (Browse / 드롭다운 내장)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SObjectPropertyEntryBox)
				.AllowedClass(UMapPreset::StaticClass())
				.ObjectPath(this, &SOCGWindow::GetMapPresetPath)
				.OnObjectChanged(this, &SOCGWindow::OnMapPresetChanged)
				.ToolTipText(LOCTEXT("PresetTip", "Select a Map Preset to edit and generate"))
			]
		];
}

TSharedRef<SWidget> SOCGWindow::BuildQuickStrip()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(OCGWindowLayout::SectionPadding)
		[
			SNew(SHorizontalBox)

			// Seed
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("SeedLabel", "Seed"))
				.Font(FAppStyle::GetFontStyle("SmallFont"))
				.ToolTipText(LOCTEXT("SeedTip", "Heightmap · Temperature · Erosion seed"))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.MinDesiredWidth(OCGWindowLayout::SeedBoxWidth)
				[
					SNew(SNumericEntryBox<int32>)
					.Value_Raw(this, &SOCGWindow::GetSeedValue)
					.OnValueCommitted_Raw(this, &SOCGWindow::OnSeedCommitted)
					.IsEnabled_Raw(this, &SOCGWindow::CanExecuteAction)
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(2.0f, 0.0f, 10.0f, 0.0f)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("\U0001F3B2")))
				.ToolTipText(LOCTEXT("RandSeedTip", "Randomize Seed (Heightmap · Temperature · Erosion)"))
				.OnClicked_Raw(this, &SOCGWindow::OnRandomizeSeedClicked)
				.IsEnabled_Raw(this, &SOCGWindow::CanExecuteAction)
			]

			// River Seed
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("RiverSeedLabel", "River Seed"))
				.Font(FAppStyle::GetFontStyle("SmallFont"))
				.ToolTipText(LOCTEXT("RiverSeedTip", "River path generation seed"))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.MinDesiredWidth(OCGWindowLayout::SeedBoxWidth)
				[
					SNew(SNumericEntryBox<int32>)
					.Value_Raw(this, &SOCGWindow::GetRiverSeedValue)
					.OnValueCommitted_Raw(this, &SOCGWindow::OnRiverSeedCommitted)
					.IsEnabled_Raw(this, &SOCGWindow::CanExecuteAction)
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(2.0f, 0.0f, 10.0f, 0.0f)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("\U0001F3B2")))
				.ToolTipText(LOCTEXT("RandRiverSeedTip", "Randomize River Seed"))
				.OnClicked_Raw(this, &SOCGWindow::OnRandomizeRiverSeedClicked)
				.IsEnabled_Raw(this, &SOCGWindow::CanExecuteAction)
			]

			// Randomize All
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 10.0f, 0.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("RandAllSeeds", "\U0001F3B2 All"))
				.ToolTipText(LOCTEXT("RandAllSeedsTip", "Randomize all seeds at once"))
				.OnClicked_Raw(this, &SOCGWindow::OnRandomizeAllSeedsClicked)
				.IsEnabled_Raw(this, &SOCGWindow::CanExecuteAction)
			]

			// 구분선
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Fill)
			.Padding(0.0f, 2.0f)
			[
				SNew(SSeparator)
				.Orientation(Orient_Vertical)
			]

			// Island 체크박스
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(8.0f, 0.0f, 4.0f, 0.0f)
			[
				SNew(SCheckBox)
				.IsChecked_Raw(this, &SOCGWindow::IsIslandChecked)
				.OnCheckStateChanged_Raw(this, &SOCGWindow::OnIslandChanged)
				.IsEnabled_Raw(this, &SOCGWindow::CanExecuteAction)
				.ToolTipText(LOCTEXT("IslandTip", "Generate as an island (surrounded by ocean)"))
				[
					SNew(STextBlock)
					.Text(LOCTEXT("IslandLabel", "Island"))
					.Font(FAppStyle::GetFontStyle("SmallFont"))
				]
			]

			// Water 체크박스
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(4.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SCheckBox)
				.IsChecked_Raw(this, &SOCGWindow::IsWaterChecked)
				.OnCheckStateChanged_Raw(this, &SOCGWindow::OnWaterChanged)
				.IsEnabled_Raw(this, &SOCGWindow::CanExecuteAction)
				.ToolTipText(LOCTEXT("WaterTip", "Include ocean water body"))
				[
					SNew(STextBlock)
					.Text(LOCTEXT("WaterLabel", "Water"))
					.Font(FAppStyle::GetFontStyle("SmallFont"))
				]
			]
		];
}

TSharedRef<SWidget> SOCGWindow::BuildActionBar()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(OCGWindowLayout::SectionPadding)
		[
			SNew(SHorizontalBox)

			// Generate All
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "PrimaryButton")
				.Text(LOCTEXT("GenerateAll", "Generate All"))
				.ToolTipText(LOCTEXT("GenerateAllTip",
					"Run the full OCG pipeline:\n"
					"  DataGeneration -> LandscapeGen -> Population -> Hydrology"))
				.OnClicked_Raw(this, &SOCGWindow::OnGenerateAllClicked)
				.IsEnabled_Raw(this, &SOCGWindow::CanExecuteAction)
			]

			// Regen River
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("RegenRiver", "Regen River"))
				.ToolTipText(LOCTEXT("RegenRiverTip",
					"Re-run only the Hydrology step using cached heightmap data.\n"
					"Requires bGenerateRiver = true in the preset."))
				.OnClicked_Raw(this, &SOCGWindow::OnRegenRiverClicked)
				.IsEnabled_Raw(this, &SOCGWindow::CanRegenRiver)
			]

			// Force PCG
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("ForcePCG", "Force PCG"))
				.ToolTipText(LOCTEXT("ForcePCGTip",
					"Force PCG graph re-generation on all OCGLandscapeVolumes in the current world."))
				.OnClicked_Raw(this, &SOCGWindow::OnForcePCGClicked)
				.IsEnabled_Raw(this, &SOCGWindow::CanExecuteAction)
			]
		];
}

FString SOCGWindow::GetMapPresetPath() const
{
	return CurrentPreset.IsValid() ? CurrentPreset->GetPathName() : FString();
}

void SOCGWindow::OnMapPresetChanged(const FAssetData& AssetData)
{
	UMapPreset* NewPreset = Cast<UMapPreset>(AssetData.GetAsset());
	CurrentPreset = NewPreset;

	if (DetailsView.IsValid())
	{
		DetailsView->SetObject(NewPreset);
	}
}

TOptional<int32> SOCGWindow::GetSeedValue() const
{
	if (CurrentPreset.IsValid())
	{
		return TOptional<int32>(CurrentPreset->Seed);
	}
	return TOptional<int32>();
}

void SOCGWindow::OnSeedCommitted(int32 NewVal, ETextCommit::Type /*CommitType*/)
{
	if (CurrentPreset.IsValid())
	{
		CurrentPreset->Seed = NewVal;
		(void)CurrentPreset->MarkPackageDirty();
	}
}

FReply SOCGWindow::OnRandomizeSeedClicked()
{
	if (CurrentPreset.IsValid())
	{
		CurrentPreset->Seed = FMath::RandRange(0, MAX_int32 - 1);
		(void)CurrentPreset->MarkPackageDirty();
	}
	return FReply::Handled();
}

TOptional<int32> SOCGWindow::GetRiverSeedValue() const
{
	if (CurrentPreset.IsValid())
	{
		return TOptional<int32>(CurrentPreset->RiverSeed);
	}
	return TOptional<int32>();
}

void SOCGWindow::OnRiverSeedCommitted(int32 NewVal, ETextCommit::Type /*CommitType*/)
{
	if (CurrentPreset.IsValid())
	{
		CurrentPreset->RiverSeed = NewVal;
		(void)CurrentPreset->MarkPackageDirty();
	}
}

FReply SOCGWindow::OnRandomizeRiverSeedClicked()
{
	if (CurrentPreset.IsValid())
	{
		CurrentPreset->RiverSeed = FMath::RandRange(0, MAX_int32 - 1);
		(void)CurrentPreset->MarkPackageDirty();
	}
	return FReply::Handled();
}

FReply SOCGWindow::OnRandomizeAllSeedsClicked()
{
	if (CurrentPreset.IsValid())
	{
		CurrentPreset->Seed      = FMath::RandRange(0, MAX_int32 - 1);
		CurrentPreset->RiverSeed = FMath::RandRange(0, MAX_int32 - 1);
		(void)CurrentPreset->MarkPackageDirty();
	}
	return FReply::Handled();
}

ECheckBoxState SOCGWindow::IsIslandChecked() const
{
	if (CurrentPreset.IsValid())
	{
		return CurrentPreset->bIsland ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}
	return ECheckBoxState::Undetermined;
}

void SOCGWindow::OnIslandChanged(ECheckBoxState NewState)
{
	if (CurrentPreset.IsValid())
	{
		CurrentPreset->bIsland = (NewState == ECheckBoxState::Checked);
		(void)CurrentPreset->MarkPackageDirty();
	}
}

ECheckBoxState SOCGWindow::IsWaterChecked() const
{
	if (CurrentPreset.IsValid())
	{
		return CurrentPreset->bContainWater ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}
	return ECheckBoxState::Undetermined;
}

void SOCGWindow::OnWaterChanged(ECheckBoxState NewState)
{
	if (!CurrentPreset.IsValid())
	{
		return;
	}

	UMapPreset* Preset = CurrentPreset.Get();
	Preset->bContainWater = (NewState == ECheckBoxState::Checked);

	// bContainWater 변경 -> UpdateInternalLandscapeFilterNames()를 트리거해야 합니다.
	// PostEditChangeProperty를 직접 호출해 파생 상태(내부 필터 이름)를 동기화합니다.
	if (FProperty* Prop = FindFProperty<FProperty>(
		UMapPreset::StaticClass(),
		GET_MEMBER_NAME_CHECKED(UMapPreset, bContainWater)))
	{
		FPropertyChangedEvent Event(Prop, EPropertyChangeType::ValueSet);
		Preset->PostEditChangeProperty(Event);
	}
}

FReply SOCGWindow::OnGenerateAllClicked()
{
	UMapPreset* Preset = CurrentPreset.Get();
	if (!Preset || !GEditor)
	{
		return FReply::Handled();
	}

	if (UOCGEditorSubsystem* Sub = GEditor->GetEditorSubsystem<UOCGEditorSubsystem>())
	{
		Sub->ExecuteGeneration(Preset);
	}

	return FReply::Handled();
}

FReply SOCGWindow::OnRegenRiverClicked()
{
	UMapPreset* Preset = CurrentPreset.Get();
	if (!Preset || !GEditor)
	{
		return FReply::Handled();
	}

	if (UOCGEditorSubsystem* Sub = GEditor->GetEditorSubsystem<UOCGEditorSubsystem>())
	{
		Sub->RegenerateRiverOnly(Preset);
	}

	return FReply::Handled();
}

FReply SOCGWindow::OnForcePCGClicked()
{
	if (!GEditor)
	{
		return FReply::Handled();
	}

	if (UOCGEditorSubsystem* Sub = GEditor->GetEditorSubsystem<UOCGEditorSubsystem>())
	{
		Sub->ForcePCGRegenerate();
	}

	return FReply::Handled();
}

bool SOCGWindow::CanExecuteAction() const
{
	return CurrentPreset.IsValid();
}

bool SOCGWindow::CanRegenRiver() const
{
	return CurrentPreset.IsValid() && CurrentPreset->bGenerateRiver;
}

#undef LOCTEXT_NAMESPACE
