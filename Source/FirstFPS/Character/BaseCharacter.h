// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FirstFPS/CharacterTypes/CombatState.h"
#include "BaseCharacter.generated.h"

class UBoxComponent;
class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class AWeapon;
class UCombatComponent;
class UBuffComponent;
class UAnimMontage;
class ABaseCharacterPlayerController;
class ABaseCharacterPlayerState;
class ULagCompensationComponent;
struct FInputActionValue; // Enhanced Input系统 用来传递输入值的结构体, 可容纳不同Action的值类型(bool/float/FVector2D/FVector)

UCLASS()
class FIRSTFPS_API ABaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ABaseCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	/** 在 Actor 的所有组件初始化完成后调用 */
	virtual void PostInitializeComponents() override;
	/** 任何需要 变量复制 的类, 都需要重写 GetLifetimeReplicatedProps虚函数 */
	/** 用于将 已声明的可复制变量 注册到复制列表, 并设置复制条件(可选) */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 真正的行为逻辑. Enhanced Input回调函数只负责解析输入, 实际行为由这些函数执行, 方便后续通过蓝图、UI、AI等调用 */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoMove(float Right, float Forward);
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoLook(float Yaw, float Pitch);
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpStart();
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpEnd();

	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped();
	bool IsAiming();
	void PlayFireMontage(bool bAiming);
	void PlayReloadMontage();

	void Elim();
	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim();
	void PlayDeathMontage(bool bAiming);

	/** 在结算阶段中, 禁止玩家接收伤害 */
	UPROPERTY(Replicated)
	bool bDisableDealDamage = false;

	void SpawnDefaultWeapon();

	void UpdateHUDHealth();
	void UpdateHUDShield();
	void UpdateHUDAmmo();

	UPROPERTY()
	TMap<FName, UBoxComponent*> HitCollisionBoxes; // 创建一个容器, 方便统一处理所有的HitBoxes
protected:
	virtual void BeginPlay() override;

	/* UInputAction 无需外部类访问, 通过蓝图配置即可 */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* JumpAction;
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* MoveAction;
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* LookAction;
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* EquipAction;
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* CrouchAction;
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* AimAction;
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* FireAction;
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* ReloadAction;

	/** Enhanced Input 回调函数. 参数FInputActionValue表示Action的不同值类型, 将其转换为实际对应类型后进行处理 */
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Equip(const FInputActionValue& Value);
	void CrouchButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();
	void FireButtonPressed();
	void FireButtonReleased();
	void ReloadButtonPressed();

	void AimOffset(float DeltaTime); // 计算 AO_Yaw 和 AO_Pitch, 由 Tick() 调用

	void PlayeHitReactMontage();
	// 被 OnTakeAnyDamage委托 所绑定的回调函数
	UFUNCTION()
	void ReceiveDamage(AActor* DamageActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);
	
	/* 在多人游戏中, 网络复制、Possess、PlayerState 和 HUD 在 BeginPlay()阶段 不一定已经全部初始化完成 */
	void PollInit();

	/** 命中判定盒: 为角色设置对应区域(头、胸腹、手臂、腿)的命中判定盒. 用于服务器回溯的射击判定 */
	// 头部
	UPROPERTY(EditAnywhere)
	UBoxComponent* head;
	// 胸部和腹部
	UPROPERTY(EditAnywhere)
	UBoxComponent* pelvis; // 骨盆
	UPROPERTY(EditAnywhere)
	UBoxComponent* spine_02;
	UPROPERTY(EditAnywhere)
	UBoxComponent* spine_03;
	UPROPERTY(EditAnywhere)
	// 手臂
	UBoxComponent* upperarm_l;
	UPROPERTY(EditAnywhere)
	UBoxComponent* upperarm_r;
	UPROPERTY(EditAnywhere)
	UBoxComponent* lowerarm_l;
	UPROPERTY(EditAnywhere)
	UBoxComponent* lowerarm_r;
	UPROPERTY(EditAnywhere)
	UBoxComponent* hand_l;
	UPROPERTY(EditAnywhere)
	UBoxComponent* hand_r;
	// 腿部
	UPROPERTY(EditAnywhere)
	UBoxComponent* thigh_l;
	UPROPERTY(EditAnywhere)
	UBoxComponent* thigh_r;
	UPROPERTY(EditAnywhere)
	UBoxComponent* calf_l;
	UPROPERTY(EditAnywhere)
	UBoxComponent* calf_r;
	UPROPERTY(EditAnywhere)
	UBoxComponent* foot_l;
	UPROPERTY(EditAnywhere)
	UBoxComponent* foot_r;
	/* 背包
	UPROPERTY(EditAnywhere)
	UBoxComponent* backpack;
	*/
