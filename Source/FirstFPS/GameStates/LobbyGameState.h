// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "LobbyGameState.generated.h"

UCLASS()
class FIRSTFPS_API ALobbyGameState : public AGameState
{
    GENERATED_BODY()

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // 只允许服务器修改大厅状态
    void SetLobbyStatus(int32 InPlayerCount, int32 InRequiredPlayers, bool bInStarting);

    int32 GetPlayerCount() const
    {
        return PlayerCount;
    }

    int32 GetRequiredPlayers() const
    {
        return RequiredPlayers;
    }

    bool IsStarting() const
    {
        return bStarting;
    }

private:
    UPROPERTY(Replicated)
    int32 PlayerCount = 0;

    UPROPERTY(Replicated)
    int32 RequiredPlayers = 2;

    UPROPERTY(Replicated)
    bool bStarting = false;
};