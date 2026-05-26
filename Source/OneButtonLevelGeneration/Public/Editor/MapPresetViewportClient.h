// Copyright (c) 2025 Code1133. All rights reserved.
// [DEPRECATED v2] Custom MapPreset editor. Replaced by default property editor. Scheduled for removal.

#pragma once

class FAdvancedPreviewScene;

class FMapPresetViewportClient : public FEditorViewportClient
{
public:
	FMapPresetViewportClient(TSharedPtr<class FMapPresetEditorToolkit> InToolkit, UWorld* InWorld,
		const TSharedPtr<SEditorViewport>& InEditorViewportWidget);

	virtual void Tick(float DeltaSeconds) override;
	// Export the current preview scene to a level
	virtual UWorld* GetWorld() const override;

private:
	TWeakObjectPtr<UWorld> MapPresetEditorWorld;
};
