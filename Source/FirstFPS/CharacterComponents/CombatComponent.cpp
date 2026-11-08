// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"
#include "FirstFPS/Weapons/Weapon.h"
#include "FirstFPS/Character/BaseCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "FirstFPS/Character/BaseCharacterPlayerController.h"
#include "FirstFPS/HUD/BaseCharacterHUD.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "Sound/SoundCue.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	BaseWalkSpeed = 600;
	AimWalkSpeed = 450;
}

void UCombatComponent::PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount)
{
	/** 处理Character拾取弹药的逻辑 */
	if (CarriedAmmoMap.Contains(WeaponType)) {
		CarriedAmmoMap[WeaponType] += AmmoAmount;
		UpdateCarriedAmmo(); // 更新HUD中的 备用弹药数 显示
	}
	if (EquippedWeapon && EquippedWeapon->IsMagEmpty() && EquippedWeapon->GetWeaponType() == WeaponType) {
		// 如果 当前Weapon 并无弹药, 且 拾取的弹药 与 当前Weapon 类型相对应, 则自动装弹
		Reloading();
	}
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Character) {
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
		if (Character->GetFollowCamera()) {
			DefaultFOV = Character->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}
		if (Character->HasAuthority()) {
			InitializeCarriedAmmo();
		}
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	bAiming = bIsAiming;
	if (Character) {
		Character->GetCharacterMovement()->MaxWalkSpeed = (bIsAiming ? AimWalkSpeed : BaseWalkSpeed);
	}
	if (Character->IsLocallyControlled()) {
		bAimButtonPressed = bAiming;
	}
	ServerSetAiming(bIsAiming);
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming; // Server 将更新的 bAiming 复制给 其他Clients (变量复制)
	if (Character) {
		// 此处只修改 Server 上的权威移动速度MaxWalkSpeed (其余Clients上的对应Character副本不受影响, 因为没有必要)
		Character->GetCharacterMovement()->MaxWalkSpeed = (bIsAiming ? AimWalkSpeed : BaseWalkSpeed);
	}
}

void UCombatComponent::OnRep_Aiming()
{
	if (Character && Character->IsLocallyControlled())
		bAiming = bAimButtonPressed;
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	if (EquippedWeapon == nullptr) return;
	/* 如果武器连续开火, bPressed会一直保持为true. 数据同步仅在状态变化时才会触发. 因此使用RepNotify, 相关函数只会触发一次, 无法满足"连续开火"效果 */
	/* 解决办法: 多播RPC. 在 Server 端调用一个 NetMulticast RPC 时, 它会在 Server 和 所有Clients 上执行 */
	bFireButtonPressed = bPressed; // 让 CombatComponent 知道玩家是否按着开火键 (在Client本地设置)
	if (bFireButtonPressed) {
		Fire();
	}
}

void UCombatComponent::Fire()
{
	if (CanFire()) {
		bCanFire = false;

		/* 本地机器: 按下开火键时, 准星扩散 */
		if (EquippedWeapon) {
			CrosshairShootingFactor = 20.f;
		}

		/* 参数HitTarget: 用于传入Line Trace目标点 */
		LocalFire(HitTarget);  // 本地开火效果: 让开火反馈立即发生(Character开火动作;Weapon开火动画), 无需通过Multicast返回来执行
		ServerFire(HitTarget); // 让处于 Server 或 未控制该Character的Clients 的Character, 播放开火特效

		StartFireTimer(); // 给 FireTimer 注册一个计时器
	}
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	MulticastFire(TraceHitTarget); // P.S. 所有机器均使用Server传来的同一个TraceHitTarget!
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	/* LocalFire() 在 本地Clinet控制的Character 上已经执行过. 因此 Server Multicast 时, 无需重复执行 (防止开火特效播放两次) */
	if (Character && Character->IsLocallyControlled() && !Character->HasAuthority()) return;
	
	/* 处于 Server 或 未控制该Character的Clients 中, 触发LocalFire() */
	LocalFire(TraceHitTarget);
}

