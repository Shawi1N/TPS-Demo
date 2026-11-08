// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BasePickup.h"
#include "ShieldPickup.generated.h"

/**
 * 
 */
UCLASS()
class FIRSTFPS_API AShieldPickup : public ABasePickup
{
	GENERATED_BODY()
protected:
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;
private:
	UPROPERTY(EditAnywhere)
	float ShieldReplenishAmount = 100.f; // 拾取该道具时, Character能恢复的护盾值总量
	UPROPERTY(EditAnywhere)
	float ShieldReplenishTime = 5.f;	 // 护盾恢复时间: 恢复 ShieldReplenishAmount 护盾值所需时间
};
