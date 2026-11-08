// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatGameMode.h"
#include "FirstFPS/Character/BaseCharacter.h"
#include "FirstFPS/Character/BaseCharacterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "FirstFPS/PlayerState/BaseCharacterPlayerState.h"
#include "FirstFPS/GameStates/CombatGameState.h"

namespace MatchState
{
	const FName Cooldown = FName(TEXT("Cooldown"));
}

ACombatGameMode::ACombatGameMode()
{
	/* 控制 Match 是否自动开始. 若为true, 玩家进入后 MatchState 不会从 WaitingToStart 过渡到 InProgress. 需要调用 StartMatch() 才能过渡到下一阶段 */
	/* 勾选 DelayedStart 后: MatchState 处于 WaitingToStart 时, 玩家使用 SpectatorClass; 处于 InProgress 时, 玩家使用 DefaultPawnClass */
	bDelayedStart = true;
}

void ACombatGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MatchState == MatchState::WaitingToStart) {
		CountdownTime = WarmupTime - (GetWorld()->GetTimeSeconds() - LevelStartingTime);
		if (CountdownTime <= 0.f) {
			StartMatch(); // AGameMode自带函数, WaitingToStart → Inprogress
		}
	}
	else if (MatchState == MatchState::InProgress) {
		CountdownTime = WarmupTime + MatchTime - (GetWorld()->GetTimeSeconds() - LevelStartingTime);
		if (CountdownTime <= 0.f) {
			SetMatchState(MatchState::Cooldown); // Inprogress → Cooldown
		}
	}
	else if (MatchState == MatchState::Cooldown) {
		CountdownTime = WarmupTime + MatchTime + CooldownTime - (GetWorld()->GetTimeSeconds() - LevelStartingTime);
		if (CountdownTime <= 0.f) {
			RestartGame(); // AGameMode自带函数: 由Server重新进入当前地图, 并重新初始化各类
			// P.S. 因为并没有调用 EndMatch(), 因此不会进入 WaitingPostMatch及其后续阶段
		}
	}
}

void ACombatGameMode::PlayerEliminated(ABaseCharacter* ElimmedCharacter, ABaseCharacterPlayerController* VictimController, ABaseCharacterPlayerController* AttackController)
{
	ABaseCharacterPlayerState* AttackerPlayerState = (AttackController ? Cast<ABaseCharacterPlayerState>(AttackController->PlayerState) : nullptr);
	ABaseCharacterPlayerState* VictimPlayerState = (VictimController ? Cast<ABaseCharacterPlayerState>(VictimController->PlayerState) : nullptr);
	ACombatGameState* CombatGameState = GetGameState<ACombatGameState>();
	
	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState && CombatGameState) { // 自杀不算作击杀
		AttackerPlayerState->AddToScore(1.f);
		CombatGameState->UpdateTopScore(AttackerPlayerState);
	}
	if (VictimPlayerState) {
		VictimPlayerState->AddToDeaths(1);
	}
	if (ElimmedCharacter) {
		ElimmedCharacter->Elim(); // 处理角色淘汰相关逻辑
	}
}

void ACombatGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (ElimmedCharacter) {
		ElimmedCharacter->Reset();  // APawn::Reset(): 解除 Pawn 与 对应Controller 的控制关系, 并请求销毁 Pawn 
		//ElimmedCharacter->Destroy();
	}
	if (ElimmedController) {
		TArray<AActor*> PlayerStarts;
		// 查找 当前World 中 指定Actor类及其派生类的所有实例, 并把结果写入数组.		APlayerStart: 本质上就是AActor, 用于标记玩家出生位置和朝向
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
		// 让 指定Controller 在 指定出生点位置 生成 新Pawn(GameMode设置的默认Pawn类型), 并控制该Pawn
		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]);
	}
}

void ACombatGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 当 ACombatGameMode 开始运行时, 记录当前世界时间, 作为本局比赛开局倒计时的起始基准
	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void ACombatGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();
	/** FConstPlayerControllerIterator: 类型别名, 用于遍历游戏中所有的PlayerController */
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It) { 
		ABaseCharacterPlayerController* BaseCharacterController = Cast<ABaseCharacterPlayerController>(*It);
		if (BaseCharacterController) {
			BaseCharacterController->OnMatchStateSet(MatchState); // 更新 Server 中 各PlayerController 的 MatchState变量
		}
	}
}