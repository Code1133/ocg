// Fill out your copyright notice in the Description page of Project Settings.


#include "OCGMaterialEditTool.h"

#include "FileHelpers.h"
#include "Materials/MaterialExpressionSetMaterialAttributes.h"


#if WITH_EDITOR
#include "Materials/MaterialExpressionLandscapeLayerBlend.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/Material.h"
#include "Materials/MaterialFunctionInterface.h"
#include "Materials/MaterialExpressionFunctionInput.h"
#endif

OCGMaterialEditTool::OCGMaterialEditTool()
{
}

OCGMaterialEditTool::~OCGMaterialEditTool()
{
}

void OCGMaterialEditTool::InsertMaterialFunctionIntoMaterial(UMaterial* TargetMaterial,
	 TArray<UMaterialFunctionInterface*> FuncToInsert)
{
#if WITH_EDITOR
	if (!TargetMaterial || FuncToInsert.IsEmpty()) return;

	// 에디터용 트랜잭션(Undo/Redo) 지원
	const FScopedTransaction Transaction(FText::FromString(TEXT("Reset and Insert Multiple Material Functions")));
	
	// 에디터용 트랜잭션 지원
	TargetMaterial->Modify();

	// Expressions 배열에서 기존 Blend 노드 찾기
	// 삭제할 Blend 노드와 함수 호출 노드를 담을 배열
	TArray<UMaterialExpression*> NodesToDelete;
	UMaterialExpressionLandscapeLayerBlend* BlendNode = nullptr;
	
	// TargetMaterial에서 "MainTiling" 파라미터 노드 검색
	UMaterialExpression* TilingParamNode = nullptr;
	const FName TilingParamName = FName("MainTiling");
	for (UMaterialExpression* Expr : TargetMaterial->GetExpressions())
	{
		if (!BlendNode) // Blend 노드는 첫 번째 것 하나만 찾습니다.
		{
			BlendNode = Cast<UMaterialExpressionLandscapeLayerBlend>(Expr);
		}

		// 1-1) 모든 Blend 노드와 함수 호출 노드를 삭제 대상으로 지정
		if (Cast<UMaterialExpressionMaterialFunctionCall>(Expr))
		{
			NodesToDelete.Add(Expr);
		}
		
		if (!TilingParamNode)
		{
			if (UMaterialExpressionParameter* ScalarParam = Cast<UMaterialExpressionParameter>(Expr))
			{
				if (ScalarParam->GetParameterName() == TilingParamName)
				{
					TilingParamNode = ScalarParam;
				}
			}
		}
	}

	// 2-2) (★핵심 변경★) Blend 노드의 기존 레이어를 모두 비웁니다.
	BlendNode->Modify();
	BlendNode->Layers.Empty(); // 노드를 삭제하는 대신, 내부 레이어 배열만 초기화합니다.
	
	for (UMaterialExpression* Node : NodesToDelete)
	{
		TargetMaterial->GetExpressionCollection().RemoveExpression(Node);
	}
	
	// 2-1) Blend 노드가 없다면 새로 생성하고 최종 출력에 연결
	if (!BlendNode)
	{
		BlendNode = NewObject<UMaterialExpressionLandscapeLayerBlend>(TargetMaterial);
		TargetMaterial->GetExpressionCollection().AddExpression(BlendNode);

		// 새로 생성했으므로, 머티리얼의 최종 출력 핀에 연결해줍니다.
		// (MaterialAttributes 모드를 우선으로 가정)
		if (TargetMaterial->bUseMaterialAttributes)
		{
			UMaterialExpression* ResultNode = GetResultNodeFromMaterialAttributes(TargetMaterial);
			TargetMaterial->GetExpressionCollection().RemoveExpression(ResultNode);
			TargetMaterial->GetEditorOnlyData()->MaterialAttributes.Expression = BlendNode;
		}
	}
	
	// 머티리얼 함수 호출 노드 생성
	int32 NodeIndex = 0;
	float TotalYOfFuncNodes = 0.0f;
	for (UMaterialFunctionInterface* MaterialFunction : FuncToInsert)
	{
		if (!MaterialFunction) continue; // 배열에 null 포인터가 있을 경우 건너뜀
		
		UMaterialExpressionMaterialFunctionCall* FuncNode = NewObject<UMaterialExpressionMaterialFunctionCall>(TargetMaterial);
		FuncNode->SetMaterialFunction(MaterialFunction);
		FuncNode->Desc = MaterialFunction->GetName();
		TargetMaterial->GetExpressionCollection().AddExpression(FuncNode);

		// 5) 찾은 "MainTiling" 파라미터를 FuncNode의 입력에 연결
		//    (여기서는 FuncNode의 입력 핀 이름이 "Tiling"이라고 가정합니다)
		if (TilingParamNode)
		{
			const FName InputNameToFind = FName("Tiling"); // 연결할 함수의 입력 핀 이름

			// 함수의 모든 입력을 순회하며 이름이 "Tiling"인 입력을 찾습니다.
			TArray<FFunctionExpressionInput>& FuncInputs = FuncNode->FunctionInputs;
			for (FFunctionExpressionInput& Input : FuncInputs)
			{
				if (Input.ExpressionInput)
				{
					// ExpressionInput이 가리키는 실제 입력 노드의 InputName을 비교합니다.
					if (Input.ExpressionInput->InputName == InputNameToFind)
					{
						// 이름을 찾았으면, 이 입력의 FExpressionInput 구조체에 파라미터 노드를 연결합니다.
						Input.Input.Expression = TilingParamNode;
						// 원하는 입력을 찾았으므로 더 이상 순회할 필요가 없습니다.
						break;
					}
				}
			}
		}
	
		// Blend 노드에 새 레이어를 '생성'하고 FuncNode를 연결
		FLayerBlendInput NewLayer;
		NewLayer.LayerName = FName(*MaterialFunction->GetName());
		NewLayer.BlendType = ELandscapeLayerBlendType::LB_WeightBlend;
		NewLayer.LayerInput.Expression = FuncNode;
		NewLayer.PreviewWeight = 1.0f;
		BlendNode->Layers.Add(NewLayer);
	
		// D) 노드 위치 지정 (세로로 정렬하여 겹치지 않게 함)
		const int32 CurrentY = 200 + (NodeIndex * 150);
		FuncNode->MaterialExpressionEditorX = 200;
		FuncNode->MaterialExpressionEditorY = CurrentY;
		
		TotalYOfFuncNodes += CurrentY;
		NodeIndex++;
	}
	
	// 3-2) 노드 위치 지정 및 에셋 변경사항 커밋
	if (NodeIndex > 0)
	{
		BlendNode->MaterialExpressionEditorX = 600;
		BlendNode->MaterialExpressionEditorY = TotalYOfFuncNodes / NodeIndex;
	}

	TargetMaterial->PostEditChange();
	TargetMaterial->MarkPackageDirty();
	SaveMaterialAsset(TargetMaterial);
#endif
}

