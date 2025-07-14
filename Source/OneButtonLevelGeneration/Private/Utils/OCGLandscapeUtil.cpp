// Copyright (c) 2025 Code1133. All rights reserved.


#include "Utils/OCGLandscapeUtil.h"

#if WITH_EDITOR
#include "Landscape.h"
#include "LandscapeEdit.h"
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
			FLandscapeEditDataInterface LandscapeEdit(Info);
			
			OutWidth = ExportRegion.Width() + 1;
			OutHeight = ExportRegion.Height() + 1;
			
			OutHeightMap.AddZeroed(OutWidth * OutHeight);
			LandscapeEdit.GetHeightDataFast(ExportRegion.Min.X, ExportRegion.Min.Y, ExportRegion.Max.X, ExportRegion.Max.Y, OutHeightMap.GetData(), 0);
		}
	}
#endif

}

void OCGLandscapeUtil::ApplyWeightMap(ALandscape* InLandscape, int32 InLayerIndex, const TArray<uint16>& NewWeightMap)
{
#if WITH_EDITOR
	if (InLandscape)
	{
		FGuid CurrentLayerGuid = InLandscape->GetLayerConst(0)->Guid;
		
		ULandscapeInfo* LandscapeInfo = InLandscape->GetLandscapeInfo();
		if (!LandscapeInfo) return;

		// TMap<FName, FLandscapeTargetLayerSettings> TargetLayers = InLandscape->GetTargetLayers();
		// TArray<FName> LayerNames;
		// for (auto Layer : LandscapeInfo->Layers)
		// {
		// 	LayerNames.Add(Layer.GetLayerName());
		// }
		//
		// FLandscapeTargetLayerSettings TargetLayer;
		// if (InLayerIndex < LayerNames.Num())
		// {
		// 	TargetLayer = TargetLayers.FindChecked(LayerNames[InLayerIndex]);
		// }
		//
		// ULandscapeLayerInfoObject* LayerInfo = TargetLayer.LayerInfoObj;

		ULandscapeLayerInfoObject* LayerInfo = LandscapeInfo->Layers[InLayerIndex].LayerInfoObj;
		
		if (!LayerInfo) return;

		FIntRect Region;
		if (LandscapeInfo->GetLandscapeExtent(Region))
		{
			Region.Max.X += 1;
			Region.Max.Y += 1;
			
			FScopedSetLandscapeEditingLayer Scope(InLandscape, CurrentLayerGuid, [InLandscape]
			{
				check(InLandscape);
				InLandscape->RequestLayersContentUpdate(ELandscapeLayerUpdateMode::Update_Weightmap_All);
			});

			FAlphamapAccessor<false, false> AlphamapAccessor(LandscapeInfo, LayerInfo);
			int32 Width = Region.Width() + 1;
			int32 Height = Region.Height() + 1;
			int32 NumPixels = Width * Height;
			
			TArray<uint8> OriginWeightData;
			OriginWeightData.AddZeroed(Width * Height);
			
			AlphamapAccessor.GetDataFast(Region.Min.X, Region.Min.Y, Region.Max.X - 1, Region.Max.Y - 1, OriginWeightData.GetData());

			TArray<uint8> TargetWeightData;
			TargetWeightData.AddZeroed(Width * Height);

			for (int32 i = 0; i < NumPixels; ++i)
			{
				float Origin = OriginWeightData[i] / 255.f;
				float New = 0;
				if (i < NewWeightMap.Num())
				{
					New =  FMath::Clamp(NewWeightMap[i] / 65535.f, 0.f, 1.f);
				}

				float Final = FMath::Clamp(Origin + New, 0.f, 1.f);    // 더하기 방식 블렌드
				TargetWeightData[i] = static_cast<uint8>(Final * 255.f);
			}

			AlphamapAccessor.SetData(Region.Min.X, Region.Min.Y, Region.Max.X - 1, Region.Max.Y - 1, TargetWeightData.GetData(), ELandscapeLayerPaintingRestriction::None);

			LandscapeInfo->ForceLayersFullUpdate();

			InLandscape->ReregisterAllComponents();
		}
	}
#endif
}
