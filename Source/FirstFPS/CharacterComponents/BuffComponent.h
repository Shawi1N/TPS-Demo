// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"

class ABaseCharacter;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FIRSTFPS_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UBuffComponent();
	friend class ABaseCharacter; // 设为friend class友元类, 让 BaseCharacter 获得 BuffComponent 的全部访问权限
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void Heal(float HealAmount, float HealTime); // 启动回血效果
	void ReplenishShield(float ShieldAmount, float ReplenishTime); // 启动恢复护盾效果
	void BuffSpeed(float BuffBaseSpeed, float BuffCrouchSpeed, float BuffTime); // 启动加速效果
	void BuffJump(float BuffJumpVelocity, float BuffTime); // 启动跳跃增益效果
	void SetInitialSpeed(float BaseSpeed, float CrouchSpeed);
	void SetInitialJump(float Velocity);
protected:
	virtual void BeginPlay() override;
	void HealRampUp(float DeltaTime); // 每帧实际执行回血: 根据 HealingRate, 计算每帧恢复的生命值
	void ShieldRampUp(float DeltaTime);
private:
	UPROPERTY()
	ABaseCharacter* Character;

	/** 生命值增益 */
	bool bHealing = false;	  // 是否正在治疗
	/* 用于 "持续治疗"效果 (按照治疗速率, 缓慢恢复生命值) 的参数 */
	float HealingRate = 0.f;  // 治疗速率 (每秒恢复的生命值)
	float AmountToHeal = 0.f; // 待恢复总量 (当恢复生命值超过 AmountToHeal, 停止增加生命值)

	/** 护盾增益(逻辑同上) */
	bool bReplenishingShield = false;
	float ShieldReplenishRate = 0.f;
	float ShieldReplenishAmount = 0.f;

	/** 速度增益 */
	FTimerHandle SpeedBuffTimer; // 速度增益计时器
	void ResetSpeed();			 // 速度增益计时结束后, 调用的回调函数
	float InitialBaseSpeed;
	float InitialCrouchSpeed;
	/* Health属性 和 Speed属性 在多人游戏里的同步机制不同, Health 是 ABaseCharacter 里的一个 replicated 变量; */
	/* MaxWalkSpeed 和 MaxWalkSpeedCrouched 是 UCharacterMovementComponent 的普通属性, 不会因为 Server 中修改就同步到 Clients */
	/* 因此需要通过 多播RPC, 让 Server 通知 所有Clients 将 MaxWalkSpeed 和 MaxWalkSpeedCrouched 的值改变  */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpeedBuff(float BaseSpeed, float CrouchSpeed);

	/** 跳跃增益 */
	FTimerHandle JumpBuffTimer;
	void ResetJump();
	float InitialJumpVelocity;
	UFUNCTION(NetMulticast, Reliable)
	void MulticastJumpBuff(float JumpVelocity);
};
