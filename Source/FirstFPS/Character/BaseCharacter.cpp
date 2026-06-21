// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "FirstFPS/Weapons/Weapon.h"
#include "FirstFPS/CharacterComponents/CombatComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "BaseCharacterAnimInstance.h"
#include "FirstFPS/FirstFPS.h"
#include "FirstFPS/Character/BaseCharacterPlayerController.h"
#include "FirstFPS/GameModes/CombatGameMode.h"
#include "TimerManager.h"
#include "Components/CapsuleComponent.h"

ABaseCharacter::ABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn; // 尽量避开碰撞，但保证Actor一定会被生成
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent); //
	CameraBoom->TargetArmLength = 400.f;
	CameraBoom->bUsePawnControlRotation = true;
	//CameraBoom->SocketOffset = FVector(0.f, 60.f, 20.f); // 摄像机不正对角色背后中心,而是稍微偏右肩

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // SpringArm末端挂载点(USpringArmComponent已定义)
	FollowCamera->bUsePawnControlRotation = false;

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true); // 开启"Component Replication组件复制"

	/** 四方向持枪移动: 角色身体朝向Controller/摄像机/准星方向, 不再朝移动方向自动转身 */
	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true; // 允许 该Character 进入蹲伏状态
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	UpdateHUDHealth();

	if (HasAuthority()) { // 仅由 Server 处理伤害
		OnTakeAnyDamage.AddDynamic(this, &ABaseCharacter::ReceiveDamage);

		SpawnDefaultWeapon(); //
	}
}

void ABaseCharacter::UpdateHUDHealth()
{
	BaseCharacterPlayerController = (BaseCharacterPlayerController == nullptr ? Cast<ABaseCharacterPlayerController>(Controller) : BaseCharacterPlayerController);  // Controller: xxx
	if (BaseCharacterPlayerController) {
		BaseCharacterPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AimOffset(DeltaTime);
}

void ABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABaseCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABaseCharacter::Look);
		/** ACharacter::Jump(): ACharacter自带的跳跃开始函数, 负责发出"角色想跳"的请求 */ 
		/** 在 CharacterMovementComponent 下一次处理移动时, 判断角色是否能跳. 如果满足跳跃条件则为角色提供一个向上的速度(JumpZVelocity) */
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		EnhancedInputComponent->BindAction(EquipAction, ETriggerEvent::Started, this, &ABaseCharacter::Equip);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &ABaseCharacter::CrouchButtonPressed);
		/* 按住右键瞄准, 松开右键退出瞄准 */
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &ABaseCharacter::AimButtonPressed);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &ABaseCharacter::AimButtonReleased);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Canceled, this, &ABaseCharacter::AimButtonReleased);

		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ABaseCharacter::FireButtonPressed);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &ABaseCharacter::FireButtonReleased);
	}
}

void ABaseCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (Combat) {
		Combat->Character = this; // 此时Actor(this)和其组件(Combat)都已创建并初始化完成, 因此可建立 Combat->Character = this 这种引用关系 
	}
}

void ABaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	/** 将 已声明的可复制变量(Replicated/ReplicatedUsing修饰) 注册到复制列表, 并设置复制条件 */
	// 正常复制给所有相关Client:	  DOREPLIFETIME(类名, 变量名)
	// 只复制给满足条件的相关Client: DOREPLIFETIME_CONDITION(类名, 变量名, 条件宏)
	DOREPLIFETIME_CONDITION(ABaseCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ABaseCharacter, Health);
}

void ABaseCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>(); 
	DoMove(MovementVector.X, MovementVector.Y);
}

void ABaseCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void ABaseCharacter::Equip(const FInputActionValue& Value)
{
	bool bEquipButtonPressed = Value.Get<bool>();
	if (bEquipButtonPressed && Combat) {
		if (HasAuthority()) {	// 只有 Server 拥有Equip权限
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else {
			ServerEquip();		// 请求 Server 执行 ServerEquip_Implementation()函数
		}
	}
}

void ABaseCharacter::CrouchButtonPressed()
{
	/** ACharacter::Crouch(): ACharacter自带的蹲伏状态函数, 负责发出"角色蹲下"的请求 */
	/** 在 CharacterMovementComponent 下一次处理移动时, 判断角色是否能蹲. 如果满足条件则调整胶囊体大小、移动速度, 更新bIsCrouched复制变量... */
	if (bIsCrouched) {
		UnCrouch(); // 请求角色停止蹲伏
	}
	else {
		Crouch();
	}
}

void ABaseCharacter::AimButtonPressed()
{
	if (Combat) {
		Combat->SetAiming(true);
	}
}

void ABaseCharacter::AimButtonReleased()
{
	if (Combat) {
		Combat->SetAiming(false);
	}
}

void ABaseCharacter::FireButtonPressed()
{
	if (Combat) {
		Combat->FireButtonPressed(true);
	}
}

void ABaseCharacter::FireButtonReleased()
{
	if (Combat) {
		Combat->FireButtonPressed(false);
	}
}

void ABaseCharacter::AimOffset(float DeltaTime)
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	float Speed = Velocity.Size();
	bool bIsInAir = GetCharacterMovement()->IsFalling();

	/** Character静止不动, 且不在空中(站在地面上): 玩家左右转镜头, 角色身体基本不动, 上半身随Controller(View方向)扭动 */
	if (Speed == 0.f && !bIsInAir) {
		bUseControllerRotationYaw = false; /* 角色静止不动时: 角色身体不跟随 Controller Yaw 旋转 */

		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f); // 当前Controller的Yaw方向
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw; // 角色静止时. 玩家转动View, AO_Yaw 会变化
	}
	/** Character跑动/跳跃: 角色移动时不额外左右扭动上半身, 同时把不断刷新参考方向 */
	if (Speed > 0.f || bIsInAir) {
		bUseControllerRotationYaw = true; // 角色身体跟随 Controller Yaw 转动

		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f); // 角色静止不动前, Controller的Yaw方向 (作为AO_Yaw的参考/起始方向)
		AO_Yaw = 0.f; /* 移动时, 不再让上半身随View方向左右偏移 */
	}
	/** Character任何状态: 玩家上下转镜头, 角色上半身抬枪/压枪 */
	AO_Pitch = GetBaseAimRotation().Pitch;  
	/* P.S. 在多人游戏中, AO_Pitch 需要特殊处理: 在远端客户端的其他玩家(Character副本)中, GetBaseAimRotation().Pitch 可能范围是[270, 360] */
	/* 因为 远端客户端 没有 该角色的PlayerController, 只能通过 APawn 提供的 GetRemoteViewPitch() 获得Pitch复制值(被压缩成16bits!) */
	/* 还原该压缩值时, 返回值范围是[0, 360] e.g. -60在 远端客户端的非控制Character 中会返回300 */
	if (AO_Pitch > 90.f && !IsLocallyControlled()) {
		/* 将返回的 [270, 360) 重新映射为 [-90, 0) */ 
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void ABaseCharacter::ReceiveDamage(AActor* DamageActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
	/* Server调用 */
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
	UpdateHUDHealth();
	PlayeHitReactMontage();

	if (Health == 0.f) {
		// 返回当前 GameMode 实例, 并转换成模板指定类型.		P.S. 只能在Server上返回有效指针, Client上可能返回 nullptr
		ACombatGameMode* CombatGameMode = GetWorld()->GetAuthGameMode<ACombatGameMode>();
		if (CombatGameMode) {
			BaseCharacterPlayerController = (BaseCharacterPlayerController == nullptr ? Cast<ABaseCharacterPlayerController>(Controller) : BaseCharacterPlayerController);
			ABaseCharacterPlayerController* AttackController = Cast<ABaseCharacterPlayerController>(InstigatorController);
			CombatGameMode->PlayerEliminated(this, BaseCharacterPlayerController, AttackController);
		}
	}
}

void ABaseCharacter::SpawnDefaultWeapon()
{
	if (!HasAuthority())
	{
		return;
	}

	if (!Combat || !DefaultWeaponClass || !GetWorld())
	{
		return;
	}

	static const FName RightHandSocketName(TEXT("RightHandSocket"));

	FTransform SpawnTransform = GetActorTransform();

	// 如果 Socket 存在，直接在右手 Socket 的世界坐标生成。
	if (GetMesh() &&
		GetMesh()->DoesSocketExist(RightHandSocketName))
	{
		SpawnTransform = GetMesh()->GetSocketTransform(
			RightHandSocketName,
			RTS_World
		);
	}

	FActorSpawnParameters SpawnParameters;

	// 武器的网络拥有链：
	// PlayerController -> Character -> Weapon
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;

	// 默认武器生成后会立即附加，因此允许无视初始碰撞生成。
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AWeapon* StartingWeapon =
		GetWorld()->SpawnActor<AWeapon>(
			DefaultWeaponClass,
			SpawnTransform,
			SpawnParameters
		);

	Combat->EquipWeapon(StartingWeapon);
}

void ABaseCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr) {
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		// 根据玩家控制器的水平朝向(Yaw), 计算出"前方方向"和"右方方向"
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		/* AddMovementInput: 继承自APawn的函数, 用于给 Pawn/Character 添加"移动意图"(这一帧想往哪个方向移动, 移动力度是多少) */
		/* 将移动方向和移动力度传给 Pawn 后, 由 MovementComponent 根据速度、加速度、碰撞、重力等规则真正移动角色 */
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void ABaseCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr) {
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ABaseCharacter::DoJumpStart()
{
	Jump();
}

void ABaseCharacter::DoJumpEnd()
{
	StopJumping();
}

void ABaseCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	if (OverlappingWeapon) { // 每次更新OverlappingWeapon, 先隐藏之前的拾取控件
		OverlappingWeapon->ShowPickupWidget(false);
	}
	OverlappingWeapon = Weapon;
	if (IsLocallyControlled()) { // 仅由本地机器(此处为Listen Server)控制的Actor会显示拾取控件
		if (OverlappingWeapon) {
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

void ABaseCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (LastWeapon) { // 同上
		LastWeapon->ShowPickupWidget(false);
	}
	if (OverlappingWeapon) {
		OverlappingWeapon->ShowPickupWidget(true);
	}
}

void ABaseCharacter::OnRep_Health()
{
	/* 所有Clients调用 */
	UpdateHUDHealth();
	PlayeHitReactMontage(); // 优化: 取代原有的MulticastHit()函数逻辑(多播RPC), 因为 属性复制/通知 比 RPC 要节约带宽 
}

void ABaseCharacter::ServerEquip_Implementation()
{
	/** ServerEquip() 是被UE网络系统包装过的RPC调用入口. 此处定义的 ServerEquip_Implementation() 是在 Server端 的执行逻辑 */
	if (Combat) {
		Combat->EquipWeapon(OverlappingWeapon);
	}
}

bool ABaseCharacter::IsWeaponEquipped()
{
	return (Combat && Combat->EquippedWeapon);
}

bool ABaseCharacter::IsAiming()
{
	return (Combat && Combat->bAiming);
}

void ABaseCharacter::PlayFireMontage(bool bAiming)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireMontage) {
		AnimInstance->Montage_Play(FireMontage);
		FName SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABaseCharacter::Elim()
{
	/** 仅 Server 调用, 因为 GameMode 只在Server中存在 */
	if (Combat && Combat->EquippedWeapon) {
		Combat->EquippedWeapon->Dropped();
	}
	MulticastElim();
	// 设置ElimTimer: 计时器结束后, 触发角色重生
	GetWorldTimerManager().SetTimer(ElimTimer, this, &ABaseCharacter::ElimTimerFinished, ElimDelay);
}

void ABaseCharacter::MulticastElim_Implementation()
{
	PlayDeathMontage(IsAiming());
	GetCharacterMovement()->DisableMovement();		   // 禁用 CharacterMovement 移动功能 (不再执行行走、跳跃等移动)
	GetCharacterMovement()->StopMovementImmediately(); // 立即清零CharacterMovement当前速度和加速度, 让角色立刻停下来
	if (BaseCharacterPlayerController) {
		DisableInput(BaseCharacterPlayerController); // 停止接收 对应PlayerController 的输入
	}
	/* 禁用碰撞 */
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABaseCharacter::ElimTimerFinished()
{
	ACombatGameMode* CombatGameMode = GetWorld()->GetAuthGameMode<ACombatGameMode>();
	if (CombatGameMode) {
		CombatGameMode->RequestRespawn(this, Controller); // 玩家重生(仅在Server执行)
	}
}

void ABaseCharacter::PlayDeathMontage(bool bAiming)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && DeathMontage) {
		AnimInstance->Montage_Play(DeathMontage);
		FName SectionName = bAiming ? FName("DeathAim") : FName("DeathHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABaseCharacter::PlayeHitReactMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage) {
		AnimInstance->Montage_Play(HitReactMontage);
		FName SectionName("FromFront");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}
