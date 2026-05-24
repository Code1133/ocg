// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

class UMapPreset;
class AOCGLevelGenerator;

/**
 * 골든 테스트(Golden Test)를 위한 임시 월드 및 AOCGLevelGenerator 테스트 픽스처입니다.
 *
 * 생성 시 GamePreview 유형의 UWorld를 생성하고 AOCGLevelGenerator를 스폰하며,
 * 소멸 시 생성된 월드와 액터를 모두 파괴합니다.
 */
struct FOCGGoldenFixture
{
	UWorld* World;
	AOCGLevelGenerator* Generator;

	FOCGGoldenFixture();
	~FOCGGoldenFixture();

	bool IsValid() const;
};

/**
 * 콘텐츠 패키지 경로에서 UMapPreset 에셋을 로드합니다.
 *
 * 에셋이 없으면 Error 대신 Warning을 출력하고 nullptr을 반환합니다.
 * 호출자는 nullptr 반환을 테스트 실패가 아닌 '테스트 건너뛰기(Skip)' 신호로 처리해야 합니다.
 *
 * @param Test 경고 메시지를 출력할 현재 실행 중인 자동화 테스트 인스턴스
 * @param AssetPath 콘텐츠 패키지의 전체 경로 (예: "/OneButtonLevelGeneration/Test/MP_Golden_Default")
 * @return 로드된 프리셋 객체, 에셋이 존재하지 않는 경우 nullptr
 */
[[nodiscard]] UMapPreset* TryLoadPreset(FAutomationTestBase* Test, const TCHAR* AssetPath);

/**
 * HeightMapData의 CRC32 체크섬을 계산하고 ExpectedCRC와 비교합니다.
 *
 * ExpectedCRC 값이 0이면 레코드(기록) 모드로 동작합니다.
 * 이 모드에서는 계산된 CRC 값을 AddInfo를 통해 로그로 출력한 뒤 무조건 true를 반환합니다.
 * 로그에 찍힌 값을 ExpectedCRC 자리에 붙여넣어 골든 해시(기준값)를 고정할 수 있습니다.
 *
 * @param Test 에러 또는 정보 로그를 출력할 현재 실행 중인 자동화 테스트 인스턴스
 * @param HeightMapData UOCGMapGenerateComponent::GenerateMaps()에서 생성된 원본 높이값(Height) 데이터
 * @param ExpectedCRC 검증할 예상 CRC32 값. 레코드 모드로 실행하여 실제 CRC를 확인하려면 0을 전달
 * @param PresetName 로그 메시지에 표시할 프리셋의 식별 이름
 * @return 해시 값이 일치하거나 레코드 모드인 경우 true, 해시가 일치하지 않는 경우 false
 */
[[nodiscard]] bool CheckHeightMapCRC(FAutomationTestBase* Test, TArrayView<const uint16> HeightMapData, uint32 ExpectedCRC, const TCHAR* PresetName);
