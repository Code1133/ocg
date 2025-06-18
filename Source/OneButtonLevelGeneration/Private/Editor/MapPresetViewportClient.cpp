#include "Editor/MapPresetViewportClient.h"
#include "AdvancedPreviewScene.h"

FMapPresetViewportClient::FMapPresetViewportClient(TSharedPtr<class FMapPresetEditorToolkit> InToolKit,
	FAdvancedPreviewScene* InPreviewScene, TSharedPtr<SEditorViewport> InEditorViewportWidget)
		: FEditorViewportClient(nullptr, InPreviewScene, InEditorViewportWidget)
{
	SetRealtime(true);
	EngineShowFlags.SetGrid(true);
}

void FMapPresetViewportClient::Tick(float DeltaSeconds)
{
	FEditorViewportClient::Tick(DeltaSeconds);

	if (PreviewScene)
	{
		PreviewScene->GetWorld()->Tick(LEVELTICK_All, DeltaSeconds);
	}
}
