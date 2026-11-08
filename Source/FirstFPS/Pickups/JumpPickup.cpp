// Fill out your copyright notice in the Description page of Project Settings.


#include "JumpPickup.h"
#include "FirstFPS/Character/BaseCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

void AJumpPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(OtherActor);
	if (BaseCharacter) {
		UBuffComponent* Buff = BaseCharacter->GetBuff();
		if (Buff) {
			
		}
	}
	Destroy();
}