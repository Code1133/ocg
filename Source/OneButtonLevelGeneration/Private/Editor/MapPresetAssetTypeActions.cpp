#include "Editor/MapPresetAssetTypeActions.h"
#include "Editor/MapPresetEditorToolkit.h"
#include "Data/MapPreset.h"
#include "Toolkits/ToolkitManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

FText FMapPresetAssetTypeActions::GetName() const
{
    return FText::FromString(TEXT("Map Preset"));
}

FColor FMapPresetAssetTypeActions::GetTypeColor() const
{
    return FColor::Cyan;
}

UClass* FMapPresetAssetTypeActions::GetSupportedClass() const
{
    return UMapPreset::StaticClass();
}

uint32 FMapPresetAssetTypeActions::GetCategories()
{
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
    return AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("OCG")), FText::FromString(TEXT("OCG")));
}

void FMapPresetAssetTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects,
    TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
    // 이미 열린 에디터가 있는지 확인
    if (OpenedEditorInstance.IsValid())
    {
        // 다이얼로그로 경고 메시지 표시
        FText Title = FText::FromString(TEXT("Editor Open Failed"));
        FText Message = FText::FromString(TEXT("MapPreset Editor is already open. Please close the existing editor before opening a new one."));
        FMessageDialog::Open(EAppMsgType::Ok, Message, Title); // [3]
        return;
    }
    
    const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

    for (auto& Object : InObjects)
    {
        if (UMapPreset* MapPreset = Cast<UMapPreset>(Object))
        {
            TSharedRef<FMapPresetEditorToolkit> NewEditor(new FMapPresetEditorToolkit());
            NewEditor->InitEditor(Mode, EditWithinLevelEditor, MapPreset);
            OpenedEditorInstance = NewEditor;
        }
    }
}
