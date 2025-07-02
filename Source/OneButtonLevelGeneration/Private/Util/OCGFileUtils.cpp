// Copyright (c) 2025 Code1133. All rights reserved.

#if WITH_EDITOR
#include "Util/OCGFileUtils.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Logging/LogMacros.h"
#endif

FOCGFileUtils::FOCGFileUtils()
{
}

FOCGFileUtils::~FOCGFileUtils()
{
}

bool FOCGFileUtils::EnsureContentDirectoryExists(const FString& InPackagePath)
{
#if WITH_EDITOR
	// 1. /Game/ 접두사를 제거하여 Content 폴더 내의 상대 경로를 만듭니다.
	FString RelativeContentPath = InPackagePath;
	if (RelativeContentPath.StartsWith(TEXT("/Game/")))
	{
		RelativeContentPath = RelativeContentPath.RightChop(6); // "/Game/" 제거
	}

	// 2. 전체 물리적 경로를 구성합니다.
	const FString FullPath = FPaths::Combine(FPaths::ProjectContentDir(), RelativeContentPath);
    
	// 3. FPaths::GetPath()를 사용하여 파일 이름을 제외한 디렉토리 경로만 추출합니다.
	const FString DirectoryPath = FPaths::GetPath(FullPath);
	
	IFileManager& FileManager = IFileManager::Get();

	// 4. 디렉토리가 이미 존재하는지 확인합니다.
	if (FileManager.DirectoryExists(*DirectoryPath))
	{
		return true;
	}
	else
	{
		// 5. 디렉토리가 없으면 생성합니다.
		return FileManager.MakeDirectory(*DirectoryPath, true);
	}
#endif
}
