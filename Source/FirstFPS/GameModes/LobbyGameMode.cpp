// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"
#include "GameFramework/GameState.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	/** 根据 GameMode 的配置, GameMode类 会自动生成指定的 GameState */
	/** GameState的类型为TObjectPtr<AGameStateBase> (TObjectPtr 是UE5对 UObject指针 的一层包装, 便于UE进行对象追踪、GC、编辑器检查) */
	/** GameState.Get()的作用是从包装器中取出真正的 AGameStateBase*指针 */
	int32 NumOfPlayers = GameState.Get()->PlayerArray.Num();
	if (NumOfPlayers >= 2) {
		UWorld* World = GetWorld();
		if (World) {
			bUseSeamlessTravel = true; // 开启无缝转移
			World->ServerTravel(FString("/Game/Maps/CombatLevel?Listen"));
		}
	}
}
