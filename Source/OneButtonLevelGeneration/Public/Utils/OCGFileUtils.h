// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * 콘텐츠(Content) 디렉터리 관련 파일 시스템 유틸리티 모음
 */
struct ONEBUTTONLEVELGENERATION_API FOCGFileUtils
{
	FOCGFileUtils() = delete;

	/**
	 * 주어진 언리얼 패키지 경로(/Game/...)에 대응하는 Content 디렉터리가 존재하도록 보장합니다.
	 * 디렉터리가 없으면 생성하고, 이미 존재하면 아무 작업도 하지 않습니다.
	 * @param InPackagePath /Game/ 접두사를 포함한 언리얼 패키지 경로
	 * @return 디렉터리가 이미 존재하거나 생성에 성공하면 true
	 */
	static bool EnsureContentDirectoryExists(const FString& InPackagePath);
};
