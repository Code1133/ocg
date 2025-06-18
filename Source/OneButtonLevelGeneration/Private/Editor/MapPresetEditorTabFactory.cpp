#include "Editor/MapPresetEditorTabFactory.h"
#include "Data/MapPreset.h"
#include "Editor/MapPresetEditorToolkit.h"

const FName FMapPresetEditorTabFactory::TabId("MapPreset_Details");

TSharedRef<SWidget> FMapPresetEditorTabFactory::CreateTabBody(TSharedRef<FMapPresetEditorToolkit> InToolkit)
{
	// 프로퍼티 에디터 모듈 로드
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	// DetailsView 생성 시 필요한 인수 설정
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bHideSelectionTip = true;

	// DetailsView 위젯 생성
	TSharedRef<IDetailsView> DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);

	// DetailsView가 표시할 에셋 객체를 설정
	DetailsView->SetObject(InToolkit->GetMapPreset());

	// Slate 위젯을 생성하여 반환
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			DetailsView
		];
}
