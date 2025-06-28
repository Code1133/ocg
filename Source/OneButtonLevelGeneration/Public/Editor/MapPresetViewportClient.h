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

public:
	TArray<UObject*> GetDefaultActors() const;

private:
	void SetupDefaultActors();
private:
	TObjectPtr<class AOCGLevelGenerator> LevelGenerator;
	FAdvancedPreviewScene* PreviewScene = nullptr;
	TWeakObjectPtr<UWorld> MapPresetEditorWorld = nullptr;
	FMapPresetEditorToolkit* MapPresetEditorToolkit = nullptr;

private:
	TObjectPtr<class ADirectionalLight> DirectionalLight;
	TObjectPtr<class ASkyAtmosphere> SkyAtmosphere;
	TObjectPtr<class AVolumetricCloud> VolumetricCloud;
	TObjectPtr<class AExponentialHeightFog> ExponentialHeightFog;
};
