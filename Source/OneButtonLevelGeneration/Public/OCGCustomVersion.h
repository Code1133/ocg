// Copyright (c) 2025-2026 Code1133. All rights reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * OCG 플러그인 에셋(UMapPreset 등)의 직렬화 포맷 버전
 * 새 버전을 추가할 때는 VersionPlusOne 바로 위에 enumerator를 추가해 주세요.
 */
struct FOCGCustomVersion
{
	FOCGCustomVersion() = delete;

	enum Type : int32
	{
		// 이 커스텀 버전이 도입되기 전 저장된 에셋 (~v1.2.0)
		BeforeCustomVersionWasAdded = 0,

		// FCustomVersion 인프라 최초 도입 (v2.0.0)
		InitialVersion = 1,

		// MapPreset 평면 필드를 nested settings struct로 묶고 마이그레이션 (v2.0.0)
		NestedSettings,

		// ^^^ 새 버전은 이 줄 위에 추가 ^^^
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};

	// 다른 플러그인과 충돌하지 않는 고유 GUID
	const static FGuid GUID;
};
