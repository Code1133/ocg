// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
struct ONEBUTTONLEVELGENERATION_API FOCGFileUtils
{
public:
	FOCGFileUtils();
	~FOCGFileUtils();

	static bool EnsureContentDirectoryExists(const FString& InPackagePath);
};
