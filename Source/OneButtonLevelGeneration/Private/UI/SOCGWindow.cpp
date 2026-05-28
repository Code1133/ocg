// Copyright (c) 2025-2026 Code1133. All rights reserved.
#include "UI/SOCGWindow.h"

#include "Data/MapPreset.h"
#include "Subsystems/OCGEditorSubsystem.h"

#include "Editor.h"
#include "IDetailsView.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyEditorDelegates.h"
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
	static const float SidebarWidth  = 130.0f;
}

// Sidebar navigation IDs and category mappings
namespace OCGNav
{
	static const FName TerrainBasics   = TEXT("terrain_basics");
	static const FName TerrainAdvanced = TEXT("terrain_advanced");
	static const FName Water           = TEXT("water");
	static const FName PCG             = TEXT("pcg");
	static const FName OCG             = TEXT("ocg");

	/** Nav ID -> 표시할 MapPreset UPROPERTY Category 목록 */
	static const TMap<FName, TArray<FName>> CategoryMap =
	{
		{
			TerrainBasics, {
				TEXT("World Settings | Basics | Landscape Settings"),
				TEXT("World Settings | Basics | Height"),
				TEXT("World Settings | Basics | Temperature"),
				TEXT("World Settings | Basics | Noise"),
			}
		},
		{
			TerrainAdvanced, {
				TEXT("World Settings | Advanced | Height"),
				TEXT("World Settings | Advanced | Temperature"),
				TEXT("World Settings | Advanced | Humidity"),
				TEXT("World Settings | Advanced | Noise"),
				TEXT("World Settings | Advanced | Erosion"),
			}
		},
		{ Water, { TEXT("Ocean Settings"), TEXT("River Settings") } },
		{ PCG, { TEXT("PCG") } },
		{ OCG, { TEXT("OCG") } },
	};
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

	// 프로퍼티 가시성 필터 델리게이트 등록
	DetailsView->SetIsPropertyVisibleDelegate(
		FIsPropertyVisible::CreateRaw(this, &SOCGWindow::IsPropertyVisible));

	// 초기 nav 항목 적용 (Terrain > Basics)
	OnNavItemClicked(OCGNav::TerrainBasics);

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

		// Body: Sidebar (좌) + DetailsView (우)
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				BuildSidebar()
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				DetailsView.ToSharedRef()
			]
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