void UCombatComponent::LocalFire(const FVector_NetQuantize& TraceHitTarget)
{
	if (Character && EquippedWeapon && CombatState == ECombatState::ECS_Unoccupied) {
		Character->PlayFireMontage(bAiming);  // 播放 Charcater 的开火动作
		EquippedWeapon->Fire(TraceHitTarget); // 播放 Weapon 的开火动画, 处理弹药消耗
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	/* 仅在本地机器处理 */
	if (Character && Character->IsLocallyControlled()) {
		/* 每帧在准星位置进行Line Trace, 并记录其目标点位置HitTarget */
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;
		
		SetHUDCrosshairs(DeltaTime);
		InterpFOV(DeltaTime);
	}
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, SecondaryWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly); // 仅复制给控制该类的Client, 以节省带宽
	DOREPLIFETIME(UCombatComponent, CombatState);
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (Character == nullptr || WeaponToEquip == nullptr) return;
	if (CombatState != ECombatState::ECS_Unoccupied) return;

	if (EquippedWeapon != nullptr && SecondaryWeapon == nullptr) { // Character 已有主武器, 但没有副武器
		EquipSecondaryWeapon(WeaponToEquip);
	}
	else { // Character 没有装备任何武器 or 主副武器皆已装备
		EquipPrimaryWeapon(WeaponToEquip);
	}
}

void UCombatComponent::SwapWeapons()
{
	if (CombatState != ECombatState::ECS_Unoccupied) return; // 可选: 装填状态时, 禁止切换武器

	AWeapon* TempWeapon = EquippedWeapon;
	EquippedWeapon = SecondaryWeapon;
	SecondaryWeapon = TempWeapon;

	/* 主武器 */
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	AttachActorToRightHand(EquippedWeapon);
	EquippedWeapon->SetHUDAmmo();
	UpdateCarriedAmmo();
	PlayEquipWeaponSound(EquippedWeapon);

	/* 副武器 */
	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
	AttacgActorToBack(SecondaryWeapon);
}

void UCombatComponent::EquipPrimaryWeapon(AWeapon* WeaponToEquip)
{
	DropEquippedWeapon();
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	AttachActorToRightHand(EquippedWeapon);
	/** 每个 Actor 都可以设置 Owner. Owner会影响 该Actor 的 Server RPC、OwnerOnly 变量复制... */
	EquippedWeapon->SetOwner(Character); // Server 调用 SetOwner() 后，Owner 状态可以自动复制
	/* 装备新Weapon时, 更新HUD弹药数显示 */
	EquippedWeapon->SetHUDAmmo();
	UpdateCarriedAmmo();
	PlayEquipWeaponSound(EquippedWeapon);
	/* 自动换弹: 装备Weapon后, 如果Weapon没有子弹, 执行Reloading() */
	if (EquippedWeapon->IsMagEmpty()) {
		Reloading();
	}
}

void UCombatComponent::EquipSecondaryWeapon(AWeapon* WeaponToEquip)
{
	SecondaryWeapon = WeaponToEquip;
	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
	AttacgActorToBack(SecondaryWeapon);
	SecondaryWeapon->SetOwner(Character);
	PlayEquipWeaponSound(SecondaryWeapon);
}

void UCombatComponent::DropEquippedWeapon()
{
	if (EquippedWeapon) {
		EquippedWeapon->Dropped(); // 若装备新Weapon时, 已装备Weapon, 先丢弃原有Weapon
	}
}

void UCombatComponent::AttachActorToRightHand(AActor* ActorToAttach)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || ActorToAttach == nullptr) return;
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket) {
		/** 当 Attach 发生在 Server 的 replicated Actor 上时, UE会把 该Actor 的 AttachmentReplication附着状态 同步到 Clients */
		HandSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

void UCombatComponent::AttacgActorToBack(AActor* ActorToAttach)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || ActorToAttach == nullptr) return;
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("BackSocket"));
	if (HandSocket) {
		/** 当 Attach 发生在 Server 的 replicated Actor 上时, UE会把 该Actor 的 AttachmentReplication附着状态 同步到 Clients */
		HandSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

void UCombatComponent::PlayEquipWeaponSound(AWeapon* WeaponToEquip)
{
	// 播放装备音效
	if (Character && WeaponToEquip && WeaponToEquip->EquipSound) {
		UGameplayStatics::PlaySoundAtLocation(this, WeaponToEquip->EquipSound, Character->GetActorLocation());
	}
}

void UCombatComponent::Reloading()
{
	if (CarriedAmmo > 0 && CombatState != ECombatState::ECS_Reloading && !bLocallyReloading) {
		bLocallyReloading = true; // 本地防重入标志: 延迟过高时, 避免CombatState未能及时同步到本地客户端(仍为Unoccupied)而重复装填动作
		HandleReloading();		  // 客户端预测: 本地机器立刻执行PlayReloadMontage()
		ServerReloading();
	}
}

void UCombatComponent::ServerReloading_Implementation()
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	CombatState = ECombatState::ECS_Reloading; // 只有Server才能修改CombatState
	if (!Character->IsLocallyControlled()) {
		HandleReloading(); // 避免 Listen Server上控制的角色 重复执行装填操作
	}
}

