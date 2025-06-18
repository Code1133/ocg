#pragma once

class FAdvancedPreviewScene;

class FMapPresetViewportClient : public FEditorViewportClient
{
public:
	FMapPresetViewportClient(TSharedPtr<class FMapPresetEditorToolkit> InToolKit, FAdvancedPreviewScene* InPreviewScene,
		TSharedPtr<SEditorViewport> InEditorViewportWidget);

	virtual void Tick(float DeltaSeconds) override;
};
