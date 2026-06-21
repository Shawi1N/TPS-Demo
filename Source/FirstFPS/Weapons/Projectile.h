// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class UBoxComponent;
class UProjectileMovementComponent;
class UParticleSystem;
class UParticleSystemComponent;
class USoundCue;

UCLASS()
class FIRSTFPS_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	AProjectile();
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;
protected:
	virtual void BeginPlay() override;

	/* 碰撞事件HitEvent的回调函数, 当 Projectile 和 某个Blocking物体 发生碰撞时触发 */
	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(EditAnyWhere)
	float Damage = 20.f;

private:
	UPROPERTY(EditAnywhere)
	UBoxComponent* CollisionBox;

	/** UE 提供的负责投射物运动的组件, 根据速度、重力、碰撞等设置, 在 Tick() 中更新 UpdatedComponent 的位置 */
	UPROPERTY(VisibleAnywhere)
	UProjectileMovementComponent* ProjectileMovementComponent;

	/** 粒子效果 */
	// (投射物)飞行时的拖尾特效
	UPROPERTY(EditAnywhere)
	UParticleSystem* Tracer; // UParticleSystem: 一个完整的粒子特效资源, 可直接在某个世界位置播放
	UParticleSystemComponent* TracerComponent; // UParticleSystemComponent: 负责在世界中附加(挂载在某个组件上)并播放一个 UParticleSystem粒子特效
	// (投射物)击中特效
	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticles;
	UPROPERTY(EditAnywhere)
	USoundCue* ImpactSound;
};
