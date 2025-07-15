// Copyright (c) 2025 Code1133. All rights reserved.

#include "PCG/Elements/OCGPointAngleFilter.h"
#include "PCGContext.h"
#include "Data/PCGPointData.h"
#include "Helpers/PCGAsync.h"


FPCGElementPtr UOCGPointAngleFilterSettings::CreateElement() const
{
	return MakeShared<FOCGPointAngleFilterElement>();
}

bool FOCGPointAngleFilterElement::ExecuteInternal(FPCGContext* Context) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FOCGPointAngleFilterElement::Execute);

	const UOCGPointAngleFilterSettings* Settings = Context->GetInputSettings<UOCGPointAngleFilterSettings>();
	check(Settings);

	const float MinAngle = FMath::Min(Settings->MinAngle, Settings->MaxAngle);
	const float MaxAngle = FMath::Max(Settings->MinAngle, Settings->MaxAngle);

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

		FPCGAsync::AsyncPointProcessing(Context, OriginalPoints.Num(), FilteredPoints, [&OriginalPoints, MinAngle, MaxAngle, bInvertFilter = Settings->bInvertFilter](int32 Index, FPCGPoint& OutPoint) -> bool
		{
			const FPCGPoint& Point = OriginalPoints[Index];
			const FVector PointNormal = Point.Transform.GetUnitAxis(EAxis::Z);

			const float CosAngle = FMath::Clamp(PointNormal.Dot(FVector::UpVector), 0.0f, 1.0f);
			const float Angle = FMath::RadiansToDegrees(FMath::Acos(CosAngle));

			// Filter points based on the Invert option
			if ((MinAngle <= Angle && Angle <= MaxAngle) == !bInvertFilter)
			{
				OutPoint = Point;
				return true;
			}
			return false;
		});
	}
	return true;
}
