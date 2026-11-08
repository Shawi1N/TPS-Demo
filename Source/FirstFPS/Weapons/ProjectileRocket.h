// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "ProjectileRocket.generated.h"

/**
 * 
 */
UCLASS()
class FIRSTFPS_API AProjectileRocket : public AProjectile
{
	GENERATED_BODY()
public:
	AProjectileRocket();
protected:
	/* 碰撞事件HitEvent的回调函数, 当 Projectile 和 某个Blocking物体 发生碰撞时触发 */
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
private:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* RocketMesh;
};
