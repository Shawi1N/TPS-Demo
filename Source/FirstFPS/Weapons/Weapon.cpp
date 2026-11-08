// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "Components/SphereComponent.h"
#include "FirstFPS/Character/BaseCharacter.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Animation/AnimationAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "FirstFPS/Character/BaseCharacterPlayerController.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true; // 参与网络复制

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);
	// 初始阶段(未拾取), Weapon不参与碰撞
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); 
	// 拾取后, 忽略对 持有角色(Pawn) 的碰撞
	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(RootComponent);
	/** AreaSphere用于"检测Actor靠近Weapon时触发拾取相关逻辑". 但仅在Server处理, Clinet只负责提交操作意图和处理本地表现 */
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(RootComponent);
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	if (PickupWidget) {
		PickupWidget->SetVisibility(false);
	}
	/** 在多人游戏中, 凡是会影响公共游戏状态的逻辑, 都应由Server作为权威端进行验证、执行和同步 */
	if (HasAuthority()) {
		// 在 Server端 检测Actor是否靠近Weapon (仅在Server端使用PickAreaSphere)
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
		AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap);
		AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap);
	}
}

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(OtherActor);
	if (BaseCharacter) {
		BaseCharacter->SetOverlappingWeapon(this);
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(OtherActor);
	if (BaseCharacter) {
		BaseCharacter->SetOverlappingWeapon(nullptr);
	}
}

void AWeapon::OnPingTooHigh(bool bPingTooHigh)
{
	bUseServerSideRewind = !bPingTooHigh; // 延迟过高, 禁用服务器回溯！
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AWeapon::ShowPickupWidget(bool bShowWidget)
{
	if (PickupWidget) {
		PickupWidget->SetVisibility(bShowWidget);
	}
}

void AWeapon::SetWeaponState(EWeaponState State)
{
	WeaponState = State;
	OnWeaponStateSet();
}

void AWeapon::OnWeaponStateSet()
{
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		OnEquipped();
		break;
	case EWeaponState::EWS_EquippedSecondary:
		OnEquippedSecondary();
		break;
	case EWeaponState::EWS_Dropped:
		OnDropped();
		break;
	}
}

void AWeapon::OnRep_WeaponState()
{
	/* P.S. Clients无需处理 SphereComponent 的碰撞检测 */
	OnWeaponStateSet();
}

void AWeapon::OnEquipped()
{
	ShowPickupWidget(false);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetEnableGravity(false);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WeaponOnwerCharcter = (WeaponOnwerCharcter == nullptr ? Cast<ABaseCharacter>(GetOwner()) : WeaponOnwerCharcter);
	if (WeaponOnwerCharcter && bUseServerSideRewind) { // 须开启"服务器回溯"
		WeaponOnwerController = (WeaponOnwerController == nullptr ? Cast<ABaseCharacterPlayerController>(WeaponOnwerCharcter->GetController()) : WeaponOnwerController);
		if (WeaponOnwerController && HasAuthority() && !WeaponOnwerController->HighPingDelegate.IsBound()) { // 如果未绑定回调函数, 添加
			WeaponOnwerController->HighPingDelegate.AddDynamic(this, &AWeapon::OnPingTooHigh);
		}
	}
}

void AWeapon::OnEquippedSecondary()
{
	ShowPickupWidget(false);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetEnableGravity(false);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WeaponOnwerCharcter = (WeaponOnwerCharcter == nullptr ? Cast<ABaseCharacter>(GetOwner()) : WeaponOnwerCharcter);
	if (WeaponOnwerCharcter) {
		WeaponOnwerController = (WeaponOnwerController == nullptr ? Cast<ABaseCharacterPlayerController>(WeaponOnwerCharcter->GetController()) : WeaponOnwerController);
		if (WeaponOnwerController && HasAuthority() && WeaponOnwerController->HighPingDelegate.IsBound()) { // 如果已绑定回调函数, 删去
			WeaponOnwerController->HighPingDelegate.RemoveDynamic(this, &AWeapon::OnPingTooHigh);
		}
	}
}

