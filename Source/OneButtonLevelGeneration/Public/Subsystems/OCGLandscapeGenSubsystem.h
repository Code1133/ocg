// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "OCGLandscapeGenSubsystem.generated.h"

/**
 * 생성된 맵 데이터를 Landscape 액터에 적용하고 바이옴 레이어 마무리 및 맵 텍스처 내보내기를 담당하는 에디터 서브시스템
 */
UCLASS()
class ONEBUTTONLEVELGENERATION_API UOCGLandscapeGenSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
};
