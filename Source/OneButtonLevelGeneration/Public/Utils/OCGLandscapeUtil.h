// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once

#include "CoreMinimal.h"

class ALandscape;
/**
 * 
 */
class ONEBUTTONLEVELGENERATION_API OCGLandscapeUtil
{
public:
	OCGLandscapeUtil();
	~OCGLandscapeUtil();

	static void ExtractHeightMap(ALandscape* InLandscape, const FGuid InGuid, int32& OutWidth, int32& OutHeight, TArray<uint16>& OutHeightMap);
};
