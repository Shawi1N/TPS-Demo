// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BasePickup.h"
#include "JumpPickup.generated.h"

/**
 * 
 */
UCLASS()
class FIRSTFPS_API AJumpPickup : public ABasePickup
{
	GENERATED_BODY()
protected:
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;
private:
	UPROPERTY(EditAnywhere)
	float JumpZVelocityBuff = 1000.f;
	UPROPERTY(EditAnywhere)
	float JumpBuffTime = 30.f;
};
