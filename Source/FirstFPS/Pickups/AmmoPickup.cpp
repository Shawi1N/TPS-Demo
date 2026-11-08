// Fill out your copyright notice in the Description page of Project Settings.


#include "AmmoPickup.h"
#include "FirstFPS/Character/BaseCharacter.h"
#include "FirstFPS/CharacterComponents/CombatComponent.h"

void AAmmoPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(OtherActor);
	if (BaseCharacter) {
		UCombatComponent* Combat = BaseCharacter->GetCombat();
		if (Combat) {
			Combat->PickupAmmo(WeaponType, AmmoAmount);
		}
	}
	Destroy(); // 拾取后, 弹药被销毁(播放拾取音效)
}