UMaterialExpression* OCGMaterialEditTool::GetResultNodeFromMaterialAttributes(UMaterial* TargetMaterial)
{
#if WITH_EDITOR
	if (!TargetMaterial)
	{
		return nullptr;
	}

	// 머티리얼이 "Material Attributes" 모드를 사용하는지 확인합니다.
	if (TargetMaterial->bUseMaterialAttributes)
	{
		// MaterialAttributes 입력 핀에 연결된 표현식(노드)을 가져옵니다.
		UMaterialExpression* ResultNode = TargetMaterial->GetEditorOnlyData()->MaterialAttributes.Expression;

		if (ResultNode)
		{
			UE_LOG(LogTemp, Log, TEXT("Material Attributes에 연결된 노드를 찾았습니다: %s"), *ResultNode->GetName());
			return ResultNode;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Material Attributes 핀에 연결된 노드가 없습니다."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("이 머티리얼은 Material Attributes 모드를 사용하지 않습니다."));
	}

	return nullptr;
#endif
}

void OCGMaterialEditTool::SaveMaterialAsset(UMaterial* TargetMaterial)
{
#if WITH_EDITOR
	if (!TargetMaterial) return;

	// ① 트랜잭션 및 Modify/MarkPackageDirty() 이후에 호출
	UPackage* Package = TargetMaterial->GetOutermost();
	if (!Package) return;

	// ② 저장할 패키지 목록 생성
	TArray<UPackage*> PackagesToSave;
	PackagesToSave.Add(Package);

	// ③ 디스크에 저장 (체크아웃 & 자동 저장)
	// bCheckDirty=false: 더티 여부 상관없이 저장
	// bPromptToSave=false: 별도 다이얼로그 없이 바로 저장
	FEditorFileUtils::PromptForCheckoutAndSave(
		PackagesToSave,
		/*bCheckDirty=*/ false,
		/*bPromptToSave=*/ false
	);
#endif
}

TArray<FName> OCGMaterialEditTool::ExtractLandscapeLayerName(UMaterial* TargetMaterial)
{
#if WITH_EDITOR
	TArray<FName> LayerNames;
	if (!TargetMaterial) return LayerNames;

	// 사용 중인 모든 표현식을 수집합니다.
	TSet<UMaterialExpression*> UsedExpressions;
	CollectUsedExpressions(TargetMaterial, UsedExpressions);

	UMaterialExpressionLandscapeLayerBlend* UsedBlendNode = nullptr;
	
	// 머티리얼이 "Material Attributes" 모드를 사용하는지 확인합니다.
	for (UMaterialExpression* Expr : TargetMaterial->GetExpressions())
	{
		if (UsedBlendNode == nullptr)
		{
			UMaterialExpressionLandscapeLayerBlend* CurBlendNode = Cast<UMaterialExpressionLandscapeLayerBlend>(Expr);
			if (CurBlendNode && UsedExpressions.Contains(CurBlendNode))
			{
				UsedBlendNode =  CurBlendNode;
				break;
			}
		}
	}

	if (UsedBlendNode == nullptr)
		return LayerNames;
	
	for (FLayerBlendInput LayerInput : UsedBlendNode->Layers)
	{
		LayerNames.Add(LayerInput.LayerName);
	}

	return LayerNames;
#endif
}

void OCGMaterialEditTool::CollectUsedExpressions(UMaterial* TargetMaterial,	TSet<UMaterialExpression*>& OutUsedExpressions)
{
	if (!TargetMaterial)
	{
		return;
	}

	// 탐색을 위한 큐(Queue)와 이미 방문한 표현식을 추적하기 위한 셋(Set)
	TQueue<UMaterialExpression*> ExpressionsToProcess;

	// 1. 탐색 시작점(Seed) 설정: 머티리얼의 최종 출력 속성들
	//    머티리얼의 모든 속성 입력을 순회하며 시작 노드를 큐에 추가합니다.
	for (int32 PropertyId = 0; PropertyId < MP_MAX; ++PropertyId)
	{
		const FExpressionInput* Input = TargetMaterial->GetExpressionInputForProperty(static_cast<EMaterialProperty>(PropertyId));
		if (Input)
		{
			// GetTracedInput()을 사용하여 Reroute 노드를 건너뛰고 실제 소스 표현식을 가져옵니다.
			UMaterialExpression* RootExpr = Input->GetTracedInput().Expression;
			if (RootExpr && !OutUsedExpressions.Contains(RootExpr))
			{
				OutUsedExpressions.Add(RootExpr);
				ExpressionsToProcess.Enqueue(RootExpr);
			}
		}
	}

	// 2. 역방향 탐색 (너비 우선 탐색 - BFS)
	UMaterialExpression* CurrentExpr = nullptr;
	while (ExpressionsToProcess.Dequeue(CurrentExpr))
	{
		// FExpressionInputIterator를 사용하여 현재 표현식의 모든 입력을 순회합니다.
		for (FExpressionInputIterator It{CurrentExpr}; It; ++It)
		{
			// 반복자가 자동으로 Reroute를 포함한 모든 유효한 입력을 찾아줍니다.
			UMaterialExpression* InputExpr = It.Expression;
			if (InputExpr && !OutUsedExpressions.Contains(InputExpr))
			{
				OutUsedExpressions.Add(InputExpr);
				ExpressionsToProcess.Enqueue(InputExpr);
			}
		}
	}
}

void OCGMaterialEditTool::AddAttributeInput(const FExpressionInput& Input,
	TSet<UMaterialExpression*>& OutUsedExpressions, TArray<UMaterialExpression*>& ExpressionsToProcess)
{
	// GetTracedInput()은 Reroute 및 Named Reroute 노드를 자동으로 해석하여 실제 소스 표현식을 반환합니다.
	if (UMaterialExpression* RootExpr = Input.GetTracedInput().Expression)
	{
		if (!OutUsedExpressions.Contains(RootExpr))
		{
			OutUsedExpressions.Add(RootExpr);
			ExpressionsToProcess.Add(RootExpr);
		}
	}
}
