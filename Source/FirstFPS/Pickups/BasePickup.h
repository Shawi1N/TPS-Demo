// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BasePickup.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class USoundCue;
class UNiagaraComponent;
class UNiagaraSystem;

UCLASS()
class FIRSTFPS_API ABasePickup : public AActor
{
	GENERATED_BODY()
	
public:	
	ABasePickup();
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;
protected:
	virtual void BeginPlay() override;

	/** 球形碰撞体(OverlapSphere)发生重叠时被调用的回调函数 */
	UFUNCTION()
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
private:
	UPROPERTY(EditAnywhere)
	USphereComponent* OverlapSphere;
	UPROPERTY(EditAnywhere)
	USoundCue* PickupSound;
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* PickupMesh;

	/* UNiagaraComponent: 挂在 Actor 上的组件, 可持续播放 Niagara 特效 */
	UPROPERTY(VisibleAnywhere)
	UNiagaraComponent* PickupEffectComponent; // 拾取物外观 (Niagara视觉特效)
	/* UNiagaraSystem: 代表一个 Niagara 特效资源, 用于在某个时刻临时播放 */
	UPROPERTY(EditAnywhere)
	UNiagaraSystem* PickupEffect;			  // 拾取时的特效
public:

};
