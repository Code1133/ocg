#include "Editor/MapPresetViewportClient.h"

#include <OCGLevelGenerator.h>

#include "AdvancedPreviewScene.h"
#include "DesktopPlatformModule.h"
#include "FileHelpers.h"
#include "IDesktopPlatform.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor/MapPresetEditorToolkit.h"
#include "UObject/SavePackage.h"
#include "ViewportToolbar/UnrealEdViewportToolbar.h"

FMapPresetViewportClient::FMapPresetViewportClient(TSharedPtr<class FMapPresetEditorToolkit> InToolkit, UWorld* InWorld,
	TSharedPtr<SEditorViewport> InEditorViewportWidget)
	: FEditorViewportClient(nullptr, nullptr, InEditorViewportWidget)
{
	SetRealtime(true);
	EngineShowFlags.SetGrid(true);

	ViewportType = LVT_Perspective; // 원근 뷰
	
	MapPresetEditorToolkit = InToolkit.Get();
	
	InToolkit->OnGenerateButtonClicked.AddLambda([&]()
	{
		UE_LOG(LogTemp, Display, TEXT("Generate button clicked in viewport client!"));
		if (LevelGenerator && MapPresetEditorWorld.IsValid())
		{
			// LevelGenerator가 유효하고 MapPresetEditorWorld가 설정되어 있다면 생성 로직 실행
			LevelGenerator->OnClickGenerate(MapPresetEditorWorld.Get());
		}
	});

	InToolkit->OnExportToLevelButtonClicked.AddLambda([&]()
	{
		ExportPreviewSceneToLevel();
	});

	MapPresetEditorWorld = InWorld;
	if (InWorld)
	{
		LevelGenerator = InWorld->SpawnActor<AOCGLevelGenerator>();
		LevelGenerator->SetMapPreset(InToolkit->GetMapPreset());
	}
}

void FMapPresetViewportClient::Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	FEditorViewportClient::Draw(View, PDI);
	if (MapPresetEditorWorld.IsValid())
	{
	}
}

void FMapPresetViewportClient::Tick(float DeltaSeconds)
{
	FEditorViewportClient::Tick(DeltaSeconds);

	if (MapPresetEditorWorld.IsValid())
	{
		MapPresetEditorWorld->Tick(LEVELTICK_All, DeltaSeconds);
	}
}

void FMapPresetViewportClient::ExportPreviewSceneToLevel()
{
    if (!MapPresetEditorToolkit || !GetWorld())
    {
    	UE_LOG(LogTemp, Warning, TEXT("Toolkit or its EditorWorld is null, cannot export."));
	    return;
    }
	
	UWorld* SourceWorld = GetWorld();
	bool bSuccess = FEditorFileUtils::SaveLevelAs(SourceWorld->GetCurrentLevel());
	if (bSuccess)
	{
		
	}
}

UWorld* FMapPresetViewportClient::GetWorld() const
{
	return MapPresetEditorWorld.Get();
}

