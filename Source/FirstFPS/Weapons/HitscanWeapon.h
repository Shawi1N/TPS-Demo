// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "HitscanWeapon.generated.h"

class UParticleSystem;
class ABaseCharacter;
class ABaseCharacterPlayerController;

UCLASS()
class FIRSTFPS_API AHitscanWeapon : public AWeapon
{
	GENERATED_BODY()
public:
	virtual void Fire(const FVector& HitTarget) override;
protected:
	void WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit);

	/** 
	ProjectileWeapon: Weapon 负责 Spawn Projectile;  Projectile 负责命中;  Projectile 负责 Damage
	HitscanWeapon:									 Weapon 负责命中;		  Weapon 负责 Damage
	*/

	/*
	UPROPERTY(EditAnywhere)
	float Damage = 20.f;
	UPROPERTY(EditAnywhere)
	float HeadShotDamage = 40.f;
	*/

	/* 击中特效 (粒子效果+声音) */
	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticles;
	UPROPERTY(EditAnywhere)
	USoundCue* HitSound;
private:
	UPROPERTY(EditAnywhere)
	UParticleSystem* BeamParticles;

	UPROPERTY(EditAnywhere)
	UParticleSystem* MuzzleFlash;

	UPROPERTY(EditAnywhere)
	USoundCue* FireSound;
};