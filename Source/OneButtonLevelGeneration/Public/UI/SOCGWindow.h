// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

struct FAssetData;
class UMapPreset;
class IDetailsView;

/**
 * OCG Generator 전용 에디터 패널 (SDockTab 콘텐츠)
 *
 * 구성:
 *  - Preset Bar   : SObjectPropertyEntryBox (UE 표준 에셋 피커, Browse 내장)
 *  - Quick Strip  : Seed / RiverSeed 인라인 편집 + 주사위 버튼, Island / Water 체크박스
 *  - Action Bar   : Generate All, Regen River, Force PCG
 *  - Details View : FDetailsView (MapPreset 모든 프로퍼티)
 */
class SOCGWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SOCGWindow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** 표시할 MapPreset을 설정하고 DetailsView를 갱신합니다. */
	void SetPreset(UMapPreset* InPreset);

	/** 현재 선택된 MapPreset을 반환합니다. nullptr 가능. */
	UMapPreset* GetCurrentPreset() const;

private:
	// Section builders
	TSharedRef<SWidget> BuildPresetBar();
	TSharedRef<SWidget> BuildQuickStrip();
	TSharedRef<SWidget> BuildActionBar();

	// Preset bar
	FString GetMapPresetPath() const;
	void OnMapPresetChanged(const FAssetData& AssetData);

	// Seeds
	TOptional<int32> GetSeedValue() const;
	void OnSeedCommitted(int32 NewVal, ETextCommit::Type CommitType);
	FReply OnRandomizeSeedClicked();

	TOptional<int32> GetRiverSeedValue() const;
	void OnRiverSeedCommitted(int32 NewVal, ETextCommit::Type CommitType);
	FReply OnRandomizeRiverSeedClicked();

	FReply OnRandomizeAllSeedsClicked();

	// Quick strip checkboxes
	ECheckBoxState IsIslandChecked() const;
	void OnIslandChanged(ECheckBoxState NewState);
	ECheckBoxState IsWaterChecked() const;
	void OnWaterChanged(ECheckBoxState NewState);

	// Actions
	FReply OnGenerateAllClicked();
	FReply OnRegenRiverClicked();
	FReply OnForcePCGClicked();

	/** Generate All / Force PCG / 시드 편집 활성 조건 */
	bool CanExecuteAction() const;

	/** Regen River 활성 조건: 프리셋 선택 + bGenerateRiver == true */
	bool CanRegenRiver() const;

	// State
	TWeakObjectPtr<UMapPreset> CurrentPreset;
	TSharedPtr<IDetailsView> DetailsView;
};
