// Fill out your copyright notice in the Description page of Project Settings.



#include "LobbyGameMode.h"

#include "Engine/World.h"
#include "TimerManager.h"

#include "FirstFPS/PlayerControllers/LobbyPlayerController.h"
#include "FirstFPS/GameStates/LobbyGameState.h"

/*
void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// 根据 GameMode 的配置, GameMode类 会自动生成指定的 GameState
	// GameState的类型为TObjectPtr<AGameStateBase> (TObjectPtr 是UE5对 UObject指针 的一层包装, 便于UE进行对象追踪、GC、编辑器检查)
	// GameState.Get()的作用是从包装器中取出真正的 AGameStateBase*指针
	int32 NumOfPlayers = GameState.Get()->PlayerArray.Num();
	if (NumOfPlayers >= 2) {
		UWorld* World = GetWorld();
		if (World) {
			bUseSeamlessTravel = true; // 开启无缝转移
			World->ServerTravel(FString("/Game/Maps/CombatLevel?Listen"));
		}
	}
}
*/

ALobbyGameMode::ALobbyGameMode()
{
    bUseSeamlessTravel = true;

    // 允许 AGameMode 正常开始, 并为玩家生成 Pawn。
    bDelayedStart = false;

    HUDClass = nullptr;
    PlayerControllerClass = ALobbyPlayerController::StaticClass();
    GameStateClass = ALobbyGameState::StaticClass();
}

void ALobbyGameMode::BeginPlay()
{
    Super::BeginPlay();
    UpdateLobbyState();
}

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    UE_LOG(
        LogTemp,
        Log,
        TEXT("Player joined lobby. Players: %d"),
        GetNumPlayers()
    );

    UpdateLobbyState();
}

void ALobbyGameMode::Logout(AController* Exiting)
{
    Super::Logout(Exiting);

    UpdateLobbyState();
}

void ALobbyGameMode::UpdateLobbyState()
{
    if (bTravelStarted) {
        return;
    }

    const int32 PlayerCount = GetNumPlayers();
    const bool bEnoughPlayers = PlayerCount >= RequiredPlayers;

    if (bEnoughPlayers) {
        // 只注册一次，避免重复刷新时不断重置计时器。
        if (!GetWorldTimerManager().IsTimerActive(StartGameTimer)) {
            GetWorldTimerManager().SetTimer(
                StartGameTimer,
                this,
                &ALobbyGameMode::TravelToCombat,
                StartDelay,
                false
            );
        }
    }
    else {
        // 等待期间有人离开，取消开始。
        GetWorldTimerManager().ClearTimer(StartGameTimer);
    }

    if (ALobbyGameState* LobbyState = GetGameState<ALobbyGameState>()) {
        LobbyState->SetLobbyStatus(
            PlayerCount,
            RequiredPlayers,
            bEnoughPlayers
        );
    }
}

void ALobbyGameMode::TravelToCombat()
{
    if (bTravelStarted || !GetWorld()) {
        return;
    }

    // 计时器结束时再次确认人数。
    if (GetNumPlayers() < RequiredPlayers) {
        UpdateLobbyState();
        return;
    }

    if (CombatMapPath.IsEmpty()){
        UE_LOG(
            LogTemp,
            Error,
            TEXT("CombatMapPath is empty.")
        );
        return;
    }

    bTravelStarted = true;

    UE_LOG(
        LogTemp,
        Log,
        TEXT("Travelling to %s"),
        *CombatMapPath
    );

    const bool bAccepted = GetWorld()->ServerTravel(CombatMapPath);

    if (!bAccepted) {
        bTravelStarted = false;

        if (ALobbyGameState* LobbyState = GetGameState<ALobbyGameState>()) {
            LobbyState->SetLobbyStatus(
                GetNumPlayers(),
                RequiredPlayers,
                false
            );
        }

        UE_LOG(
            LogTemp,
            Error,
            TEXT("ServerTravel request failed.")
        );
    }
}