private:
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	USpringArmComponent* CameraBoom;
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	UCameraComponent* FollowCamera;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UCombatComponent* Combat; // 自定义组件, 用于管理角色所有战斗逻辑
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UBuffComponent* Buff; // 自定义组件, 用于管理角色所有增益效果逻辑
	UPROPERTY(VisibleAnywhere)
	ULagCompensationComponent* LagCompensation; // 自定义组件, 用于服务器回溯相关逻辑

	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* FireMontage;
	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* ReloadMontage;
	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* HitReactMontage;
	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* DeathMontage;

	/** 复制通知RepNotify: 一般命名为 OnRep_复制变量名. 该变量发生复制时, 自动触发自定义函数 */
	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	AWeapon* OverlappingWeapon;
	/** RepNotify触发函数的参数有2种形式: 1)无参数 2)带一个参数, 用来接收"复制前的旧值" */
	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);
	
	/** 远程过程调用RPC: 让一台机器去调用另一台机器上的函数 */
	/** Reliable: 该RPC会不断重发, 直到接收方确认收到 */
	UFUNCTION(Server, Reliable)	// Server RPC: 由 拥有该Actor的Clinet 调用, 但这个函数会在 Server 执行
	void ServerEquip();

	/* 虽然以下数据不是属性复制的(不影响游戏逻辑判定, 一般无需复制), 但它们被各端用同步后的基础数据重新算出来, 因此视觉上几乎是一致的 */
	float AO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation; // 保存 该Character静止不动时的正方向(用于后续与 Controller方向 做插值算出AO)

	/** 玩家生命值 */
	UPROPERTY(EditAnywhere, Category = "Player States")
	float MaxHealth = 100.f;
	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player States")
	float Health = 100.f;
	UFUNCTION()
	void OnRep_Health(float LashHealth); // 因为多了治疗操作, 因此需通过 LashHealth 判断是否需要执行PlayHitReactMontage
	/** 玩家护盾值(与生命值同理) */
	UPROPERTY(EditAnywhere, Category = "Player States")
	float MaxShield = 100.f;
	UPROPERTY(ReplicatedUsing = OnRep_Shield, EditAnywhere, Category = "Player States")
	float Shield = 0.f;
	UFUNCTION()
	void OnRep_Shield(float LashShield);

	/* 淘汰计时器 */
	FTimerHandle ElimTimer;
	UPROPERTY(EditDefaultsOnly) // 确保各角色淘汰时间一致
	float ElimDelay = 3.f;
	void ElimTimerFinished();

	bool bElimmed = false;

	UPROPERTY()
	ABaseCharacterPlayerController* BaseCharacterPlayerController;
	UPROPERTY()
	ABaseCharacterPlayerState* BaseCharacterPlayerState;

	/* 默认武器 */
	UPROPERTY(EditAnywhere)
	TSubclassOf<AWeapon> DefaultWeaponClass;

	void DropOrDestroyWeapon(AWeapon* Weapon);
public:
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE void SetHealth(float Amount) { Health = Amount; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE float GetShield() const { return Shield; }
	FORCEINLINE void SetShield(float Amount) { Shield = Amount; }
	FORCEINLINE float GetMaxShield() const { return MaxShield; }
	ECombatState GetCombatState() const;
	FORCEINLINE UCombatComponent* GetCombat() const { return Combat; }
	FORCEINLINE UBuffComponent* GetBuff() const { return Buff; }
	FORCEINLINE bool IsElimmed() const { return bElimmed; }
	AWeapon* GetEquippedWeapon() const;
	FORCEINLINE ULagCompensationComponent* GetLagCompensation() const { return LagCompensation; }
};