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

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	BaseWalkSpeed = 600;
	AimWalkSpeed = 450;
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
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	bAiming = bIsAiming;		/** 这一步发生在当前本地机器. 所以 Clinet 不需要等 Server 数据返回, 就能立刻看到瞄准动作 */
	if (Character) {
		Character->GetCharacterMovement()->MaxWalkSpeed = (bIsAiming ? AimWalkSpeed : BaseWalkSpeed);
	}
	ServerSetAiming(bIsAiming); /** 发送 Server RPC, 将状态变量的修改发送到 Server */
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
	if (bCanFire) {
		bCanFire = false;

		/* 本地机器: 按下开火键时, 准星扩散 */
		if (EquippedWeapon) {
			CrosshairShootingFactor = 20.f;
		}
		/* Server: 处理开火逻辑, 传入Line Trace目标点 */
		ServerFire(HitTarget); // 播放 Charcater的开火动作 和 Weapon的开火动画

		StartFireTimer(); // 给 FireTimer 注册一个计时器
	}
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	MulticastFire(TraceHitTarget); // Server 和 所有Clients 播放开火表现	P.S. 所有机器均使用Server传来的同一个TraceHitTarget!
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (Character && EquippedWeapon) {
		Character->PlayFireMontage(bAiming);  // 播放 Charcater 的开火动作
		EquippedWeapon->Fire(TraceHitTarget); // 播放 Weapon 的开火动画
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming; // Server 将更新的 bAiming 复制给 其他Clients (变量复制)
	if (Character) {
		// 此处只修改 Server 上的权威移动速度MaxWalkSpeed (其余Clients上的对应Character副本不受影响, 因为没有必要)
		Character->GetCharacterMovement()->MaxWalkSpeed = (bIsAiming ? AimWalkSpeed : BaseWalkSpeed); 
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
	DOREPLIFETIME(UCombatComponent, bAiming);
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (Character == nullptr || WeaponToEquip == nullptr) return;

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket) {
		/** 当 Attach 发生在 Server 的 replicated Actor 上时, UE会把 该Actor 的 AttachmentReplication附着状态 同步到 Clients */
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
	}
	/** 每个 Actor 都可以设置 Owner. Owner会影响 该Actor 的 Server RPC、OwnerOnly 变量复制... */
	EquippedWeapon->SetOwner(Character); // Server 调用 SetOwner() 后，Owner 状态可以自动复制
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
}
*/