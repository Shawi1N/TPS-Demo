// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterOverlay.generated.h"

class UProgressBar;
class UTextBlock;
class UImage;

UCLASS()
class FIRSTFPS_API UCharacterOverlay : public UUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget))
	UProgressBar* HealthBar;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* HealthText;
	UPROPERTY(meta = (BindWidget))
	UProgressBar* ShieldBar;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* ShieldText;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* MatchCountdownText;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* ScoreAmount;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* DeathAmount;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* WeaponAmmoAmount;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* CarriedAmmoAmount;
	UPROPERTY(meta = (BindWidget))
	UImage* HighPingImage;
	UPROPERTY(meta = (BindWidgetAnim), Transient) // Transient: 说明 UWidgetAnimation* 只是运行时自动绑定的临时引用, 不应该被保存
	UWidgetAnimation* HighPingAnimation;		  // UWidgetAnimation: UMG Widget 里的动画资源, 用来在 C++ 中播放、停止或控制 UI动画
};
