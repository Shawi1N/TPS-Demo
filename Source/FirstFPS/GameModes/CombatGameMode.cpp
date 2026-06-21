// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatGameMode.h"
#include "FirstFPS/Character/BaseCharacter.h"
#include "FirstFPS/Character/BaseCharacterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerState.h"

void ACombatGameMode::PlayerEliminated(ABaseCharacter* ElimmedCharacter, ABaseCharacterPlayerController* VictimController, ABaseCharacterPlayerController* AttackController)
{
	// 比赛已经结束后，不再处理新的淘汰和计分
	if (HasMatchEnded())
	{
		return;
	}

	/*
	 * 只有以下情况才给击杀者加分：
	 * 1. AttackerController 有效；
	 * 2. 不是自杀。
	 */
	if (AttackController &&
		AttackController != VictimController)
	{
		APlayerState* AttackerPlayerState = AttackController->GetPlayerState<APlayerState>();

		if (AttackerPlayerState)
		{
			const float NewScore =
				AttackerPlayerState->GetScore() + 1.f;

			AttackerPlayerState->SetScore(NewScore);

			if (NewScore >= ScoreToWin)
			{
				FinishMatch(AttackerPlayerState);
			}
		}
	}

	if (ElimmedCharacter)
	{
		// 最后一击即使结束了比赛，也应该播放淘汰表现
		ElimmedCharacter->Elim();
	}
}

void ACombatGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	/*
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
	*/
	// 比赛结束后不再生成新角色
	if (HasMatchEnded())
	{
		return;
	}

	if (ElimmedCharacter)
	{
		ElimmedCharacter->Reset();
	}

	if (!ElimmedController)
	{
		return;
	}

	TArray<AActor*> PlayerStarts;

	UGameplayStatics::GetAllActorsOfClass(
		this,
		APlayerStart::StaticClass(),
		PlayerStarts
	);

	if (PlayerStarts.Num() == 0)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("RequestRespawn: 当前关卡中没有 PlayerStart")
		);

		return;
	}

	const int32 Selection =
		FMath::RandRange(0, PlayerStarts.Num() - 1);

	RestartPlayerAtPlayerStart(
		ElimmedController,
		PlayerStarts[Selection]
	);
}

void ACombatGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It) { // 类型别名, 用于遍历游戏中所有的PlayerController 
		ABaseCharacterPlayerController* BaseCharacterController = Cast<ABaseCharacterPlayerController>(*It);
		if (BaseCharacterController) {
			BaseCharacterController->OnMatchStateSet(MatchState); // 更新各PlayerController中的MatchState
		}
	}
}
void ACombatGameMode::FinishMatch(APlayerState* Winner)
{
	if (!Winner || HasMatchEnded())
	{
		return;
	}

	WinnerPlayerState = Winner;

	/*
	 * EndMatch 会将比赛从 InProgress 转换到赛后状态，
	 * 并触发 ACombatGameMode::OnMatchStateSet()。
	 */
	EndMatch();

	const FString WinnerName = Winner->GetPlayerName();

	for (FConstPlayerControllerIterator It =
		GetWorld()->GetPlayerControllerIterator();
		It;
		++It)
	{
		ABaseCharacterPlayerController* PlayerController =
			Cast<ABaseCharacterPlayerController>(It->Get());

		if (!PlayerController)
		{
			continue;
		}

		APlayerState* CurrentPlayerState =
			PlayerController->GetPlayerState<APlayerState>();

		const bool bIsWinner =
			CurrentPlayerState == Winner;

		/*
		 * 每一个服务器端 PlayerController 都对应自己的 owning client。
		 * 调用 Client RPC 后，结果会发送给对应客户端。
		 */
		PlayerController->ClientHandleMatchEnded(
			bIsWinner,
			WinnerName
		);
	}
}