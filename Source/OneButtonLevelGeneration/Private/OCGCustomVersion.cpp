// Copyright (c) 2025-2026 Code1133. All rights reserved.

#include "OCGCustomVersion.h"

#include "Serialization/CustomVersion.h"

const FGuid FOCGCustomVersion::GUID(0xA451E3D8, 0x449D57F9, 0x8D0AFDBA, 0x890EBDF7);

// 엔진 커스텀 버전 레지스트리에 등록 "OCGVer"는 디버깅용 friendly name
FCustomVersionRegistration GRegisterOCGCustomVersion(FOCGCustomVersion::GUID, FOCGCustomVersion::LatestVersion, TEXT("OCGVer"));
