// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "OCGEditorSubsystem.generated.h"

/**
 * OCG의 최상위 에디터 서브시스템
 * 생성 및 초기화 에디터 액션을 소유하며, 다른 OCG 서브시스템에 작업을 위임합니다.
 */
UCLASS()
class ONEBUTTONLEVELGENERATION_API UOCGEditorSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
};
