#pragma once

class FAdvancedPreviewScene;

class FMapPresetViewportClient : public FEditorViewportClient
{
public:
	FMapPresetViewportClient(TSharedPtr<class FMapPresetEditorToolkit> InToolkit, UWorld* InWorld,
		TSharedPtr<SEditorViewport> InEditorViewportWidget);

	virtual void Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	virtual void Tick(float DeltaSeconds) override;

	// Export the current preview scene to a level
	void ExportPreviewSceneToLevel();
	virtual UWorld* GetWorld() const override;
	
private:
	TObjectPtr<class AOCGLevelGenerator> LevelGenerator;
	FAdvancedPreviewScene* PreviewScene = nullptr;
	TWeakObjectPtr<UWorld> MapPresetEditorWorld = nullptr;
	FMapPresetEditorToolkit* MapPresetEditorToolkit = nullptr;;
};
