#include "Editor/MapPresetViewport.h"
#include "Editor/MapPresetViewportClient.h"

void SMapPresetViewport::Construct(const FArguments& InArgs)
{
	ToolkitPtr = InArgs._MapPresetEditorToolkit;
	MapPresetEditorWorld = InArgs._World;

	SEditorViewport::Construct(SEditorViewport::FArguments());
}

SMapPresetViewport::~SMapPresetViewport()
{
}

UWorld* SMapPresetViewport::GetWorld() const
{
	return MapPresetEditorWorld.Get();
}

TSharedRef<FEditorViewportClient> SMapPresetViewport::MakeEditorViewportClient()
{
	// 여기서 ViewportClient를 생성하고 SEditorViewport의 베이스 클래스로 전달.
	ViewportClient = MakeShareable(new FMapPresetViewportClient(ToolkitPtr.Pin(), MapPresetEditorWorld.Get(), SharedThis(this)));
	return ViewportClient.ToSharedRef();
}
