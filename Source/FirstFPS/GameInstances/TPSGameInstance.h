// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "TPSGameInstance.generated.h"

/**
 *  管理房间操作
 */
UCLASS()
class FIRSTFPS_API UTPSGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    // 创建监听服务器并进入 Lobby。
    void HostLANGame();

    // 返回 true 只表示已发起连接，不代表连接成功。
    bool JoinLANGame(const FString& Address, FString& OutError);

    // 离开当前世界并返回菜单。
    void LeaveGame();

private:
    UPROPERTY(EditDefaultsOnly, Category = "Maps")
    FName LobbyMapName = TEXT("/Game/Maps/LobbyMap");

    UPROPERTY(EditDefaultsOnly, Category = "Maps")
    FName MenuMapName = TEXT("/Game/Maps/MenuMap");
};
