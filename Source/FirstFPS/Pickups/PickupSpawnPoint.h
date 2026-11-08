// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupSpawnPoint.generated.h"

class ABasePickup;

UCLASS()
class FIRSTFPS_API APickupSpawnPoint : public AActor
{
	GENERATED_BODY()
public:	
	APickupSpawnPoint();
	virtual void Tick(float DeltaTime) override;
protected:
	virtual void BeginPlay() override;

	/* OnDestroyed 是动态委托, 绑定的函数必须是 UFUNCTION(), 并且参数要匹配：void FunctionName(AActor* DestroyedActor); */
	UFUNCTION()
	void StartSpawnPickupTimer(AActor* DestroyActor); // AActor::OnDestroyed()委托绑定的回调函数

	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<class ABasePickup>> PickupClasses; // 保存"可以随机生成的拾取物类型"
	UPROPERTY()
	ABasePickup* SpawnedPickup;  // 保存当前已经生成出来的拾取物实例 
		
	void SpawnPickup(); // 生成拾取物的逻辑
	
	void SpawnPickupTimerFinished(); // 当计时器时间到了，服务器生成一个新的 Pickup
	
private:	
	FTimerHandle SpawnPickupTimer;

	/* 计时器等待时间区间 */
	UPROPERTY(EditAnywhere)
	float SpawnPickupTimeMin;
	UPROPERTY(EditAnywhere)
	float SpawnPickupTimeMax;

};
