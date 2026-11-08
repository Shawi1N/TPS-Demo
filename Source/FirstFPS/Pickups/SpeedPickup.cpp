// Fill out your copyright notice in the Description page of Project Settings.


#include "SpeedPickup.h"
#include "FirstFPS/Character/BaseCharacter.h"
#include "FirstFPS/CharacterComponents/BuffComponent.h"

ASpeedPickup::ASpeedPickup()
{
	bReplicates = true; // 让 ASpeedPickup 这个拾取物 Actor 本身参与网络复制
}

void ASpeedPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(OtherActor);
	if (BaseCharacter) {
		UBuffComponent* Buff = BaseCharacter->GetBuff();
		if (Buff) {
			Buff->BuffSpeed(BaseSpeedBuff, CrouchSpeedBuff, SpeedBuffTime);
		}
	}
	Destroy();
}