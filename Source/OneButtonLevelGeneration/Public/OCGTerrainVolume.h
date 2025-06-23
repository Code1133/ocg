// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OCGTerrainVolume.generated.h"

class UOCGHierarchyDataAsset;
class UBoxComponent;
class UPCGComponent;

UCLASS()
class ONEBUTTONLEVELGENERATION_API AOCGTerrainVolume : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AOCGTerrainVolume();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	UFUNCTION(BlueprintCallable, Category = "Utilities")
	void AdjustVolumeToBoundsOfActor(const AActor* TargetActor);
	
	UBoxComponent* GetBoxComponent() const { return BoxComponent; }
	UPCGComponent* GetPCGComponent() const { return PCGComponent; }
private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess="true"))
	TObjectPtr<UOCGHierarchyDataAsset> DataAsset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess="true"))
	bool DebugPoints;
private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> BoxComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPCGComponent> PCGComponent;
};
