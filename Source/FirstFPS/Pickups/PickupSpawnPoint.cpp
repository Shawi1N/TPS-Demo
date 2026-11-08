// Fill out your copyright notice in the Description page of Project Settings.


#include "PickupSpawnPoint.h"
#include "BasePickup.h"

APickupSpawnPoint::APickupSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true; // 该Actor 会复制到 Clients
}

void APickupSpawnPoint::BeginPlay()
{
	Super::BeginPlay();

	StartSpawnPickupTimer((AActor*)nullptr); // 游戏开始后，立刻启动一次生成拾取物的计时器
}

void APickupSpawnPoint::StartSpawnPickupTimer(AActor* DestroyActor)
{
	const float SpawnTime = FMath::FRandRange(SpawnPickupTimeMin, SpawnPickupTimeMax);
	GetWorldTimerManager().SetTimer(SpawnPickupTimer, this, &APickupSpawnPoint::SpawnPickupTimerFinished, SpawnTime);
}

void APickupSpawnPoint::SpawnPickupTimerFinished()
{
	/* 当计时器时间结束, 在 Server 生成一个新的 Pickup类 */
	if (HasAuthority()) {
		SpawnPickup();
	}
}

void APickupSpawnPoint::SpawnPickup()
{
	int32 NumPickupClasses = PickupClasses.Num();
	if (NumPickupClasses > 0) {
		int32 Selection = FMath::RandRange(0, NumPickupClasses - 1);
		/* AActor* SpawnActor(UClass* Class, FTransform* Transform): 生成一个 Class 类型的 Actor, 使用 Transform 作为生成位置、旋转和缩放 */
		SpawnedPickup = GetWorld()->SpawnActor<ABasePickup>(PickupClasses[Selection], GetActorTransform());

		if (HasAuthority()) {
			/* OnDestroyed: AActor 自带的一个事件委托, 用来在 某个Actor 被销毁时通知其他对象 */
			/* 当当前拾取物被销毁时, 重新启动生成拾取物计时器 */
			SpawnedPickup->OnDestroyed.AddDynamic(this, &APickupSpawnPoint::StartSpawnPickupTimer);
		}
	}
}

void APickupSpawnPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

