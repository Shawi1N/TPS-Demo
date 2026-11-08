// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BaseCharacterPlayerState.generated.h"

class ABaseCharacter;
class ABaseCharacterPlayerController;

UCLASS()
class FIRSTFPS_API ABaseCharacterPlayerState : public APlayerState
{
	GENERATED_BODY()
public:
	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const override;
	void AddToScore(float ScoreAmount);  // 自定义函数, 用于添加Score
	void AddToDeaths(int32 DeathsAmount);
	virtual void OnRep_Score() override; // Clients 收到 Server中更新的Score后调用的通知函数(APlayerState中为空的虚函数), 此处重写用于更新对应UI
	UFUNCTION()
	virtual void OnRep_Deaths();
private:
	UPROPERTY()
	ABaseCharacter* Character;
	/** 通过Controller访问对应HUD */
	UPROPERTY()
	ABaseCharacterPlayerController* Controller;

	UPROPERTY(ReplicatedUsing = OnRep_Deaths)
	int32 Deaths;
};