TSharedRef<SWidget> SOCGWindow::BuildSidebar()
{
	// 단색 배경을 그리기 위한 정적 브러시.
	// SBorder의 BorderBackgroundColor가 이 브러시를 곱셈(tint)하여 최종 색상을 결정합니다.
	// "NoBorder"는 실제로 렌더링되지 않으므로 단색 표현에 적합하지 않습니다.
	static const FSlateColorBrush SolidBrush(FLinearColor::White);

	// 항목 하나를 생성하는 로컬 헬퍼.
	// IsActiveFunc : 이 항목이 "활성" 상태인지 반환하는 조건 람다
	// bSub         : true이면 들여쓰기 적용 (Terrain sub-item)
	auto MakeNavItem = [this](
		FName Id,
		const FText& Label,
		bool bSub,
		TFunction<bool()> IsActiveFunc
	) -> TSharedRef<SWidget>
	{
		const float LeftPad = bSub ? 20.f : 8.f;

		return SNew(SHorizontalBox)

		// 좌측 2px 액센트 바: 활성 시 파란색, 비활성 시 투명
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SBox)
			.WidthOverride(2.f)
			[
				SNew(SBorder)
				.Padding(0.f)
				.BorderImage(&SolidBrush)
				.BorderBackgroundColor_Lambda([IsActiveFunc]() -> FLinearColor
				{
					return IsActiveFunc()
						? FLinearColor(0.2f, 0.5f, 1.0f, 1.0f)
						: FLinearColor::Transparent;
				})
			]
		]

		// 우측 버튼 영역: 활성 시 짙은 파란 배경
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(SBorder)
			.Padding(0.f)
			.BorderImage(&SolidBrush)
			.BorderBackgroundColor_Lambda([IsActiveFunc]() -> FLinearColor
			{
				return IsActiveFunc()
					? FLinearColor(0.08f, 0.15f, 0.30f, 0.90f)
					: FLinearColor::Transparent;
			})
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "NoBorder")
				.HAlign(HAlign_Left)
				.ContentPadding(FMargin(LeftPad, 6.f, 8.f, 6.f))
				.OnClicked_Lambda([this, Id]()
				{
					OnNavItemClicked(Id);
					return FReply::Handled();
				})
				[
					SNew(STextBlock)
					.Text(Label)
					.Font(FAppStyle::GetFontStyle("SmallFont"))
					.ColorAndOpacity_Lambda([IsActiveFunc]() -> FSlateColor
					{
						return FSlateColor(IsActiveFunc()
							? FLinearColor(0.9f, 0.95f, 1.0f)   // 활성: 청백색
							: FLinearColor(0.55f, 0.55f, 0.55f)); // 비활성: 회색
					})
				]
			]
		];
	};

	// Terrain sub-item 가시성 (terrain_basics or terrain_advanced가 선택된 경우에만 표시)
	auto TerrainSubVisibility = [this]() -> EVisibility
	{
		return (ActiveNavItem == OCGNav::TerrainBasics || ActiveNavItem == OCGNav::TerrainAdvanced)
			? EVisibility::Visible : EVisibility::Collapsed;
	};

	// 그룹 구분선 (좌우 여백 포함)
	auto MakeSeparator = []() -> TSharedRef<SWidget>
	{
		return SNew(SBorder)
			.Padding(FMargin(8.f, 4.f))
			.BorderImage(FAppStyle::GetBrush("NoBorder"))
			[
				SNew(SSeparator)
				.Orientation(Orient_Horizontal)
				.Thickness(1.0f)
			];
	};

	return SNew(SBox)
		.WidthOverride(OCGWindowLayout::SidebarWidth)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(0.f)
			[
				SNew(SVerticalBox)

				// 헤더
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(8.f, 5.f, 8.f, 4.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("SidebarHeader", "SETTINGS"))
					.Font(FAppStyle::GetFontStyle("TinyFont"))
					.ColorAndOpacity(FLinearColor(0.4f, 0.4f, 0.4f))
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SSeparator).Orientation(Orient_Horizontal)
				]

				// ── Terrain 그룹 ──────────────────────────────
				// Terrain (클릭 시 terrain_basics 선택 / 두 sub 중 하나가 활성이면 하이라이트)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					MakeNavItem(
						OCGNav::TerrainBasics,
						LOCTEXT("NavTerrain", "Terrain"),
						false,
						[this]() { return ActiveNavItem == OCGNav::TerrainBasics || ActiveNavItem == OCGNav::TerrainAdvanced; }
					)
				]

				// Terrain sub-items (terrain이 활성일 때만 표시)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SVerticalBox)
					.Visibility_Lambda(TerrainSubVisibility)

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						MakeNavItem(
							OCGNav::TerrainBasics,
							LOCTEXT("NavTerrainBasics", "Basics"),
							true,
							[this]() { return ActiveNavItem == OCGNav::TerrainBasics; }
						)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						MakeNavItem(
							OCGNav::TerrainAdvanced,
							LOCTEXT("NavTerrainAdvanced", "Advanced"),
							true,
							[this]() { return ActiveNavItem == OCGNav::TerrainAdvanced; }
						)
					]
				]

				// ── 구분선: Terrain / Water ───────────────────
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					MakeSeparator()
				]

				// Water
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					MakeNavItem(
						OCGNav::Water,
						LOCTEXT("NavWater", "Water"),
						false,
						[this]() { return ActiveNavItem == OCGNav::Water; }
					)
				]

				// ── 구분선: Water / PCG+OCG ───────────────────
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					MakeSeparator()
				]

				// PCG
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					MakeNavItem(
						OCGNav::PCG,
						LOCTEXT("NavPCG", "PCG"),
						false,
						[this]() { return ActiveNavItem == OCGNav::PCG; }
					)
				]

				// OCG
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					MakeNavItem(
						OCGNav::OCG,
						LOCTEXT("NavOCG", "OCG"),
						false,
						[this]() { return ActiveNavItem == OCGNav::OCG; }
					)
				]
			]
		];
}

void SOCGWindow::OnNavItemClicked(FName ItemId)
{
	ActiveNavItem = ItemId;

	// AllowedCategories 업데이트
	AllowedCategories.Reset();
	if (const TArray<FName>* Cats = OCGNav::CategoryMap.Find(ItemId))
	{
		for (const FName& Cat : *Cats)
		{
			AllowedCategories.Add(Cat);
		}
	}

	// DetailsView에 필터 재적용
	if (DetailsView.IsValid())
	{
		DetailsView->ForceRefresh();
	}
}

bool SOCGWindow::IsPropertyVisible(const FPropertyAndParent& PropertyAndParent) const
{
	if (AllowedCategories.IsEmpty())
	{
		return true;
	}

	// 프로퍼티 자신의 카테고리 확인
	const FString OwnCategory = PropertyAndParent.Property.GetMetaData(TEXT("Category"));
	if (!OwnCategory.IsEmpty() && AllowedCategories.Contains(FName(*OwnCategory)))
	{
		return true;
	}

	// 부모 체인 확인 (배열/구조체 내부 프로퍼티 대응)
	for (const FProperty* Parent : PropertyAndParent.ParentProperties)
	{
		if (!Parent) { continue; }
		const FString ParentCategory = Parent->GetMetaData(TEXT("Category"));
		if (!ParentCategory.IsEmpty() && AllowedCategories.Contains(FName(*ParentCategory)))
		{
			return true;
		}
	}

	return false;
}

#undef LOCTEXT_NAMESPACE
