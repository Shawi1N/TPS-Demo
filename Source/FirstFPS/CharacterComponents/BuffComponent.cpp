// Fill out your copyright notice in the Description page of Project Settings.


#include "BuffComponent.h"
#include "FirstFPS/Character/BaseCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

UBuffComponent::UBuffComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UBuffComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UBuffComponent::SetInitialSpeed(float BaseSpeed, float CrouchSpeed)
{
	InitialBaseSpeed = BaseSpeed;
	InitialCrouchSpeed = CrouchSpeed;
}

void UBuffComponent::SetInitialJump(float Velocity)
{
	InitialJumpVelocity = Velocity;
}

void UBuffComponent::Heal(float HealAmount, float HealTime)
{
	bHealing = true;
	HealingRate = HealAmount / HealTime; // 每秒恢复的生命值
	AmountToHeal += HealAmount; // 当恢复生命值超过 AmountToHeal, 停止治疗
}

void UBuffComponent::ReplenishShield(float ShieldAmount, float ReplenishTime)
{
	bReplenishingShield = true;
	ShieldReplenishRate = ShieldAmount / ReplenishTime;
	ShieldReplenishAmount += ShieldAmount;
}

void UBuffComponent::BuffSpeed(float BuffBaseSpeed, float BuffCrouchSpeed, float BuffTime)
{
	if (Character && Character->GetCharacterMovement()) {
		Character->GetWorldTimerManager().SetTimer(SpeedBuffTimer, this, &UBuffComponent::ResetSpeed, BuffTime);

		Character->GetCharacterMovement()->MaxWalkSpeed = BuffBaseSpeed;
		Character->GetCharacterMovement()->MaxWalkSpeedCrouched = BuffCrouchSpeed;
		MulticastSpeedBuff(BuffBaseSpeed, BuffCrouchSpeed);
	}
	
}

void UBuffComponent::BuffJump(float BuffJumpVelocity, float BuffTime)
{
	if (Character && Character->GetCharacterMovement()) {
		Character->GetWorldTimerManager().SetTimer(JumpBuffTimer, this, &UBuffComponent::ResetJump, BuffTime);

		Character->GetCharacterMovement()->JumpZVelocity = BuffJumpVelocity;
		MulticastJumpBuff(InitialJumpVelocity);
	}
}

void UBuffComponent::MulticastSpeedBuff_Implementation(float BaseSpeed, float CrouchSpeed)
{
	Character->GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
	Character->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
}

void UBuffComponent::MulticastJumpBuff_Implementation(float JumpVelocity)
{
	Character->GetCharacterMovement()->JumpZVelocity = JumpVelocity;
}

void UBuffComponent::ResetSpeed()
{
	if (Character && Character->GetCharacterMovement()) {
		Character->GetCharacterMovement()->MaxWalkSpeed = InitialBaseSpeed;
		Character->GetCharacterMovement()->MaxWalkSpeedCrouched = InitialCrouchSpeed;
		MulticastSpeedBuff(InitialBaseSpeed, InitialCrouchSpeed);
	}
}

void UBuffComponent::ResetJump()
{
	if (Character && Character->GetCharacterMovement()) {
		Character->GetCharacterMovement()->JumpZVelocity = InitialJumpVelocity;
		MulticastJumpBuff(InitialJumpVelocity);
	}
}

void UBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	HealRampUp(DeltaTime);
	ShieldRampUp(DeltaTime);
}

void UBuffComponent::HealRampUp(float DeltaTime)
{
	if (!bHealing || Character == nullptr || Character->IsElimmed()) return;

	const float HealThisFrame = HealingRate * DeltaTime; // 每帧恢复的生命值
	Character->SetHealth(FMath::Clamp(Character->GetHealth() + HealThisFrame, 0.f, Character->GetMaxHealth()));
	Character->UpdateHUDHealth();
	AmountToHeal -= HealThisFrame;

	if (AmountToHeal <= 0.f || Character->GetHealth() >= Character->GetMaxHealth()) {
		bHealing = false;
		AmountToHeal = 0.f;
	}
}

void UBuffComponent::ShieldRampUp(float DeltaTime)
{
	if (!bReplenishingShield || Character == nullptr || Character->IsElimmed()) return;

	const float ReplenishThisFrame = ShieldReplenishRate * DeltaTime; // 每帧恢复的护盾值
	Character->SetShield(FMath::Clamp(Character->GetShield() + ReplenishThisFrame, 0.f, Character->GetMaxShield()));
	Character->UpdateHUDShield();
	ShieldReplenishAmount -= ReplenishThisFrame;

	if (ShieldReplenishAmount <= 0.f || Character->GetShield() >= Character->GetMaxShield()) {
		bReplenishingShield = false;
		ShieldReplenishAmount = 0.f;
	}
}
