#include "Editor/MapPresetAssetTypeActions.h"
#include "Editor/MapPresetEditorToolkit.h"
#include "Data/MapPreset.h"

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
    const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

    for (auto& Object : InObjects)
    {
        if (UMapPreset* MapPreset = Cast<UMapPreset>(Object))
        {
            // 새 에디터 툴킷 인스턴스를 생성합니다.
            TSharedRef<FMapPresetEditorToolkit> NewEditor(new FMapPresetEditorToolkit());
            NewEditor->InitEditor(Mode, EditWithinLevelEditor, MapPreset);
        }
    }
}
