// Copyright (c) 2025 Code1133. All rights reserved.

#include "OCGDeveloperSettings.h"

#include "OCGLog.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"

TArray<FString> UOCGDeveloperSettings::FindUnresolvedAssets() const
{
	const IAssetRegistry& AssetRegistry =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

	// 타입이 SoftObjectPtr인 Property의 실제 에셋의 존재여부를 확인
	TArray<FString> Failures;
	for (TFieldIterator<FSoftObjectProperty> It(GetClass()); It; ++It)
	{
		const FSoftObjectPtr& Value = *It->ContainerPtrToValuePtr<FSoftObjectPtr>(this);

		// 비어 있는 값은 통과. 값이 있는데 에셋이 없을 때만 실패로 간주.
		if (Value.IsNull())
		{
			continue;
		}

		const FSoftObjectPath Path = Value.ToSoftObjectPath();
		if (!AssetRegistry.GetAssetByObjectPath(Path).IsValid())
		{
			Failures.Add(FString::Printf(TEXT("%s = %s"), *It->GetName(), *Path.ToString()));
		}
	}

	return Failures;
}

void UOCGDeveloperSettings::ValidateConfiguredAssets() const
{
	const TArray<FString> Failures = FindUnresolvedAssets();
	if (Failures.IsEmpty())
	{
		return;
	}

	UE_LOG(LogOCGModule, Error,
		TEXT("Configured asset(s) failed to resolve:\n  %s\nCheck Project Settings > One Button Level Generation."),
		*FString::Join(Failures, TEXT("\n  ")));
}