// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BasePickup.h"
#include "HealthPickup.generated.h"

UCLASS()
class FIRSTFPS_API AHealthPickup : public ABasePickup
{
	GENERATED_BODY()
public:
	AHealthPickup();
protected:
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;
private:
	UPROPERTY(EditAnywhere)
	float HealAmount = 60.f; // 拾取该道具时, Character能恢复的生命值总量
	UPROPERTY(EditAnywhere)
	float HealTime = 5.f;	 // 治疗时间: 恢复 HealAmount 生命值所需时间
};
