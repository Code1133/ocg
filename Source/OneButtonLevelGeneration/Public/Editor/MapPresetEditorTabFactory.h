#pragma once

#include "CoreMinimal.h"

class FMapPresetEditorToolkit;
class FMapPresetEditorTabFactory
{
public:
	static const FName TabId;

	static TSharedRef<SWidget> CreateTabBody(TSharedRef<FMapPresetEditorToolkit> InToolkit);
};
