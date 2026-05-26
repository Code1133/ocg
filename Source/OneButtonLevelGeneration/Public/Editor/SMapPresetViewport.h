// Copyright (c) 2025 Code1133. All rights reserved.
// [DEPRECATED v2] Custom MapPreset editor. Replaced by default property editor. Scheduled for removal.

#pragma once

#include "CoreMinimal.h"
#include "SEditorViewport.h"

class FAdvancedPreviewScene;
class FMapPresetEditorToolkit;
class FMapPresetViewportClient;


class SMapPresetViewport : public SEditorViewport
{
public:
	SLATE_BEGIN_ARGS(SMapPresetViewport) {}
		SLATE_ARGUMENT(TWeakPtr<FMapPresetEditorToolkit>, MapPresetEditorToolkit)
		SLATE_ARGUMENT(UWorld*, World)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SMapPresetViewport() override;

	virtual UWorld* GetWorld() const override;

protected:
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;

private:
	TWeakObjectPtr<UWorld> MapPresetEditorWorld;
	TWeakPtr<FMapPresetEditorToolkit> ToolkitPtr;
	TSharedPtr<FMapPresetViewportClient> ViewportClient;
};

