// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once
#include "AssetTypeActions_Base.h"
#include "MapPresetEditorToolkit.h"

class FOneButtonLevelGenerationStyle
{
public:
	static void Initialize();
	static void Shutdown();
	static TSharedPtr<ISlateStyle> Get();

private:
	static inline TSharedPtr<FSlateStyleSet> StyleSet = nullptr;
};

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
