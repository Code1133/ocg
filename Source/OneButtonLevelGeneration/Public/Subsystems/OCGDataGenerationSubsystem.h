// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "OCGDataGenerationSubsystem.generated.h"

/**
 * 높이맵, 온도맵, 습도맵 생성과 바이옴 배정, 평탄화(Smoothing) 및 침식 데이터 처리를 담당하는 에디터 서브시스템
 */
UCLASS()
class ONEBUTTONLEVELGENERATION_API UOCGDataGenerationSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
};
