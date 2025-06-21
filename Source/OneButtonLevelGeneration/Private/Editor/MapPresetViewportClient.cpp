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
	
	if (PreviewScene)
	{
		PreviewScene->GetWorld()->Tick(LEVELTICK_All, DeltaSeconds);
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

    // IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    // if (!DesktopPlatform)
    // {
    //     UE_LOG(LogTemp, Error, TEXT("Failed to get IDesktopPlatform module."));
    //     return;
    // }
    //
    // void* ParentWindowHandle = nullptr; // 부모 창 핸들 (보통 null)
    // FString DefaultFilePath = FPaths::ProjectSavedDir() + TEXT("OCGLevels"); // 기본 저장 디렉토리 (파일 대화 상자의 초기 경로)
    // FString FileTypes = TEXT("Unreal Map (*.umap)|*.umap"); // 파일 타입 필터
    // TArray<FString> OutFilenames;
    //
    // // 저장 대화 상자 열기
    // bool bFileChosen = DesktopPlatform->SaveFileDialog(
    //     ParentWindowHandle,
    //     TEXT("Save Generated Map As"),// 다이얼로그 제목
    //     DefaultFilePath,
    //     TEXT("GeneratedMap.umap"),    // 기본 파일 이름
    //     FileTypes,
    //     EFileDialogFlags::None,       // 특별한 플래그 없음
    //     OutFilenames
    // );
    //
    // // 사용자가 저장 취소 또는 파일 선택 실패 시 처리
    // if (!bFileChosen || OutFilenames.Num() == 0)
    // {
    //     UE_LOG(LogTemp, Warning, TEXT("Map save cancelled by user."));    
    //     return; 
    // }
    //
    // FString TargetFilePath = OutFilenames[0]; // 사용자가 선택한 전체 파일 경로 (예: C:/Project/Saved/MyMaps/GeneratedMap.umap)

	//bool bSuccess = FEditorFileUtils::SaveMap(SourceWorld, TargetFilePath);
	bool bSuccess = FEditorFileUtils::SaveLevelAs(SourceWorld->GetCurrentLevel());
	if (bSuccess)
	{
	}
}

UWorld* FMapPresetViewportClient::GetWorld() const
{
	return MapPresetEditorWorld.Get();
}

