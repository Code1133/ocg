// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "PCGSettings.h"
#include "OCGPointAngleFilter.generated.h"


/**
 *
 */
UCLASS(MinimalAPI, BlueprintType, ClassGroup = (Procedural))
class UOCGPointAngleFilterSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
	//~Begin UPCGSettings interface
#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const override { return FName(TEXT("PointAngleFilter")); }
	virtual FText GetDefaultNodeTitle() const override { return NSLOCTEXT("OCGPointAngleFilterSettings", "NodeTitle", "Slope Angle Filter"); }
	virtual EPCGSettingsType GetType() const override { return EPCGSettingsType::Filter; }
#endif

protected:
	virtual TArray<FPCGPinProperties> InputPinProperties() const override { return DefaultPointInputPinProperties(); }
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override { return DefaultPointOutputPinProperties(); }
	virtual FPCGElementPtr CreateElement() const override;
	//~End UPCGSettings interface

public:
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Settings",
		meta = (ClampMin = "0.0", ClampMax = "90.0", UIMin = "0.0", UIMax = "90.0", Units = "Degrees", PCG_Overridable)
	)
	float MinAngle = 0.0f;

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Settings",
		meta = (ClampMin = "0.0", ClampMax = "90.0", UIMin = "0.0", UIMax = "90.0", Units = "Degrees", PCG_Overridable)
	)
	float MaxAngle = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (PCG_Overridable))
	bool bInvertFilter = false;
};


class FOCGPointAngleFilterElement : public IPCGElement
{
protected:
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
};
