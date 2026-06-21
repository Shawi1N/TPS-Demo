// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget); // 播放枪械开火动画(生成枪口火焰、声音...)

	/** 仅在 Server端 处理投射物(子弹)运动逻辑 */
	if (!HasAuthority()) return;
												   // 枪口Socket: MuzzleFlash(Rifle资源中已建立, 用于生成枪口火焰)
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (MuzzleFlashSocket) {
		/** 设置 Projectile 的生成位置和旋转(朝向) */
		// GetSocketTransform: 根据 Mesh信息(Socket挂载在Mesh的相对位置), 计算出 Socket 的 FTransform(当前游戏世界中的位置、旋转、缩放)
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		// HitTarget表示 准星Trace得到的目标点(并非最终的击中点!), 用于确定投射物的起始飞行方向
		FVector ToTarget = HitTarget - SocketTransform.GetLocation();
		FRotator TargetRotation = ToTarget.Rotation();
		if (ProjectileClass) {
			/** 设置 Spawn 参数(Owner和Instigator) */
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = GetOwner();			 // Owner: 拥有 Spawned Actor 的AActor, 负责建立归属关系(网络复制、忽略碰撞)
			APawn* InstigatorPawn = Cast<APawn>(GetOwner()); // GetOwner(): 返回 该Actor(ProjectileWeapon) 的 Owner(Character)
			SpawnParams.Instigator = InstigatorPawn; // Instigator: 负责 Spawned Actor 所造成伤害 APawn, 结合UE伤害系统使用(AI仇恨、友伤判断...)
			/** 生成投射物: Projectile是在Server生成的. 因为它开启了Replication, 所有Clients也会收到复制出来的 Projectile副本 */
			UWorld* World = GetWorld();
			if (World) {
				World->SpawnActor<AProjectile>(
					ProjectileClass,
					SocketTransform.GetLocation(),
					TargetRotation,
					SpawnParams
				);
			}
		}
	}
}
