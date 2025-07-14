// Copyright (c) 2025 Code1133. All rights reserved.


#include "Utils/OCGLandscapeUtil.h"

#if WITH_EDITOR
#include "Landscape.h"
#include "LandscapeEdit.h"
#include "LandscapeEditorModule.h"
#include "LandscapeInfo.h"
#endif

OCGLandscapeUtil::OCGLandscapeUtil()
{
}

OCGLandscapeUtil::~OCGLandscapeUtil()
{
}

void OCGLandscapeUtil::ExtractHeightMap(ALandscape* InLandscape, const FGuid InGuid, int32& OutWidth, int32& OutHeight,
	TArray<uint16>& OutHeightMap)
{
#if WITH_EDITOR
	if (InLandscape)
	{
		ULandscapeInfo* Info = InLandscape->GetLandscapeInfo();
		if (!Info)
		{
			return;
		}
		
		FScopedSetLandscapeEditingLayer Scope(InLandscape, InGuid);

		FIntRect ExportRegion;
		if (Info->GetLandscapeExtent(ExportRegion))
		{
			
			ILandscapeEditorModule& LandscapeEditorModule = FModuleManager::GetModuleChecked<ILandscapeEditorModule>("LandscapeEditor");
			FLandscapeEditDataInterface LandscapeEdit(Info);
			
			OutWidth = ExportRegion.Width() + 1;
			OutHeight = ExportRegion.Height() + 1;
			
			OutHeightMap.AddZeroed(OutWidth * OutHeight);
			LandscapeEdit.GetHeightDataFast(ExportRegion.Min.X, ExportRegion.Min.Y, ExportRegion.Max.X, ExportRegion.Max.Y, OutHeightMap.GetData(), 0);
		}
	}
#endif

}
