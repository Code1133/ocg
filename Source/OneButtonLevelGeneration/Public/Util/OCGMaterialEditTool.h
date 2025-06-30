// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class UMaterial;
class UMaterialFunction;

/**
 * 
 */
class ONEBUTTONLEVELGENERATION_API OCGMaterialEditTool
{
public:
	OCGMaterialEditTool();
	~OCGMaterialEditTool();
	
	static void InsertMaterialFunctionIntoMaterial(UMaterial* TargetMaterial, TArray<UMaterialFunctionInterface*> FuncToInsert);

	static UMaterialExpression* GetResultNodeFromMaterialAttributes(UMaterial* TargetMaterial);

	static void SaveMaterialAsset(UMaterial* TargetMaterial);

	static TArray<FName> ExtractLandscapeLayerName(UMaterial* TargetMaterial);

	static void CollectUsedExpressions(UMaterial* TargetMaterial, TSet<UMaterialExpression*>& OutUsedExpressions);

	static void AddAttributeInput(const FExpressionInput& Input, TSet<UMaterialExpression*>& OutUsedExpressions, TArray<UMaterialExpression*>& ExpressionsToProcess);
};
