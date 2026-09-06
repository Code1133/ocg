// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Strategies/OCGErosionStrategyBase.h"
#include "OCGTestStrategies.generated.h"

/**
 * 테스트 전용 침식 전략
 */
UCLASS()
class UOCGTestErosionStrategy : public UOCGErosionStrategyBase
{
	GENERATED_BODY()

public:
	virtual void ApplyErosion(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer) override
	{
		bApplied = true;
	}

	bool bApplied = false;
};