void AWeapon::OnDropped()
{
	if (HasAuthority()) {
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // 丢弃的Weapon可以被玩家重新拾取
	}
	WeaponMesh->SetSimulatePhysics(true); // 物理模拟: 组件的位置和旋转受物理引擎影响
	WeaponMesh->SetEnableGravity(true);   // 重力影响: 物体在进行物理模拟时，会受到世界重力并向下坠落
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // WeaponMesh参与检测和物理碰撞

	WeaponOnwerCharcter = (WeaponOnwerCharcter == nullptr ? Cast<ABaseCharacter>(GetOwner()) : WeaponOnwerCharcter);
	if (WeaponOnwerCharcter) {
		WeaponOnwerController = (WeaponOnwerController == nullptr ? Cast<ABaseCharacterPlayerController>(WeaponOnwerCharcter->GetController()) : WeaponOnwerController);
		if (WeaponOnwerController && HasAuthority() && WeaponOnwerController->HighPingDelegate.IsBound()) { // 如果已绑定回调函数, 删去
			WeaponOnwerController->HighPingDelegate.RemoveDynamic(this, &AWeapon::OnPingTooHigh);
		}
	}
}

void AWeapon::SpendRound()
{
	/** SpendRound() 虽然在Client和Server都会调用, 但它们操作的是各自机器上的Weapon副本 */
	Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity); // 弹药数减一  
	SetHUDAmmo();								   // 更新HUD弹药数

	if (HasAuthority()) {		// 如果位于Server
		ClientUpdateAmmo(Ammo); /* 服务器校正 */
	}
	else {
		/* 刚完成了一次射击, 但Server的响应还未送达 */
		++Sequence;	// 如果位于 本地Client, 尚未处理的弹药请求数量增加
	}
}

void AWeapon::ClientUpdateAmmo_Implementation(int32 ServerAmmo)
{
	if (HasAuthority()) return;
	Ammo = ServerAmmo;
	--Sequence; // 未同步次数 - 1 (已同步一条待处理的服务器请求)
	Ammo -= Sequence; /* 按已发送但未确认的请求数量修正 */
	SetHUDAmmo();
}

void AWeapon::AddAmmo(int32 AmmoToAdd)
{
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MagCapacity);
	SetHUDAmmo();
	ClientAddAmmo(AmmoToAdd); // AddAmmo() 只在Server中调用, 因此直接触发 相应Client RPC 即可
}

void AWeapon::ClientAddAmmo_Implementation(int32 AmmoToAdd)
{
	if (HasAuthority()) return;
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MagCapacity);
	SetHUDAmmo();
}


void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, WeaponState);
	// 让 AWeapon 中的 bUseServerSideRewind 参与网络复制, 但只复制给该Weapon的Owner [持有该Weapon的Character]
	DOREPLIFETIME_CONDITION(AWeapon, bUseServerSideRewind, COND_OwnerOnly);
}

void AWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();
	if (Owner == nullptr) {
		WeaponOnwerCharcter = nullptr;
		WeaponOnwerController = nullptr;
	}
	else {
		WeaponOnwerCharcter = (WeaponOnwerCharcter == nullptr ? Cast<ABaseCharacter>(Owner) : WeaponOnwerCharcter);
		if (WeaponOnwerCharcter && WeaponOnwerCharcter->GetEquippedWeapon() && WeaponOnwerCharcter->GetEquippedWeapon() == this) {
			/* 仅在 Weapon 为 Character当前手持武器 时, 才更新 HUDAmmo (否则副武器的Ammo会错误显示在HUD中) */
			SetHUDAmmo();
		}
	}
}

void AWeapon::SetHUDAmmo()
{
	WeaponOnwerCharcter = (WeaponOnwerCharcter == nullptr ? Cast<ABaseCharacter>(GetOwner()) : WeaponOnwerCharcter);
	if (WeaponOnwerCharcter) {
		WeaponOnwerController = (WeaponOnwerController == nullptr ? Cast<ABaseCharacterPlayerController>(WeaponOnwerCharcter->GetController()) : WeaponOnwerController);
		if (WeaponOnwerController) {
			WeaponOnwerController->SetHUDWeaponAmmo(Ammo);
		}
	}
}

void AWeapon::Fire(const FVector& HitTarget)
{
	if (FireAnimation) {
		WeaponMesh->PlayAnimation(FireAnimation, false); // 第二个参数bool bLooping 表示"是否循环播放动画"
	}
	if (HasAuthority()) {
		SpendRound(); // 只有 Server 才能处理 弹药消耗
	}
}

void AWeapon::Dropped()
{
	SetWeaponState(EWeaponState::EWS_Dropped); // 属性复制
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	// 解除附着时的Transform处理规则: KeepWorld表示组件与父组件解除附着时, 保持当前的世界坐标变换不变
	WeaponMesh->DetachFromComponent(DetachRules); // 解除当前组件与父组件之间的附着关系
	SetOwner(nullptr);
	WeaponOnwerCharcter = nullptr;
	WeaponOnwerController = nullptr;
}
