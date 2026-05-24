// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"

struct FOCGBenchmarkEntry
{
	FName PresetName;
	double TotalMs;
	FString Timestamp; // ISO 8601 형식: "YYYY-MM-DDTHH:MM:SS"
};

/**
 * 생성 함수(Generation)의 실제 실행 시간을 측정하고,
 * 그 결과를 'Saved/OCG_Bench/' 경로에 CSV 파일로 기록합니다.
 *
 * 사용 예시:
 *   FOCGBenchmarkRunner::Run(TEXT("Default"), [&]{ MyGenerateFunc(); });
 */
class FOCGBenchmarkRunner
{
public:
	/**
	 * GenerateFunc의 실행 시간을 측정하고 결과를 구조체로 반환합니다.
	 *
	 * @param PresetName CSV 파일에 기록할 프리셋 식별자
	 * @param GenerateFunc 실행 시간을 측정할 함수(또는 콜백)
	 * @return 측정된 시간과 타임스탬프 정보가 포함된 벤치마크 결과
	 */
	static FOCGBenchmarkEntry Run(FName PresetName, TFunctionRef<void()> GenerateFunc);

	/**
	 * 벤치마크 결과를 'Saved/OCG_Bench/OCGBench_YYYYMMDD.csv' 파일에 한 줄(Row) 추가합니다.
	 * @param Entry Run() 함수가 반환한 측정 결과
	 */
	static void AppendToCSV(const FOCGBenchmarkEntry& Entry);

private:
	static FString GetCSVPath();
	static void EnsureHeaderWritten(const FString& FilePath);
};
