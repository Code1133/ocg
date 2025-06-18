#include "Editor/MapPresetViewport.h"
#include "Editor/MapPresetViewportClient.h"

void SMapPresetViewport::Construct(const FArguments& InArgs, TSharedPtr<FAdvancedPreviewScene> InPreviewScene)
{
	ToolkitPtr = InArgs._MapPresetEditorToolkit;
	PreviewScene = InPreviewScene;

	SEditorViewport::Construct(SEditorViewport::FArguments());
}

SMapPresetViewport::~SMapPresetViewport()
{
}

TSharedRef<FEditorViewportClient> SMapPresetViewport::MakeEditorViewportClient()
{
	// 여기서 ViewportClient를 생성하고 SEditorViewport의 베이스 클래스로 전달.
	ViewportClient = MakeShareable(new FMapPresetViewportClient(ToolkitPtr.Pin(), PreviewScene.Get(), SharedThis(this)));
	return ViewportClient.ToSharedRef();
}
