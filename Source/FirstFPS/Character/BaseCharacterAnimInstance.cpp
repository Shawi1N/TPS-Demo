// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseCharacterAnimInstance.h"
#include "BaseCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

void UBaseCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	BaseCharacter = Cast<ABaseCharacter>(TryGetPawnOwner());

}

void UBaseCharacterAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (BaseCharacter == nullptr)
		BaseCharacter = Cast<ABaseCharacter>(TryGetPawnOwner()); // 防止初始化阶段没有拿到角色(?)

	if (BaseCharacter == nullptr) return;

	FVector Velocity = BaseCharacter->GetVelocity(); // 获取 BaseCharacter (CharacterMovementComponent)当前计算出来的速度
	Velocity.Z = 0.f;
	Speed = Velocity.Size(); // 仅关心XY平面的速度
	bIsInAir = BaseCharacter->GetCharacterMovement()->IsFalling(); // 判断角色当前是否在空中 (BaseCharacter->GetVelocity().Z > 0.f)
	bIsAccelerating = BaseCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f;
	bIsCrouched = BaseCharacter->bIsCrouched; // 此处的 bIsCrouched 是 ACharacter 自带的一个布尔变量. 它的网络同步已由 Character移动系统 处理好
	bIsAiming = BaseCharacter->IsAiming();
	bWeaponEquipped = BaseCharacter->IsWeaponEquipped();

	/* GetBaseAimRotation(): 如果该Pawn存在Player Controller, 返回Controller方向(全局变量, 正对世界X轴为0°). 此处表示"视角方向" */
	FRotator AimRotation = BaseCharacter->GetBaseAimRotation();
	/* MakeRotFromX(): 根据方向向量生成一个FRotator. 此处使用 角色当前速度方向(角色移动方向, 正对世界X轴为0°), 表示"实际移动方向" */
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(BaseCharacter->GetVelocity());
	/* NormalizedDeltaRotator(A,B): 计算 A相对于B的旋转差值, 再把角度规范化到最短角度范围内(-180°, 180°]. 避免角度跳变导致Blend Space的动画乱跳 */
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	/* 每帧把 当前旋转值DeltaRotation 平滑地插值到 目标旋转DeltaRot, 插值速度(当前值靠近目标值的快慢程度)是 5.f */
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 5.f); // 让 Blend Space 的参数平滑变化，避免角色移动动画生硬跳变
	YawOffset = DeltaRotation.Yaw;

	AO_Yaw = BaseCharacter->GetAO_Yaw();
	AO_Pitch = BaseCharacter->GetAO_Pitch();
}