// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FirstFPS/Weapons/WeaponTypes.h"
#include "Weapon.generated.h"

/** 武器状态枚举 */
UENUM(BlueprintType) // 该类型可在Blueprint中作为变量类型使用
enum class EWeaponState : uint8 {
	EWS_Initial UMETA(DisplayName = "Initial State"), // 未拾取
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_EquippedSecondary UMETA(DisplayName = "Equipped Secondary"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),		  // 武器掉落, 此时可开启物理碰撞
	
	EWS_MAX UMETA(DisplayName = "DefaultMAX")
};

class USphereComponent;
class UWidgetComponent;
class UAnimationAsset;
class UTexture2D;
class ABaseCharacter;
class ABaseCharacterPlayerController;
class USoundCue;

UCLASS()
class FIRSTFPS_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeapon();
	virtual void Tick(float DeltaTime) override;
	void ShowPickupWidget(bool bShowWidget); // 设置 拾取提示控件 的显示/隐藏
	void SetWeaponState(EWeaponState State);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnRep_Owner() override; // AActor::Owner: 当前 Actor 的逻辑所有者; 本质上是AActor*, 已实现复制通知
	void SetHUDAmmo();
	virtual void Fire(const FVector& HitTarget);
	void Dropped(); // 处理武器丢弃逻辑
	void AddAmmo(int32 AmmoToAdd); // 为Weapon添加弹药

	/** 准星纹理变量 (不同的Weapon可能使用不同的准星) */
	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	UTexture2D* CrosshairsCenter;
	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	UTexture2D* CrosshairsLeft;
	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	UTexture2D* CrosshairsRight;
	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	UTexture2D* CrosshairsTop;
	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	UTexture2D* CrosshairsBottom;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float FireDelay = .15f; // 当前Weapon的射击冷却时间(开火所需间隔)
	UPROPERTY(EditAnywhere, Category = "Combat")
	bool bAutomatic = true; // 当前Weapon是否支持连发

	UPROPERTY(EditAnywhere)
	USoundCue* EquipSound; // 不同武器装备时可能拥有不同的音效

	bool bDestroyWeapon = false; // 决定 Character持有该Weapon 后, 在 Character 死亡时是否销毁Weapon 

protected:
	virtual void BeginPlay() override;

	virtual void OnWeaponStateSet();
	virtual void OnEquipped();
	virtual void OnEquippedSecondary();
	virtual void OnDropped();

	UFUNCTION()
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	virtual void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UPROPERTY()
	ABaseCharacter* WeaponOnwerCharcter;
	UPROPERTY()
	ABaseCharacterPlayerController* WeaponOnwerController;
	
	/**
	ProjectileWeapon: Weapon 负责 Spawn Projectile;  Projectile 负责命中;  Projectile 负责 Damage
	HitscanWeapon:									 Weapon 负责命中;		  Weapon 负责 Damage
	*/
	UPROPERTY(EditAnywhere)
	float Damage = 20.f;
	UPROPERTY(EditAnywhere)
	float HeadShotDamage = 40.f;

	UPROPERTY(Replicated, EditAnywhere)
	bool bUseServerSideRewind = false; // 是否启用"服务器回溯"

	/* HighPingDelegate 的 回调函数 */
	UFUNCTION()
	void OnPingTooHigh(bool bPingTooHigh);
private:
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USphereComponent* AreaSphere;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	UWidgetComponent* PickupWidget;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	UAnimationAsset* FireAnimation; // UE 动画资源的抽象基类, 属于UAnimSequence、UAnimMontage、UBlendSpace的共同父类

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState;
	UFUNCTION()
	void OnRep_WeaponState();

	/** 不同Weapon在瞄准时的 相机视野角ZoomedFOV 和 镜头FOV变化的插值速度ZoomInterpSpeed 可能不同 */
	UPROPERTY(EditAnywhere)
	float ZoomedFOV = 45.f;
	UPROPERTY(EditAnywhere)
	float ZoomInterpSpeed = 25.f;

	UPROPERTY(EditAnywhere)
	int32 Ammo; /** 不再使用属性复制. 避免 客户端预测 后, Server因为延迟造成错误的校正 */

	/** Client已经预测执行, 但Server结果尚未返回的射击数量; 该值会在 SpendRound() 中递增, 在 ClientUpdateAmmo() 中递减 */
	int32 Sequence = 0;

	UFUNCTION(Client, Reliable)
	void ClientUpdateAmmo(int32 ServerAmmo); // 弹药每次减1, 调用 该Client RPC (由Server调用, 在 该Actor 的 Owning Client 上执行)
	UFUNCTION(Client, Reliable)
	void ClientAddAmmo(int32 AmmoToAdd);

	void SpendRound();

	UPROPERTY(EditAnywhere)
	int32 MagCapacity; // Weapon的弹匣(最大)容量

	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType;
public:
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }
	FORCEINLINE bool IsMagEmpty() { return Ammo <= 0; }
	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }
	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	FORCEINLINE int32 GetMagCapacity() const { return MagCapacity; }
	FORCEINLINE float GetDamage() const { return Damage; }
	FORCEINLINE float GetHeadShotDamage() const { return HeadShotDamage; }
};
