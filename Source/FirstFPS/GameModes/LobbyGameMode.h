// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "LobbyGameMode.generated.h"

UCLASS()
class FIRSTFPS_API ALobbyGameMode : public AGameMode
{
    GENERATED_BODY()

public:
    ALobbyGameMode();

    virtual void PostLogin(APlayerController* NewPlayer) override;

    virtual void Logout(AController* Exiting) override;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(EditDefaultsOnly, Category = "Lobby", meta = (ClampMin = "2"))
    int32 RequiredPlayers = 2;

    UPROPERTY(EditDefaultsOnly, Category = "Lobby", meta = (ClampMin = "0.1"))
    float StartDelay = 2.f;

    // 你的原始代码使用 CombatLevel。
    // 如果实际地图叫 CombatMap，请修改此配置。
    UPROPERTY(EditDefaultsOnly, Category = "Lobby")
    FString CombatMapPath = TEXT("/Game/Maps/CombatMap");

    FTimerHandle StartGameTimer;

    bool bTravelStarted = false;

    void UpdateLobbyState();
    void TravelToCombat();
};

