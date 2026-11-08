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
#include "FirstFPS/CharacterComponents/BuffComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "BaseCharacterAnimInstance.h"
#include "FirstFPS/FirstFPS.h"
#include "FirstFPS/Character/BaseCharacterPlayerController.h"
#include "FirstFPS/GameModes/CombatGameMode.h"
#include "TimerManager.h"
#include "Components/CapsuleComponent.h"
#include "FirstFPS/PlayerState/BaseCharacterPlayerState.h"
#include "FirstFPS/Weapons/WeaponTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Components/BoxComponent.h"
#include "FirstFPS/CharacterComponents/LagCompensationComponent.h"
#include "FirstFPS/GameModes/LobbyGameMode.h"

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

	Buff = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffComponent"));
	Buff->SetIsReplicated(true); // 开启"Component Replication组件复制"

	LagCompensation = CreateDefaultSubobject<ULagCompensationComponent>(TEXT("LagCompensationComponent"));
	// LagCompensation->SetIsReplicated(true); 仅在Server中使用, 无需设置为"组件复制"

	/** 四方向持枪移动: 角色身体朝向Controller/摄像机/准星方向, 不再朝移动方向自动转身 */
	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true; // 允许 该Character 进入蹲伏状态

	/** 命中判定盒: 用于服务器回溯的射击判定 */
	head = CreateDefaultSubobject<UBoxComponent>(TEXT("head"));
	head->SetupAttachment(GetMesh(), FName("head")); // 把 head(SceneComponent) 挂载到当前角色的Mesh上, 并附加在名称为"head"的Socket/Bone上
	HitCollisionBoxes.Add(FName("head"), head);

	pelvis = CreateDefaultSubobject<UBoxComponent>(TEXT("pelvis"));
	pelvis->SetupAttachment(GetMesh(), FName("pelvis"));
	HitCollisionBoxes.Add(FName("pelvis"), pelvis);

	spine_02 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_02"));
	spine_02->SetupAttachment(GetMesh(), FName("spine_02"));
	HitCollisionBoxes.Add(FName("spine_02"), spine_02);

	spine_03 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_03"));
	spine_03->SetupAttachment(GetMesh(), FName("spine_03"));
	HitCollisionBoxes.Add(FName("spine_03"), spine_03);

	upperarm_l = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_l"));
	upperarm_l->SetupAttachment(GetMesh(), FName("upperarm_l"));
	HitCollisionBoxes.Add(FName("upperarm_l"), upperarm_l);

	upperarm_r = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_r"));
	upperarm_r->SetupAttachment(GetMesh(), FName("upperarm_r"));
	HitCollisionBoxes.Add(FName("upperarm_r"), upperarm_r);

	lowerarm_l = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_l"));
	lowerarm_l->SetupAttachment(GetMesh(), FName("lowerarm_l"));
	HitCollisionBoxes.Add(FName("lowerarm_l"), lowerarm_l);

	lowerarm_r = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_r"));
	lowerarm_r->SetupAttachment(GetMesh(), FName("lowerarm_r"));
	HitCollisionBoxes.Add(FName("lowerarm_r"), lowerarm_r);

	hand_l = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_l"));
	hand_l->SetupAttachment(GetMesh(), FName("hand_l"));
	HitCollisionBoxes.Add(FName("hand_l"), hand_l);

	hand_r = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_r"));
	hand_r->SetupAttachment(GetMesh(), FName("hand_r"));
	HitCollisionBoxes.Add(FName("hand_r"), hand_r);

	thigh_l = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_l"));
	thigh_l->SetupAttachment(GetMesh(), FName("thigh_l"));
	HitCollisionBoxes.Add(FName("thigh_l"), thigh_l);

	thigh_r = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_r"));
	thigh_r->SetupAttachment(GetMesh(), FName("thigh_r"));
	HitCollisionBoxes.Add(FName("thigh_r"), thigh_r);

	calf_l = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_l"));
	calf_l->SetupAttachment(GetMesh(), FName("calf_l"));
	HitCollisionBoxes.Add(FName("calf_l"), calf_l);

	calf_r = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_r"));
	calf_r->SetupAttachment(GetMesh(), FName("calf_r"));
	HitCollisionBoxes.Add(FName("calf_r"), calf_r);

	foot_l = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_l"));
	foot_l->SetupAttachment(GetMesh(), FName("foot_l"));
	HitCollisionBoxes.Add(FName("foot_l"), foot_l);

	foot_r = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_r"));
	foot_r->SetupAttachment(GetMesh(), FName("foot_r"));
	HitCollisionBoxes.Add(FName("foot_r"), foot_r);

	/* 这些 HitBox 完全不参与碰撞; 只有服务器做回溯命中验证时, 才临时打开, 并且只响应专用的 ECC_HitBox Channel */
	for (auto Box : HitCollisionBoxes) {
		if (Box.Value) {
			Box.Value->SetCollisionObjectType(ECC_HitBox);
			Box.Value->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			Box.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
			Box.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 关闭所有 HitBoxes 的碰撞
		}
	}
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	SpawnDefaultWeapon();

	UpdateHUDHealth();
	UpdateHUDShield();
	UpdateHUDAmmo();

	if (HasAuthority()) { // 仅由 Server 处理伤害
		OnTakeAnyDamage.AddDynamic(this, &ABaseCharacter::ReceiveDamage);
	}
}

