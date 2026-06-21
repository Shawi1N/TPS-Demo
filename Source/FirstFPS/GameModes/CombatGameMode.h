// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "CombatGameMode.generated.h"

class ABaseCharacter;
class ABaseCharacterPlayerController;
class APlayerState;
class ACharacter;
class AController;

UCLASS()
class FIRSTFPS_API ACombatGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	// 项目自定义的"玩家淘汰处理函数"
	virtual void PlayerEliminated(ABaseCharacter* ElimmedCharacter, ABaseCharacterPlayerController* VictimController, ABaseCharacterPlayerController* AttackController);
	// 项目自定义的"玩家重生处理函数"
	virtual void RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController);
protected:
	virtual void OnMatchStateSet() override; // 重写MatchState
protected:
	/** 玩家达到多少分时获胜 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match")
	float ScoreToWin = 5.f;

	/** 只保存在服务器的获胜者 */
	UPROPERTY()
	APlayerState* WinnerPlayerState = nullptr;

	void FinishMatch(APlayerState* Winner);
};
