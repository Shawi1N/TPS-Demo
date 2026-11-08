// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LagCompensationComponent.generated.h"

class ABaseCharacter;
class ABaseCharacterPlayerController;

/* 结构体FBoxInformation: 用于存储某一帧时, 某个HitBox的位置、旋转和大小 */
USTRUCT(BlueprintType)
struct FBoxInformation
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Location;
	UPROPERTY()
	FRotator Rotation;
	UPROPERTY()
	FVector BoxExtent;
};

/* 结构体FFramePackage: 存储 角色的所有击中判定信息 和 这些数据的采集时间 <每一帧的击中判定信息> */
USTRUCT(BlueprintType)
struct FFramePackage
{
	GENERATED_BODY()

	UPROPERTY()
	float Time;								 // 击中判定信息的获取时间
	
	UPROPERTY()
	TMap<FName, FBoxInformation> HitBoxInfo; // 角色的所有击中判定信息: [骨骼名称, 对应的HitBox信息]

	UPROPERTY()
	ABaseCharacter* Character;
};

/* 结构体FServerSideRewindResult: 命中结果, 存储 是否击中的布尔值 和 是否击中头部的布尔值 */
USTRUCT(BlueprintType)
struct FServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	bool bHitConfirmed;

	UPROPERTY()
	bool bHeadShot;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FIRSTFPS_API ULagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	friend class ABaseCharacter; // 允许 ABaseCharacter类 直接访问 当前类(LagCompensationCompoent)的 private和protected成员

	ULagCompensationComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	/** Hitscan即时检测命中的服务端回溯	*/
	FServerSideRewindResult ServerSideRewind(ABaseCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime);
	/* Server命中请求 */
	UFUNCTION(Server, Reliable)
	void ServerScoreRequest(
		ABaseCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation,
		float HitTime
	);

protected:
	virtual void BeginPlay() override;
	void SaveFramePackage();
	void SaveFramePackage(FFramePackage& Package);
	FFramePackage InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime);
	// 返回命中结果
	FServerSideRewindResult ConfirmHit(
		const FFramePackage& Package,		   // HitTime 时刻, 被击中角色的 HitBoxes 状态 
		ABaseCharacter* HitCharacter,		   // 客户端声称自己击中的角色
		const FVector_NetQuantize& TraceStart, // HitscanWeapon的射线检测起点
		const FVector_NetQuantize& HitLocation // 客户端当时瞄准的命中点
	);
	void CacheBoxPositions(ABaseCharacter* HitCharacter, FFramePackage& OutFramePackage);
	void MoveBoxes(ABaseCharacter* HitCharacter, const FFramePackage& Package);
	void ResetHitBoxes(ABaseCharacter* HitCharacter, const FFramePackage& Package);
	void EnableCharacterMeshCollision(ABaseCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled);
private:
	UPROPERTY()
	ABaseCharacter* Character;
	UPROPERTY()
	ABaseCharacterPlayerController* Controller;

	/* 通过 双链表, 存储 历史帧数据. 只保存指定时长(MaxRecordTime)的历史数据, 一旦超过指定时长, 自动丢弃旧帧数据 */
	TDoubleLinkedList<FFramePackage> FrameHistory;
	UPROPERTY(EditAnywhere)
	float MaxRecordTime = 0.4f; // 保存400ms内的历史数据(HitBoxes信息)
};
