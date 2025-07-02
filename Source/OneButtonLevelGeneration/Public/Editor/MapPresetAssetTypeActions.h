#pragma once
#include "AssetTypeActions_Base.h"
#include "MapPresetEditorToolkit.h"

class FMapPresetAssetTypeActions : public FAssetTypeActions_Base
{
public:

	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;

	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;

	static inline TWeakPtr<FMapPresetEditorToolkit> OpenedEditorInstance;
};
