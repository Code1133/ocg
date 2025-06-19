#include "Editor/MapPresetViewportClient.h"

#include <OCGLevelGenerator.h>

#include "AdvancedPreviewScene.h"
#include "Editor/MapPresetEditorToolkit.h"

FMapPresetViewportClient::FMapPresetViewportClient(TSharedPtr<class FMapPresetEditorToolkit> InToolKit,
                                                   FAdvancedPreviewScene* InPreviewScene, TSharedPtr<SEditorViewport> InEditorViewportWidget)
		: FEditorViewportClient(nullptr, InPreviewScene, InEditorViewportWidget)
{
	SetRealtime(true);
	EngineShowFlags.SetGrid(true);

	InToolKit->OnGenerateButtonClicked.AddLambda([&]()
	{
		UE_LOG(LogTemp, Display, TEXT("Generate button clicked in viewport client!"));
		if (LevelGenerator)
		{
			LevelGenerator->OnClickGenerate();
		}
	});

	UWorld* World = InPreviewScene->GetWorld();
	if (World)
	{
		LevelGenerator = World->SpawnActor<AOCGLevelGenerator>();
		LevelGenerator->SetMapPreset(InToolKit->GetMapPreset());
	}
	
}

void FMapPresetViewportClient::Tick(float DeltaSeconds)
{
	FEditorViewportClient::Tick(DeltaSeconds);

	if (PreviewScene)
	{
		PreviewScene->GetWorld()->Tick(LEVELTICK_All, DeltaSeconds);
	}
}
