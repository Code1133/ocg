#pragma once

class FAdvancedPreviewScene;

class FMapPresetViewportClient : public FEditorViewportClient
{
public:
	FMapPresetViewportClient(TSharedPtr<class FMapPresetEditorToolkit> InToolkit, UWorld* InWorld,
		TSharedPtr<SEditorViewport> InEditorViewportWidget);

	virtual void Tick(float DeltaSeconds) override;
	// Export the current preview scene to a level
	virtual UWorld* GetWorld() const override;

private:
	TWeakObjectPtr<UWorld> MapPresetEditorWorld;
};
