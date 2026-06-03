// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "Modules/ModuleInterface.h"

class IAssetTypeActions;

/**
 * Main module class for the One Button Level Generation plugin.
 * - Slate style (ClassIcon/ClassThumbnail.MapPreset) 등록
 * - FMapPresetAssetTypeActions를 AssetTools에 등록
 * - 나머지 Editor 기능(Toolbar, Console command)은 UOCGEditorSubsystem이 담당.
 */
class FOneButtonLevelGenerationModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TArray<TSharedPtr<IAssetTypeActions>> RegisteredAssetTypeActions;
};
