#include "Editor/MapPresetViewportClient.h"

#include <OCGLevelGenerator.h>

#include "FileHelpers.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ModelComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Editor/MapPresetEditorToolkit.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyLight.h"

FMapPresetViewportClient::FMapPresetViewportClient(TSharedPtr<class FMapPresetEditorToolkit> InToolkit, UWorld* InWorld,
                                                   TSharedPtr<SEditorViewport> InEditorViewportWidget)
	: FEditorViewportClient(nullptr, nullptr, InEditorViewportWidget)
{
	SetRealtime(true);
	EngineShowFlags.SetGrid(true);
	bShowWidget = false;
	
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
		SetupDefaultActors();
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
	
	// 월드 복제
	UPackage* DestWorldPackage = CreatePackage(TEXT("/Temp/MapPresetEditor/World"));
	FObjectDuplicationParameters Parameters(SourceWorld, DestWorldPackage);
	Parameters.DestName = SourceWorld->GetFName();
	Parameters.DestClass = SourceWorld->GetClass();
	Parameters.DuplicateMode = EDuplicateMode::World;
	Parameters.PortFlags = PPF_Duplicate;

	UWorld* DuplicatedWorld = CastChecked<UWorld>(StaticDuplicateObjectEx(Parameters));
	DuplicatedWorld->SetFeatureLevel(SourceWorld->GetFeatureLevel());

	ULevel* SourceLevel = SourceWorld->PersistentLevel;
	ULevel* DuplicatedLevel = DuplicatedWorld->PersistentLevel;

	// AOCGLevelGenerator 액터를 찾아서 제거합니다.
	LevelGenerator ->SetMapPreset(nullptr);
	LevelGenerator = nullptr;
	for (AActor* Actor : DuplicatedLevel->Actors)
	{
		if (Actor && Actor->IsA<AOCGLevelGenerator>())
		{
			DuplicatedWorld->DestroyActor(Actor);
		}
	}

	if (DuplicatedLevel->Model != NULL
		&& DuplicatedLevel->Model == SourceLevel->Model
		&& DuplicatedLevel->ModelComponents.Num() == SourceLevel->ModelComponents.Num())
	{
		DuplicatedLevel->Model->ClearLocalMaterialIndexBuffersData();
		for (int32 ComponentIndex = 0; ComponentIndex < DuplicatedLevel->ModelComponents.Num(); ++ComponentIndex)
		{
			UModelComponent* SrcComponent = SourceLevel->ModelComponents[ComponentIndex];
			UModelComponent* DstComponent = DuplicatedLevel->ModelComponents[ComponentIndex];
			DstComponent->CopyElementsFrom(SrcComponent);
		}
	}
	
	bool bSuccess = FEditorFileUtils::SaveLevelAs(DuplicatedWorld->GetCurrentLevel());

	if (bSuccess)
	{

	}
	GEngine->DestroyWorldContext(DuplicatedWorld);
	DuplicatedWorld->DestroyWorld(true);
	DuplicatedWorld->MarkAsGarbage();
	DuplicatedWorld->SetFlags(RF_Transient);
	DuplicatedWorld->Rename(nullptr, GetTransientPackage(), REN_NonTransactional | REN_DontCreateRedirectors);
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS, true);
}

UWorld* FMapPresetViewportClient::GetWorld() const
{
	return MapPresetEditorWorld.Get();
}

TArray<UObject*> FMapPresetViewportClient::GetDefaultActors() const
{
	TArray<UObject*> DefaultActors;
	if (DirectionalLight)
	{
		DefaultActors.Add(DirectionalLight);
	}
	if (SkyAtmosphere)
	{
		DefaultActors.Add(SkyAtmosphere);
	}
	if (VolumetricCloud)
	{
		DefaultActors.Add(VolumetricCloud);
	}
	if (ExponentialHeightFog)
	{
		DefaultActors.Add(ExponentialHeightFog);
	}
	return DefaultActors;
}

void FMapPresetViewportClient::SetupDefaultActors()
{
	if (!MapPresetEditorWorld.IsValid())
		return;

	const FTransform Transform(FVector(0.0f, 0.0f, 0.0f));
	ASkyLight* SkyLight = Cast<ASkyLight>(GEditor->AddActor(MapPresetEditorWorld->GetCurrentLevel(), ASkyLight::StaticClass(), Transform));
	SkyLight->GetLightComponent()->SetMobility(EComponentMobility::Movable);
	SkyLight->GetLightComponent()->SetRealTimeCaptureEnabled(true);

	DirectionalLight = Cast<ADirectionalLight>(GEditor->AddActor(MapPresetEditorWorld->GetCurrentLevel(), ADirectionalLight::StaticClass(), Transform));
	DirectionalLight->GetComponent()->bAtmosphereSunLight = 1;
	DirectionalLight->GetComponent()->AtmosphereSunLightIndex = 0;
	// The render proxy is create right after AddActor, so we need to mark the render state as dirty again to get the new values set on the render side too.
	DirectionalLight->MarkComponentsRenderStateDirty();

	SkyAtmosphere = Cast<ASkyAtmosphere>(GEditor->AddActor(MapPresetEditorWorld->GetCurrentLevel(), ASkyAtmosphere::StaticClass(), Transform));

	VolumetricCloud = Cast<AVolumetricCloud>(GEditor->AddActor(MapPresetEditorWorld->GetCurrentLevel(), AVolumetricCloud::StaticClass(), Transform));

	ExponentialHeightFog = Cast<AExponentialHeightFog>(GEditor->AddActor(MapPresetEditorWorld->GetCurrentLevel(), AExponentialHeightFog::StaticClass(), Transform));
}
