// Copyright (c) 2025 Code1133. All rights reserved.


#include "Utils/OCGLandscapeUtil.h"

#include "Data/MapData.h"

#if WITH_EDITOR
#include "Landscape.h"
#include "LandscapeEdit.h"
#include "LandscapeInfo.h"
#if ENGINE_MINOR_VERSION > 5
#include "LandscapeEditLayer.h"
#endif
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

void OCGLandscapeUtil::AddWeightMap(ALandscape* InLandscape, int32 InLayerIndex, int32 Width, int32 Height,
	const TArray<uint16>& InHeightDiffMap, TArray<uint8>& OutOriginWeightMap)
{
#if WITH_EDITOR
	if (InLandscape)
	{
		TArray<uint8> WeightMap;
		MakeWeightMapFromHeightDiff(InHeightDiffMap, WeightMap);

		TArray<uint8> BlurredWeightMap;
		BlurWeightMap(WeightMap, BlurredWeightMap, Width, Height);

#if ENGINE_MINOR_VERSION > 5
		FGuid CurrentLayerGuid = InLandscape->GetEditLayerConst(0)->GetGuid();
#else
		FGuid CurrentLayerGuid = InLandscape->GetLayerConst(0)->Guid;
#endif
		
		ULandscapeInfo* LandscapeInfo = InLandscape->GetLandscapeInfo();
		if (!LandscapeInfo) return;

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
			int32 RegionWidth = Region.Width() + 1;
			int32 RegionHeight = Region.Height() + 1;
			int32 NumPixels = RegionWidth * RegionHeight;
			
			OutOriginWeightMap.AddZeroed(RegionWidth * RegionHeight);
			
			AlphamapAccessor.GetDataFast(Region.Min.X, Region.Min.Y, Region.Max.X - 1, Region.Max.Y - 1, OutOriginWeightMap.GetData());

			TArray<uint8> TargetWeightData;
			TargetWeightData.AddZeroed(RegionWidth * RegionHeight);

			for (int32 i = 0; i < NumPixels; ++i)
			{
				float Origin = OutOriginWeightMap[i] / 255.f;
				float New = 0;
				// if (i < WeightMap.Num())
				// {
				// 	New =  FMath::Clamp(WeightMap[i] / 255.f, 0.f, 1.f);
				// }
				if (i < BlurredWeightMap.Num())
				{
					New = FMath::Clamp(WeightMap[i] / 255.f, 0.f, 1.f);
				}

				float Final = FMath::Clamp(Origin + New, 0.f, 1.f);    // 더하기 방식 블렌드
				TargetWeightData[i] = static_cast<uint8>(Final * 255.f);
			}

			AlphamapAccessor.SetData(Region.Min.X, Region.Min.Y, Region.Max.X - 1, Region.Max.Y - 1, TargetWeightData.GetData(), ELandscapeLayerPaintingRestriction::None);

			LandscapeInfo->ForceLayersFullUpdate();

			InLandscape->ReregisterAllComponents();
			
			//OCGMapDataUtils::ExportMap(WeightMap, FIntPoint(Width, Height), TEXT("AddWeightMap.png"));
			//OCGMapDataUtils::ExportMap(BlurredWeightMap, FIntPoint(Width, Height), TEXT("BlurredWeightMap.png"));
			//OCGMapDataUtils::ExportMap(TargetWeightData, FIntPoint(RegionWidth, RegionHeight), TEXT("FinalWeightMap.png"));
		}
	}
#endif
}

void OCGLandscapeUtil::ApplyWeightMap(ALandscape* InLandscape, int32 InLayerIndex, const TArray<uint8>& WeightMap)
{
#if WITH_EDITOR
	if (WeightMap.IsEmpty())
		return;
	
	if (InLandscape)
	{
#if ENGINE_MINOR_VERSION > 5
		FGuid CurrentLayerGuid = InLandscape->GetEditLayerConst(0)->GetGuid();
#else
		FGuid CurrentLayerGuid = InLandscape->GetLayerConst(0)->Guid;
#endif
		
		ULandscapeInfo* LandscapeInfo = InLandscape->GetLandscapeInfo();
		if (!LandscapeInfo) return;

		ULandscapeLayerInfoObject* LayerInfo = LandscapeInfo->Layers[InLayerIndex].LayerInfoObj;

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
			AlphamapAccessor.SetData(Region.Min.X, Region.Min.Y, Region.Max.X - 1, Region.Max.Y - 1, WeightMap.GetData(), ELandscapeLayerPaintingRestriction::None);
			
			LandscapeInfo->ForceLayersFullUpdate();

			InLandscape->ReregisterAllComponents();

			//OCGMapDataUtils::ExportMap(WeightMap, FIntPoint(Region.Max.X - 1, Region.Max.Y - 1), TEXT("OriginWeightMap.png"));
		}
	}
#endif
}

void OCGLandscapeUtil::MakeWeightMapFromHeightDiff(const TArray<uint16>& HeightDiff, TArray<uint8>& OutWeight)
{
	OutWeight.AddZeroed(HeightDiff.Num());
	for (int32 i = 0; i < HeightDiff.Num(); ++i)
	{
		OutWeight[i] = (HeightDiff[i] > 0) ? 255 : 0;
	}
}

void OCGLandscapeUtil::BlurWeightMap(const TArray<uint8>& InWeight, TArray<uint8>& OutWeight, const int32 Width, const int32 Height)
{
	TArray<float> TempAccum;
	TempAccum.SetNumZeroed(InWeight.Num());
	for (int32 y = 0; y < Height; ++y)
	{
		for (int32 x = 0; x < Width; ++x)
		{
			for (int32 dx = -1; dx <= 1; ++dx)
			{
				for (int32 dy = -1; dy <= 1; ++dy)
				{
					int32 nx = x + dx;
					int32 ny = y + dy;

					if (nx >= 0 && nx < Width && ny >= 0 && ny < Height)
					{
						TempAccum[ny * Width + nx] += InWeight[y * Width + x] / 9.0f;
					}
				}
			}
		}
	}
	
	OutWeight.SetNumUninitialized(InWeight.Num());
	for (int32 i = 0; i < InWeight.Num(); ++i)
	{
		OutWeight[i] = FMath::Clamp(FMath::RoundToInt(TempAccum[i]), 0, 255);
	}
}
