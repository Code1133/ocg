// Copyright (c) 2025 Code1133. All rights reserved.

#include "PCG/Elements/OCGPointHeightFilter.h"
#include "PCGContext.h"
#include "Data/PCGPointData.h"
#include "Helpers/PCGAsync.h"


FPCGElementPtr UOCGPointHeightFilterSettings::CreateElement() const
{
	return MakeShared<FOCGPointHeightFilterElement>();
}

bool FOCGPointHeightFilterElement::ExecuteInternal(FPCGContext* Context) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FOCGPointHeightFilterElement::Execute);

	const UOCGPointHeightFilterSettings* Settings = Context->GetInputSettings<UOCGPointHeightFilterSettings>();
	check(Settings);

	const float MinHeight = FMath::Min(Settings->MinHeight, Settings->MaxHeight);
	const float MaxHeight = FMath::Max(Settings->MinHeight, Settings->MaxHeight);

	TArray<FPCGTaggedData> Inputs = Context->InputData.GetInputsByPin(PCGPinConstants::DefaultInputLabel);
	TArray<FPCGTaggedData>& Outputs = Context->OutputData.TaggedData;

	for (const FPCGTaggedData& Input : Inputs)
	{
		const UPCGPointData* OriginalData = Cast<UPCGPointData>(Input.Data);
		if (!OriginalData)
		{
			continue;
		}

		// Set Filtered Output
		FPCGTaggedData& FilteredOutput = Outputs.Emplace_GetRef(Input);
		FilteredOutput.Pin = PCGPinConstants::DefaultOutputLabel;

		UPCGPointData* FilteredData = FPCGContext::NewObject_AnyThread<UPCGPointData>(Context);
		FilteredData->InitializeFromData(OriginalData);
		FilteredOutput.Data = FilteredData;

		const TArray<FPCGPoint>& OriginalPoints = OriginalData->GetPoints();
		TArray<FPCGPoint>& FilteredPoints = FilteredData->GetMutablePoints();

		FPCGAsync::AsyncPointProcessing(Context, OriginalPoints.Num(), FilteredPoints, [&OriginalPoints, MinHeight, MaxHeight, bInvertFilter = Settings->bInvertFilter](int32 Index, FPCGPoint& OutPoint) -> bool
		{
			const FPCGPoint& Point = OriginalPoints[Index];
			const float Height = Point.Transform.GetTranslation().Z;

			// Filter points based on the Invert option
			if ((MinHeight <= Height && Height <= MaxHeight) == !bInvertFilter)
			{
				OutPoint = Point;
				return true;
			}
			return false;
		});
	}
	return true;
}
