// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectile.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundCue.h"
#include "FirstFPS/Character/BaseCharacter.h"
#include "FirstFPS/FirstFPS.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true; // 开启网络复制

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	// 让 CollisionBox 可以被 Visibility类型的射线检测(UE常见的Trace Channel类型) 检测到
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh , ECollisionResponse::ECR_Block);

	// 子弹模型一般无需显示出来(非常小), 因此无需使用Mesh

	/* 根据 ProjectileMovementComponent 设置, 它将逐帧更新另一个组件UpdatedComponent(默认为根组件)的位置 */
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->InitialSpeed = 15000.f;
	ProjectileMovementComponent->MaxSpeed = 15000.f;
	ProjectileMovementComponent->bRotationFollowsVelocity = true; // 让 Projectile 的旋转方向(朝向)始终和飞行方向一致
} 

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	/** 每台机器上的 Projectile副本 各自执行 BeginPlay(), 各自生成本地的粒子特效 */
	if (Tracer) {
		// SpawnEmitterAttached(): 生成一个粒子特效组件, 并把其附加到某个 SceneComponent 上播放
		TracerComponent = UGameplayStatics::SpawnEmitterAttached(
			Tracer,			// 要播放的粒子特效资源
			CollisionBox,	// 特效要附加到的组件
			FName(),		// Socket名称, 若为空则直接附加到组件本身
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition // Attach时如何处理位置/旋转
		); 
	}

	if (HasAuthority()) { // 只在 Server端 绑定OnHit(处理碰撞, 对Projectile的碰撞位置拥有控制权)
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit); // 当 CollisionBox 撞到阻挡物时, 会调用OnHit()
	}
}

void AProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	/* 尽可能使用已有的网络复制和RPC, 尽最大可能降低网络带宽 */
	Destroy(); // 如果 Actor 是 replicated, 并且由 Server端 调用 Destroy() 销毁: 则销毁会通过网络复制同步到所有相关Clients
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AProjectile::Destroyed()
{
	Super::Destroyed();

	if (ImpactParticles) {
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());
	}
	if (ImpactSound) {
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
}