void ABaseCharacter::SpawnDefaultWeapon()
{
	/*
	ACombatGameMode* CombatGameMode = Cast<ACombatGameMode>(UGameplayStatics::GetGameMode(this));
	if (CombatGameMode && GetWorld() && !bElimmed && DefaultWeaponClass) { // 仅在进入 CombatGameMode 时, Character才会生成默认武器
		AWeapon* StartingWeapon = GetWorld()->SpawnActor<AWeapon>(DefaultWeaponClass);
		StartingWeapon->bDestroyWeapon = true; // 对于默认武器, 将其设为"持有该Weapon的角色死亡时, 销毁该Weapon"
		if (Combat) {
			Combat->EquipWeapon(StartingWeapon);
		}
	}
	*/

	// 默认武器只由服务器生成。
	if (!HasAuthority() || !GetWorld() || bElimmed || !Combat || !DefaultWeaponClass) {
		return;
	}
	// 避免重复生成。
	if (GetEquippedWeapon()) {
		return;
	}

	AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();

	const bool bAllowedGameMode = Cast<ACombatGameMode>(GameMode) != nullptr || Cast<ALobbyGameMode>(GameMode) != nullptr;
	if (!bAllowedGameMode) {
		return;
	}

	AWeapon* StartingWeapon = GetWorld()->SpawnActor<AWeapon>(DefaultWeaponClass);

	if (!StartingWeapon) {
		return;
	}

	StartingWeapon->bDestroyWeapon = true;
	Combat->EquipWeapon(StartingWeapon);
}

void ABaseCharacter::UpdateHUDHealth()
{
	BaseCharacterPlayerController = (BaseCharacterPlayerController == nullptr ? Cast<ABaseCharacterPlayerController>(Controller) : BaseCharacterPlayerController);  // Controller: xxx
	if (BaseCharacterPlayerController) {
		BaseCharacterPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void ABaseCharacter::UpdateHUDShield()
{
	BaseCharacterPlayerController = (BaseCharacterPlayerController == nullptr ? Cast<ABaseCharacterPlayerController>(Controller) : BaseCharacterPlayerController);  // Controller: xxx
	if (BaseCharacterPlayerController) {
		BaseCharacterPlayerController->SetHUDShield(Shield, MaxShield);
	}
}

void ABaseCharacter::UpdateHUDAmmo()
{
	BaseCharacterPlayerController = (BaseCharacterPlayerController == nullptr ? Cast<ABaseCharacterPlayerController>(Controller) : BaseCharacterPlayerController);  // Controller: xxx
	if (BaseCharacterPlayerController && Combat && Combat->EquippedWeapon) {
		BaseCharacterPlayerController->SetHUDWeaponAmmo(Combat->EquippedWeapon->GetAmmo());
		BaseCharacterPlayerController->SetHUDCarriedAmmo(Combat->CarriedAmmo);
	}
}

void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AimOffset(DeltaTime);
	PollInit(); // 如果 PlayerState 已同步 → 缓存 PlayerState 变量 → 刷新HUD
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

		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &ABaseCharacter::ReloadButtonPressed);
	}
}

void ABaseCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	/** 此时Actor(this)和其组件(Combat,Buff...)都已创建并初始化完成, 因此可建立 Combat->Character = this 这种引用关系 */
	if (Combat) {
		Combat->Character = this; 
	}
	if (Buff) {
		Buff->Character = this;
		Buff->SetInitialSpeed(GetCharacterMovement()->MaxWalkSpeed, GetCharacterMovement()->MaxWalkSpeedCrouched);
		Buff->SetInitialJump(GetCharacterMovement()->JumpZVelocity);
	}
	if (LagCompensation) {
		LagCompensation->Character = this;
		if (Controller) { // APawn::Controller
			LagCompensation->Controller = Cast<ABaseCharacterPlayerController>(Controller);
		}
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
	DOREPLIFETIME(ABaseCharacter, Shield);
	DOREPLIFETIME(ABaseCharacter, bDisableDealDamage);
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
	if (bEquipButtonPressed) {
		ServerEquip(); // 只有 Server 拥有Equip权限, 请求 Server 执行 ServerEquip_Implementation()函数
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

void ABaseCharacter::ReloadButtonPressed()
{
	if (Combat) {
		Combat->Reloading();
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
	if (bDisableDealDamage || bElimmed) return;

	/* DamageToHealth: 实际需减去的Health值 */
	float DamageToHealth = Damage;
	if (Shield > 0.f) {
		if (Shield >= Damage) {
			Shield -= Damage;
			DamageToHealth = 0;
		}
		else {
			DamageToHealth -= Shield;
			Shield = 0.f;
		}
	}

	/* Server调用 */
	Health = FMath::Clamp(Health - DamageToHealth, 0.f, MaxHealth);
	UpdateHUDHealth();
	UpdateHUDShield();
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

void ABaseCharacter::PollInit()
{
	if (BaseCharacterPlayerState == nullptr) { // 执行成功后不再进入该函数
		BaseCharacterPlayerState = GetPlayerState<ABaseCharacterPlayerState>();
		if (BaseCharacterPlayerState) {
			/* 刷新HUD, 避免其显示为默认值 */
			BaseCharacterPlayerState->AddToScore(0.f);
			BaseCharacterPlayerState->AddToDeaths(0);
		}
	}
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

void ABaseCharacter::OnRep_Health(float LashHealth)
{
	/* 所有Clients调用 */
	UpdateHUDHealth();
	if (Health < LashHealth) {
		PlayeHitReactMontage(); // 优化: 取代原有的MulticastHit()函数逻辑(多播RPC), 因为 属性复制/通知 比 RPC 要节约带宽 
	}
}

void ABaseCharacter::OnRep_Shield(float LashShield)
{
	/* 所有Clients调用 */
	UpdateHUDShield();
	if (Shield < LashShield) {
		PlayeHitReactMontage(); // 优化: 取代原有的MulticastHit()函数逻辑(多播RPC), 因为 属性复制/通知 比 RPC 要节约带宽 
	}
}

void ABaseCharacter::ServerEquip_Implementation()
{
	/** ServerEquip() 是被UE网络系统包装过的RPC调用入口. 此处定义的 ServerEquip_Implementation() 是在 Server端 的执行逻辑 */
	if (Combat) {
		if (OverlappingWeapon) {
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else if (Combat->ShouldSwapWeapons()) {
			Combat->SwapWeapons();
		}
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

void ABaseCharacter::PlayReloadMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage && Combat && Combat->EquippedWeapon) {
		AnimInstance->Montage_Play(ReloadMontage);
		FName SectionName;
		switch (Combat->EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_AssultRifle:
			SectionName = FName("Rifle");
			break;
		}
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABaseCharacter::Elim()
{
	/** 仅 Server 调用, 因为 GameMode 只在Server中存在 */
	if (Combat) {
		if (Combat->EquippedWeapon) {
			DropOrDestroyWeapon(Combat->EquippedWeapon);
		}
		if (Combat->SecondaryWeapon) {
			DropOrDestroyWeapon(Combat->SecondaryWeapon);
		}
	}
	MulticastElim();
	// 设置ElimTimer: 计时器结束后, 触发角色重生
	GetWorldTimerManager().SetTimer(ElimTimer, this, &ABaseCharacter::ElimTimerFinished, ElimDelay);
}

void ABaseCharacter::DropOrDestroyWeapon(AWeapon* Weapon)
{
	if (Weapon == nullptr) return;
	if (Weapon) {
		if (Weapon->bDestroyWeapon) {
			Weapon->Destroy();
		}
		else {
			Weapon->Dropped();
		}
	}
}

void ABaseCharacter::MulticastElim_Implementation()
{
	if (BaseCharacterPlayerController) {
		BaseCharacterPlayerController->SetHUDWeaponAmmo(0); // 角色被淘汰时, 对应玩家HUD中的弹药数清零
	}

	bElimmed = true;
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

ECombatState ABaseCharacter::GetCombatState() const
{
	return Combat->CombatState;
}

AWeapon* ABaseCharacter::GetEquippedWeapon() const
{
	if (GetCombat() == nullptr) return nullptr;
	return GetCombat()->EquippedWeapon;
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
