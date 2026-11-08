// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileRocket.h"
#include "Kismet/GameplayStatics.h"

AProjectileRocket::AProjectileRocket()
{
	RocketMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rocket Mesh"));
	RocketMesh->SetupAttachment(RootComponent);
	RocketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // RocketMesh 不参与碰撞检测
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	APawn* FiringPawn = GetInstigator(); // 返回 ProjectileRocket 的 Instigator(促使 该Actor 产生某个行为的 Pawn)
	if (FiringPawn) {
		AController* FiringController = FiringPawn->GetController();
		if (FiringController) {
			/** ApplyRadialDamageWithFalloff(): 类似ApplyDamage(), 该函数用于施加"带距离衰减的范围伤害" */
			UGameplayStatics::ApplyRadialDamageWithFalloff(
				this,						// 当前世界环境
				Damage,						// 中心区域的基础伤害 (内半径中的完整伤害)
				10.f,						// 外半径边缘会施加的最低伤害
				GetActorLocation(),			// 范围伤害的中心位置
				200.f,						// 内半径 (完整伤害区域的半径)
				500.f,						// 外半径 (范围伤害的最大半径)
				1.f,						// 控制伤害从内半径到外半径的伤害衰减系数 (1.0：接近线性衰减)
				UDamageType::StaticClass(),	// 伤害类型
				TArray<AActor*>(),			// 不会受到这次伤害的 Actor 列表
				this,						// 实际造成伤害的 Actor (DamageCauser)
				FiringController			// 本次伤害应归属的 Controller (InstigatedByController)
			);
		}
	}

	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit); // Destroy(); 销毁火箭弹(ProjectileRocket)
}