#pragma once

#include "CoreMinimal.h"
#include "SEditorViewport.h"

class FAdvancedPreviewScene;
class FMapPresetEditorToolkit;
class FMapPresetViewportClient;

class SMapPresetViewport : public SEditorViewport
{
public:
	SLATE_BEGIN_ARGS(SMapPresetViewport) {}
	SLATE_ARGUMENT(TWeakPtr<FMapPresetEditorToolkit>, MapPresetEditorToolkit)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<FAdvancedPreviewScene> InPreviewScene);
	virtual ~SMapPresetViewport() override;

protected:
	// SEditorViewport의 순수 가상 함수를 오버라이드합니다.
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;

private:
	// 이 뷰포트가 참조할 프리뷰 씬
	TSharedPtr<FAdvancedPreviewScene> PreviewScene;

	// 이 뷰포트를 소유하는 툴킷
	TWeakPtr<FMapPresetEditorToolkit> ToolkitPtr;

	// 생성된 뷰포트 클라이언트
	TSharedPtr<FMapPresetViewportClient> ViewportClient;
};
