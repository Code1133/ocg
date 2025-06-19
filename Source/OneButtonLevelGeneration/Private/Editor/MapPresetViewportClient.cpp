#include "Editor/MapPresetViewportClient.h"

#include <OCGLevelGenerator.h>

#include "AdvancedPreviewScene.h"
#include "DesktopPlatformModule.h"
#include "FileHelpers.h"
#include "IDesktopPlatform.h"
#include "Editor/MapPresetEditorToolkit.h"
#include "ViewportToolbar/UnrealEdViewportToolbar.h"

FMapPresetViewportClient::FMapPresetViewportClient(TSharedPtr<class FMapPresetEditorToolkit> InToolKit,
                                                   FAdvancedPreviewScene* InPreviewScene, TSharedPtr<SEditorViewport> InEditorViewportWidget)
		: FEditorViewportClient(nullptr, InPreviewScene, InEditorViewportWidget)
{
	SetRealtime(true);
	EngineShowFlags.SetGrid(true);

	ViewportType = LVT_Perspective; // 원근 뷰

	InToolKit->OnGenerateButtonClicked.AddLambda([&]()
	{
		UE_LOG(LogTemp, Display, TEXT("Generate button clicked in viewport client!"));
		if (LevelGenerator)
		{
			LevelGenerator->OnClickGenerate(InPreviewScene->GetWorld());
		}
	});

	InToolKit->OnExportToLevelButtonClicked.AddLambda([&]()
	{
		ExportPreviewSceneToLevel();
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

void FMapPresetViewportClient::ExportPreviewSceneToLevel()
{
    // 미리 보기 월드 유효성 검사
    if (!LevelGenerator || !PreviewScene || !PreviewScene->GetWorld() || !PreviewScene->GetWorld()->GetCurrentLevel())
    {
        UE_LOG(LogTemp, Warning, TEXT("PreviewScene or its World is null, cannot export."));
        return;
    }

    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (!DesktopPlatform)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get IDesktopPlatform module."));
        return;
    }

    void* ParentWindowHandle = nullptr; // 부모 창 핸들 (보통 null)
    FString DefaultFilePath = FPaths::ProjectSavedDir() + TEXT("OCGLevels"); // 기본 저장 디렉토리 (파일 대화 상자의 초기 경로)
    FString FileTypes = TEXT("Unreal Map (*.umap)|*.umap"); // 파일 타입 필터
    TArray<FString> OutFilenames;
    
    // 저장 대화 상자 열기
    bool bFileChosen = DesktopPlatform->SaveFileDialog(
        ParentWindowHandle,
        TEXT("Save Generated Map As"),// 다이얼로그 제목
        DefaultFilePath,
        TEXT("GeneratedMap.umap"),    // 기본 파일 이름
        FileTypes,
        EFileDialogFlags::None,       // 특별한 플래그 없음
        OutFilenames
    );

    // 사용자가 저장 취소 또는 파일 선택 실패 시 처리
    if (!bFileChosen || OutFilenames.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Map save cancelled by user."));    
        return; 
    }

    FString TargetFilePath = OutFilenames[0]; // 사용자가 선택한 전체 파일 경로 (예: C:/Project/Saved/MyMaps/GeneratedMap.umap)

    // 1. 저장될 패키지 이름 (Long Package Name) 결정
    // 예: C:/Project/Saved/MyMaps/GeneratedMap.umap -> /Game/MyMaps/GeneratedMap
    FString PackageNameStr = FPackageName::FilenameToLongPackageName(TargetFilePath);
    if (PackageNameStr.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to generate package name from file path: %s"), *TargetFilePath);
        return;
    }

    // 2. 월드 에셋의 짧은 이름 (패키지 내에서 UWorld 객체의 이름) 결정
    // 예: GeneratedMap.umap -> GeneratedMap
    FName WorldAssetName = FName(*FPaths::GetBaseFilename(TargetFilePath));

	UPackage* TargetPackage = CreatePackage(*PackageNameStr);
	if (!TargetPackage)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create/get package %s for new world."), *PackageNameStr);
		return;
	}

	TargetPackage->SetDirtyFlag(true);

	ERHIFeatureLevel::Type TargetFeatureLevel = GEditor ? GEditor->DefaultWorldFeatureLevel : ERHIFeatureLevel::Num; 
	
    // 3. UWorld::CreateWorld를 사용하여 새로운 UWorld 인스턴스 생성
	UWorld* NewWorld = UWorld::CreateWorld(
		EWorldType::Editor,         // InWorldType: 에디터 월드 타입
		true,                       // bInformEngineOfWorld: 생성된 월드를 GWorld에 추가하여 에디터가 인식할 수 있도록 합니다.
		WorldAssetName,             // WorldName: 생성될 월드 UObject의 이름 (패키지 내에서 UWorld 에셋의 이름)
		TargetPackage,              // InWorldPackage: 생성하거나 로드한 UPackage 객체
		true,                       // bAddToRoot: UWorld 객체를 가비지 컬렉션으로부터 보호하기 위해 루트에 추가합니다.
		TargetFeatureLevel,         // InFeatureLevel: 이 월드가 렌더링될 Feature Level을 지정합니다.
		nullptr,                    // InIVS: InitializationValues. 일반적으로 nullptr로 두어 기본 초기화 값을 사용합니다.
		false                       // bInSkipInitWorld: 월드 초기화(PersistentLevel 생성 등)를 건너뛸지 여부. false로 두어 초기화를 수행합니다.
	);

    if (!NewWorld)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create new world for saving."));
    	TargetPackage->SetDirtyFlag(false);
        return;
    }
	
	if (LevelGenerator)
	{
		LevelGenerator->OnClickGenerate(NewWorld);
	}

    // 5. 새로 생성된 UWorld를 지정된 파일 경로로 저장
    bool bSuccess = FEditorFileUtils::SaveMap(NewWorld, TargetFilePath);

    if (bSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("Successfully saved generated map to: %s"), *TargetFilePath);
    	NewWorld->DestroyWorld(false);
    	TargetPackage->MarkAsUnloaded();
    	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save generated map to: %s"), *TargetFilePath);
    }
	TargetPackage->SetDirtyFlag(false);
}

