// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthPickup.h"
#include "FirstFPS/Character/BaseCharacter.h"
#include "FirstFPS/CharacterComponents/BuffComponent.h"

AHealthPickup::AHealthPickup()
{
	bReplicates = true;
}

void AHealthPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(OtherActor);
	if (BaseCharacter) {
		UBuffComponent* Buff = BaseCharacter->GetBuff();
		if (Buff) {
			Buff->Heal(HealAmount, HealTime); // 交由 BuffComponent 处理治疗逻辑
		}
	}

	Destroy();
}