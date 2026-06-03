// Copyright (c) 2025-2026 Code1133. All rights reserved.
#include "Tests/Benchmark/FOCGBenchmarkRunner.h"

#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

FOCGBenchmarkEntry FOCGBenchmarkRunner::Run(FName PresetName, TFunctionRef<void()> GenerateFunc)
{
	const double StartSeconds = FPlatformTime::Seconds();
	GenerateFunc();
	const double EndSeconds = FPlatformTime::Seconds();

	FOCGBenchmarkEntry Entry;
	Entry.PresetName = PresetName;
	Entry.TotalMs = (EndSeconds - StartSeconds) * 1000.0;
	Entry.Timestamp = FDateTime::Now().ToString(TEXT("%Y-%m-%dT%H:%M:%S"));
	return Entry;
}

void FOCGBenchmarkRunner::AppendToCSV(const FOCGBenchmarkEntry& Entry)
{
	const FString FilePath = GetCSVPath();
	EnsureHeaderWritten(FilePath);

	const FString Row = FString::Printf(TEXT("%s,%.3f,%s\n"), *Entry.PresetName.ToString(), Entry.TotalMs, *Entry.Timestamp);
	FFileHelper::SaveStringToFile(Row, *FilePath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_Append);
}

FString FOCGBenchmarkRunner::GetCSVPath()
{
	const FString Dir = FPaths::ProjectSavedDir() / TEXT("OCG_Bench");
	IFileManager::Get().MakeDirectory(*Dir, true);

	const FString DateStr = FDateTime::Now().ToString(TEXT("%Y%m%d"));
	return Dir / FString::Printf(TEXT("OCGBench_%s.csv"), *DateStr);
}

void FOCGBenchmarkRunner::EnsureHeaderWritten(const FString& FilePath)
{
	if (IFileManager::Get().FileSize(*FilePath) > 0)
	{
		return;
	}

	FFileHelper::SaveStringToFile(TEXT("PresetName,TotalMs,Timestamp\n"), *FilePath);
}