void UCombatComponent::FinishReloading()
{
	if (Character == nullptr) return;
	bLocallyReloading = false;
	if (Character->HasAuthority()) {
		CombatState = ECombatState::ECS_Unoccupied;
		UpdateAmmoValues();
	}
	if (bFireButtonPressed) {
		Fire(); // 如果一直按住开火键, 换弹完成继续持续射击 (因为 FireButtonPressed(bool bPressed) 只在 bPressed 被改变时调用)
	}
}

void UCombatComponent::UpdateAmmoValues()
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	int32 ReloadAmount = AmountToReload();
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType())) {
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	Controller = (Controller == nullptr ? Cast<ABaseCharacterPlayerController>(Character->GetController()) : Controller);
	if (Controller) {
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
	EquippedWeapon->AddAmmo(ReloadAmount);
}

bool UCombatComponent::ShouldSwapWeapons()
{
	return (EquippedWeapon != nullptr && SecondaryWeapon != nullptr);
}

void UCombatComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
	case ECombatState::ECS_Unoccupied:
		if (bFireButtonPressed) {
			Fire();
		}
		break;
	case ECombatState::ECS_Reloading:
		if (!Character->IsLocallyControlled()) HandleReloading(); // 本地机器无需重复执行装填操作
		break;
	case ECombatState::ECS_MAX:
		break;
	}
}

void UCombatComponent::HandleReloading()
{
	Character->PlayReloadMontage();
}

int32 UCombatComponent::AmountToReload()
{
	if (EquippedWeapon == nullptr) return 0;
	int32 RoomInMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo(); // 计算Weapon还可装多少弹药
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType())) {
		int32 AmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		return FMath::Clamp(RoomInMag, 0, AmountCarried); // 不能超过自身携带的该类型弹药
	}
	return 0;
}

void UCombatComponent::UpdateCarriedAmmo()
{
	if (EquippedWeapon == nullptr) return;

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType())) {
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	Controller = (Controller == nullptr ? Cast<ABaseCharacterPlayerController>(Character->Controller) : Controller);
	if (Controller) {
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	/** 获取准星(屏幕中心)的世界坐标和方向 */
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport) {					  // GEngine->GameViewport: 当前游戏窗口的管理对象
		GEngine->GameViewport->GetViewportSize(ViewportSize); // 获取当前游戏画面的宽和高(结果存入参数ViewportSize)
		// e.g. 窗口分辨率为 1920 x 1080, 则 ViewportSize.X = 1920, ViewportSize.Y = 1080
	}
	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f); // 将 屏幕中心点 作为 准星位置
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;
	// DeprojectScreenToWorld(): 把 屏幕上的2D坐标 转换成 当对应3D世界坐标和方向. 如果转换失败, 则返回false
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0), // 当前玩家(玩家0)控制器
		CrosshairLocation,		// 输入: 屏幕二维坐标点
		CrosshairWorldPosition,	// 输出: 屏幕点对应的"世界坐标"
		CrosshairWorldDirection	// 输出: 屏幕点指向的"世界方向"(单位向量)
	);
	/** 线性追踪Line Trace */
	if (bScreenToWorld) { // 如果转换成功(已获取屏幕中心的世界坐标和方向)
		FVector Start = CrosshairWorldPosition;
		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH; // TRACE_LENGTH: 宏定义的一个较大数值, 用于延长Trace终点
		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult, // Line Trace结果
			Start,
			End,
			ECC_Visibility  // Trace使用的碰撞通道
		);
		if (!TraceHitResult.bBlockingHit) { // 如果 Line Trace 没有命中任何目标, 将 目标点 设为Trace终点
			TraceHitResult.ImpactPoint = End;
		}
	}
}

