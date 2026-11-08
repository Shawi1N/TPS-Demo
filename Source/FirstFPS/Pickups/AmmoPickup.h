// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BasePickup.h"
#include "FirstFPS/Weapons/WeaponTypes.h"
#include "AmmoPickup.generated.h"


UCLASS()
class FIRSTFPS_API AAmmoPickup : public ABasePickup
{
	GENERATED_BODY()
public:
protected:
	/** 球形碰撞体(OverlapSphere)发生重叠时被调用的回调函数 */
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;
private:
	UPROPERTY(EditAnywhere)
	int32 AmmoAmount = 30;

	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType;
};
