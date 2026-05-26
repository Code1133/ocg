// Copyright (c) 2025 Code1133. All rights reserved.
// [DEPRECATED v2] Empty base component. Scheduled for removal.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/OCGBiomeSettings.h"
#include "OCGComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ONEBUTTONLEVELGENERATION_API
UOCGComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UOCGComponent();
};
