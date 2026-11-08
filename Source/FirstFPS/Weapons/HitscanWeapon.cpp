// Fill out your copyright notice in the Description page of Project Settings.

#include "HitScanWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "FirstFPS/Character/BaseCharacter.h"
#include "FirstFPS/Character/BaseCharacterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "WeaponTypes.h"
#include "FirstFPS/CharacterComponents/LagCompensationComponent.h"

void AHitscanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget); // 播放枪械开火动画, 处理弹药消耗逻辑

	/* ApplyDamage() 需要参数 Owner 和 Instigator */
	APawn* OwnerPawn = Cast<APawn>(GetOwner());						// 该Weapon的拥有者(Owner)
	if (OwnerPawn == nullptr) return;
	AController* InstigatorController = OwnerPawn->GetController(); // 该Weapon造成的某个行为的发起者(Instigator)

	// 枪口Socket: MuzzleFlash(Rifle资源中已建立, 用于生成枪口火焰), 其位置作为"射线检测起始点"
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket) {
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();

		FHitResult FireHit; // 用于存储 射线检测函数(LineTraceSingleByChannel()) 的碰撞结果
		WeaponTraceHit(Start, HitTarget, FireHit); // 本地机器的射线检测

		ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(FireHit.GetActor());
		if (BaseCharacter && InstigatorController) { // 如果击中的是 BaseCharacter(玩家角色类)

			/* 当前机器所执行的普通Trace是否可以直接作为权威伤害依据: 未开启"服务器回溯" or 开枪玩家就在本地(ListenServer控制的角色) */
			bool bCauseAuthDamage = !bUseServerSideRewind || OwnerPawn->IsLocallyControlled();

			// 1. 若本身已处于Server 且 先前的LineTrace可作为权威伤害依据, 直接ApplyDamage()
			if (HasAuthority() && bCauseAuthDamage) {
				const float DamageToCause = FireHit.BoneName.ToString() == FString("head") ? HeadShotDamage : Damage; // 根据命中不同的HitBox(命中部位), 造成不同的伤害
				UGameplayStatics::ApplyDamage(BaseCharacter, DamageToCause, InstigatorController, this, UDamageType::StaticClass());
			}
			// 2. 开启了"服务器回溯", Server不使用 Fire() 在当前世界状态下的 LineTrace 直接判定伤害(不进入此处的两个if判断), 而是由 真正开枪的Owning Client 发起"服务器回溯"
			if (!HasAuthority() && bUseServerSideRewind) {
				WeaponOnwerCharcter = WeaponOnwerCharcter == nullptr ? Cast<ABaseCharacter>(OwnerPawn) : WeaponOnwerCharcter;
				WeaponOnwerController = WeaponOnwerController == nullptr ? Cast<ABaseCharacterPlayerController>(InstigatorController) : WeaponOnwerController;
				if (WeaponOnwerCharcter && WeaponOnwerController && WeaponOnwerCharcter->GetLagCompensation() && WeaponOnwerCharcter->IsLocallyControlled()) {
					/* WeaponOnwerCharcter->IsLocallyControlled(): 真正开枪的Owning Client */
					WeaponOnwerCharcter->GetLagCompensation()->ServerScoreRequest(
						BaseCharacter,
						Start,
						HitTarget,
						WeaponOnwerController->GetServerTime() - WeaponOnwerController->SingleTripTime
						// 回溯到"客户端开枪那一刻, 服务器的时间" (减去 从 Server 到 Client 的传输时间)
					);
				}
			}
		}
		// 播放击中特效 (粒子效果+声音) 
		if (ImpactParticles) { 
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, FireHit.ImpactPoint, FireHit.ImpactNormal.Rotation());
		}
		if (HitSound) {
			UGameplayStatics::PlaySoundAtLocation(this, HitSound, FireHit.ImpactPoint);
		}
		// 播放枪口火焰特效和枪声
		if (MuzzleFlash) {
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlash, SocketTransform);
		}
		if (FireSound) {
			UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
		}
	}
}

void AHitscanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit)
{
	UWorld* World = GetWorld();
	if (World) {
		FVector End = TraceStart + (HitTarget - TraceStart) * 1.25f; // 射线终点略微前移, 避免没有碰到模型表面

		/* LineTraceSingleByChannel(): 通过 指定Channel 对世界进行射线检测, 并返回第一个阻挡命中, 碰撞结果保存在 参数OnHit 中 */
		World->LineTraceSingleByChannel(OutHit, TraceStart, End, ECollisionChannel::ECC_Visibility);
		FVector BeamEnd = End;
		if (OutHit.bBlockingHit) { // bool bBlockingHit: 本次射线检测是否发生碰撞 (是否击中目标)
			BeamEnd = OutHit.ImpactPoint;
		}
		else {
			OutHit.ImpactPoint = End;
		}
		// 光束粒子(子弹轨迹)
		if (BeamParticles) {
			UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
				World,
				BeamParticles,
				TraceStart,
				FRotator::ZeroRotator,
				true
			);
			if (Beam) {
				Beam->SetVectorParameter(FName("Target"), BeamEnd);
			}
		}
	}
}