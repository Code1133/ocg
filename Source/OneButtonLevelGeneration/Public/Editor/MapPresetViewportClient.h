// Copyright (c) 2025 Code1133. All rights reserved.

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