void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
	if (Character == nullptr || Character->GetController() == nullptr) return;

	/* 存储 Controller 和 HUD 的指针, 避免每帧调用时反复读取 */
	Controller = (Controller == nullptr ? Cast<ABaseCharacterPlayerController>(Character->GetController()) : Controller);
	if (Controller) {
		HUD = (HUD == nullptr ? Cast<ABaseCharacterHUD>(Controller->GetHUD()) : HUD);
		if (HUD) {
			/* 设置每帧要显示的准星纹理 */
			FHUDPackage HUDPackage;
			if (EquippedWeapon) {
				HUDPackage.CrosshairsCenter = EquippedWeapon->CrosshairsCenter;
				HUDPackage.CrosshairsLeft = EquippedWeapon->CrosshairsLeft;
				HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairsRight;
				HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairsTop;
				HUDPackage.CrosshairsBottom = EquippedWeapon->CrosshairsBottom;
			}
			else { // 未装备武器, 不显示准星
				HUDPackage.CrosshairsCenter = nullptr;
				HUDPackage.CrosshairsLeft = nullptr;
				HUDPackage.CrosshairsRight = nullptr;
				HUDPackage.CrosshairsTop = nullptr;
				HUDPackage.CrosshairsBottom = nullptr;
			}
			/** 计算准星扩散值: 受角色移动速度、是否在空中、是否瞄准、是否射击影响 */
			// 移动速度对准星扩散的影响: 将当前水平速度映射到[0, 2], 速度越快, CrosshairVelocityFactor越大, 准星扩散越明显
			FVector2D WalkSpeedRange(0.f, Character->GetCharacterMovement()->MaxWalkSpeed);
			FVector2D VelocityMultiplierRange(0.f, 2.f);
			FVector Velocity = Character->GetVelocity();
			Velocity.Z = 0.f;
			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());
			// 空中状态对准星扩散的影响
			if (Character->GetCharacterMovement()->IsFalling()) {
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
			}
			else {
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
			}
			// 瞄准状态对准星扩散的影响: 瞄准时准星逐渐收缩
			if (bAiming) {
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 1.f, DeltaTime, 30.f);
			}
			else {
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
			}
			// 射击对准星扩散的影响: 开火时将 CrosshairShootingFactor 设置为一个较大的值, 否则每帧让该值平滑恢复到0
			CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 5.f);

			HUDPackage.CrosshairSpread = 0.5f + CrosshairVelocityFactor + CrosshairInAirFactor - CrosshairAimFactor + CrosshairShootingFactor;
			HUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (EquippedWeapon == nullptr) return;

	/** FMath::FInterpTo(Current, Target, DeltaTime, InterpSpeed): 让一个 float值 从 当前值Current, 平滑地靠近目标值Target */
	if (bAiming) { // 瞄准时, 缩小FOV
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	else {	// 未瞄准, FOV恢复默认值
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
	}

	if (Character && Character->GetFollowCamera()) {
		Character->GetFollowCamera()->SetFieldOfView(CurrentFOV); // 缩放 Camera 的 FOV
	}
}

bool UCombatComponent::CanFire()
{
	return !EquippedWeapon->IsMagEmpty() && bCanFire && CombatState == ECombatState::ECS_Unoccupied && !bLocallyReloading;
}

void UCombatComponent::StartFireTimer()
{
	if (EquippedWeapon == nullptr || Character == nullptr) return;

	if (Character) {
		// 给 FireTimer 注册一个计时器, 等 FireDelay 秒后, 在 当前对象(this) 上调用 FireTimerFinished()
		Character->GetWorldTimerManager().SetTimer(FireTimer, this, &UCombatComponent::FireTimerFinished, EquippedWeapon->FireDelay);
	}
}

void UCombatComponent::FireTimerFinished()
{
	if (EquippedWeapon == nullptr) return;

	bCanFire = true;
	if (bFireButtonPressed && EquippedWeapon->bAutomatic) { // 如果开火键仍然被按下 且 Weapon支持连发功能
		Fire();
	}
	/* 自动换弹: 如果开火结束后, Weapon已没有子弹, 执行Reloading() */
	if (EquippedWeapon->IsMagEmpty()) {
		Reloading();
	}
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	Controller = (Controller == nullptr ? Cast<ABaseCharacterPlayerController>(Character->GetController()) : Controller);
	if (Controller) {
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
}

void UCombatComponent::InitializeCarriedAmmo()
{
	// TMap::Emplace(Key, Value) 与 TMap::Add(Key, Value) 都用于添加或更新键值对
	// Emplace() 的特点是尝试直接在容器内部构造 键值对(Key, Value), 减少临时对象的创建
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssultRifle, StartingARAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, StartingRocketAmmo);
}

/*
void UCombatComponent::OnRep_EquippedWeapon()
{
	// 不同复制属性对应的 OnRep 回调之间没有确定执行顺序，客户端的回调顺序与服务器修改顺序、属性声明顺序均没有可靠关系
	// 服务器产生的 WeaponState 和 Attachment 同步依然会到达客户端。OnRep_EquippedWeapon() 只是额外执行一次本地状态校正
	if (Character && EquippedWeapon) {
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
		if (HandSocket) {
			HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
		}
	}
	// 播放装备音效
	if (EquippedWeapon->EquipSound) {
		UGameplayStatics::PlaySoundAtLocation(this, EquippedWeapon->EquipSound, Character->GetActorLocation());
	}
}
*/