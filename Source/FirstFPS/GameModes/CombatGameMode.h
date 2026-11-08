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

namespace MatchState
{
	/** 进入Cooldown阶段: 比赛正式结束, 在Announcement组件中显示 比赛结果(获胜方) 和 倒计时信息 */
	extern FIRSTFPS_API const FName Cooldown;
	// extern：表示此处只是声明. 用于防止 多个.cpp文件 包含 该.h文件 时, 产生重复定义
	// FIRSTFPS_API：表示 Cooldown 可以从 FirstFPS模块 导出, 供 其他UE模块 使用
}

UCLASS()
class FIRSTFPS_API ACombatGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	ACombatGameMode();
	virtual void Tick(float DeltaTime) override;

	// 项目自定义的"玩家淘汰处理函数"
	virtual void PlayerEliminated(ABaseCharacter* ElimmedCharacter, ABaseCharacterPlayerController* VictimController, ABaseCharacterPlayerController* AttackController);
	// 项目自定义的"玩家重生处理函数"
	virtual void RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController);

	UPROPERTY(EditAnywhere)
	float WarmupTime = 10.f; // 停留在 WaitingToStart阶段 的时间
	UPROPERTY(EditAnywhere)
	float MatchTime = 120.f; // 停留在 Inprogress阶段 (正式比赛) 的时间
	UPROPERTY(EditAnywhere)
	float CooldownTime = 10.f; // 停留在 Cooldown阶段 (自定义MatchState) 的时间

	float LevelStartingTime = 0.f;

protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;

private:
	float CountdownTime = 0.f;
};
