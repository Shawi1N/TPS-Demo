// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FirstFPS/Weapons/WeaponTypes.h"
#include "FirstFPS/CharacterTypes/CombatState.h"
#include "CombatComponent.generated.h"

#define TRACE_LENGTH 80000.f

class ABaseCharacter;
class AWeapon;
class ABaseCharacterPlayerController;
class ABaseCharacterHUD;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FIRSTFPS_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** CombatComponent 和 BaseCharacter 彼此高度依赖. 因此设为friend class友元类, 让 BaseCharacter 获得 CombatComponent 的全部访问权限 */
	friend class ABaseCharacter;

	void EquipWeapon(AWeapon* WeaponToEquip);
	void SwapWeapons();
	void Reloading();
	UFUNCTION(BlueprintCallable)
	void FinishReloading();

	/** 处理Character拾取弹药的逻辑 */
	void PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount);
	bool bLocallyReloading = false;
protected:
	virtual void BeginPlay() override;

	void SetAiming(bool bIsAiming);

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);

	void FireButtonPressed(bool bPressed);

	void Fire();
	void LocalFire(const FVector_NetQuantize& TraceHitTarget);
	/* FVector_NetQuantize: UE为网络传输优化过的FVector类型(本质上依然是三维向量, 但在 网络复制/RPC传输 时会降低数值精度, 以减少带宽) */
	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	/** 为了保证玩家的瞄准结果和实际射击结果一致, 一般通过准星确认目标点(在准星位置发起Line Trace) */
	void TraceUnderCrosshairs(FHitResult& TraceHitResult); // 将 Line Trace的结果 存储在 引用参数TraceHitResult 中

	/** 根据当前Weapon, 应用对应的准星贴图 */
	/** 同时准星会根据移动、跳跃、开火、瞄准等状态进行平滑扩散和收缩, 因此需要 DeltaTime(上一帧到这一帧经过了多少秒) */
	void SetHUDCrosshairs(float DeltaTime);

	UFUNCTION(Server, Reliable) // 服务器RPC
	void ServerReloading();
	void HandleReloading();
	int32 AmountToReload(); // 计算需要放入Weapon中的弹药数

	void DropEquippedWeapon();
	void AttachActorToRightHand(AActor* ActorToAttach);
	void AttacgActorToBack(AActor* ActorToAttach);
	void UpdateCarriedAmmo(); // 根据当前装备武器的WeaponType, 从 CarriedAmmoMap 获取对应的备用弹药数量, 用于更新HUD上显示的备用弹药数
	void PlayEquipWeaponSound(AWeapon* WeaponToEquip);
	void EquipPrimaryWeapon(AWeapon* WeaponToEquip);   // 主武器装备逻辑
	void EquipSecondaryWeapon(AWeapon* WeaponToEquip); // 副武器装备逻辑
private:
	UPROPERTY()
	ABaseCharacter* Character;
	/* 存储当前玩家的PlayerController和HUD, 避免重复读取 */
	UPROPERTY()
	ABaseCharacterPlayerController* Controller;
	UPROPERTY()
	ABaseCharacterHUD* HUD;

	UPROPERTY(Replicated)
	AWeapon* EquippedWeapon;
	UPROPERTY(Replicated)
	AWeapon* SecondaryWeapon; // 副武器

	UPROPERTY(ReplicatedUsing = OnRep_Aiming)
	bool bAiming = false;			// 网络状态: 角色是否处于瞄准状态

	bool bAimButtonPressed = false; // 本地输入状态: 玩家现在是否按着右键 (本地玩家当前最新输入意图)

	UFUNCTION()
	void OnRep_Aiming();			// Owning Client 收到迟到状态时, 重新应用最新本地输入

	/* 瞄准时的行走速度 应比 正常行走速度 慢 */
	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;
	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;

	/* 控制瞄准镜头的缩放范围和速度 */
	float DefaultFOV; // 未瞄准时的FOV(Camera默认FOV)
	float CurrentFOV; // 当前FOV
	UPROPERTY(EditAnywhere, Category = "Combat")
	float ZoomInterpSpeed = 20.f; // 恢复为 默认FOV(未瞄准) 的插值速度
	/** 处理FOV变化 */
	void InterpFOV(float DeltaTime);

	/* 因为 FInterpTo() 需要上一帧的值, 所以将各准星插值变量存储起来 */
	float CrosshairVelocityFactor;
	float CrosshairInAirFactor;
	float CrosshairAimFactor;
	float CrosshairShootingFactor;

	FVector HitTarget; // 准星位置下 Line Trace 的目标点

	/* 自动射击: 通过 计时器 实现 */
	FTimerHandle FireTimer;
	bool bCanFire = true;
	bool CanFire();
	void StartFireTimer();	  // 开火后启动一个计时器, 在 FireDelay 秒后恢复开火能力
	void FireTimerFinished(); // 开火冷却结束后自动调用的函数
	bool bFireButtonPressed;

	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CarriedAmmo;
	UFUNCTION()
	void OnRep_CarriedAmmo();

	TMap<EWeaponType, int32> CarriedAmmoMap; // TMap<KeyType, ValueType>: 类似std::unordered_map, 此处用于"武器类型对应的携带弹药数"
	UPROPERTY(EditAnywhere)
	int32 StartingARAmmo = 45; // 角色初始自带的突击步枪弹药
	UPROPERTY(EditAnywhere)
	int32 StartingRocketAmmo = 0;
	void InitializeCarriedAmmo();

	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;
	UFUNCTION()
	void OnRep_CombatState();

	void UpdateAmmoValues();
public:
	bool ShouldSwapWeapons();
};
