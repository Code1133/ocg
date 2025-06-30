// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class ONEBUTTONLEVELGENERATION_API OCGFileUtils
{
public:
	OCGFileUtils();
	~OCGFileUtils();

	static bool EnsureContentDirectoryExists(const FString& InPackagePath);
};
