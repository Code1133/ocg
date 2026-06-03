// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once

#include "CoreMinimal.h"

class UMaterial;
class UMaterialFunction;

/**
 * 랜드스케이프 머티리얼 편집 유틸리티 모음
 * LandscapeLayerBlend 노드 구성, 머티리얼 함수 삽입, 레이어 이름 추출 등
 * 에디터 전용 머티리얼 그래프 조작을 담당합니다. 모든 함수는 정적(static)입니다.
 */
struct ONEBUTTONLEVELGENERATION_API FOCGMaterialEditTool
{
	FOCGMaterialEditTool() = delete;

	/**
	 * TargetMaterial의 기존 LandscapeLayerBlend 레이어와 머티리얼 함수 호출 노드를 모두 정리한 뒤,
	 * FuncToInsert의 각 머티리얼 함수를 새 레이어로 추가하고 "MainTiling" 파라미터를 각 함수의 "Tiling" 입력에 연결합니다.
	 * 작업 후 PostEditChange 및 에셋 저장까지 수행합니다. (에디터 Undo/Redo 트랜잭션 지원)
	 * @param TargetMaterial 편집 대상 머티리얼
	 * @param FuncToInsert 레이어로 삽입할 머티리얼 함수 목록
	 */
	static void InsertMaterialFunctionIntoMaterial(UMaterial* TargetMaterial, TArray<UMaterialFunctionInterface*> FuncToInsert);

	/**
	 * Material Attributes 모드 머티리얼의 최종 출력(MaterialAttributes 핀)에 연결된 표현식 노드를 반환합니다.
	 * @return 연결된 노드. 해당 모드가 아니거나 연결이 없으면 nullptr.
	 */
	static UMaterialExpression* GetResultNodeFromMaterialAttributes(UMaterial* TargetMaterial);

	/** TargetMaterial이 속한 패키지를 디스크에 저장합니다. (더티 검사/프롬프트 없이 즉시 저장) */
	static void SaveMaterialAsset(UMaterial* TargetMaterial);

	/**
	 * TargetMaterial의 첫 번째 LandscapeLayerBlend 노드에서 모든 레이어 이름을 추출합니다.
	 * @return 레이어 이름 배열. Blend 노드가 없으면 빈 배열.
	 */
	static TArray<FName> ExtractLandscapeLayerName(UMaterial* TargetMaterial);
};
