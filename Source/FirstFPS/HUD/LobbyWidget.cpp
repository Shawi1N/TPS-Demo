// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyWidget.h"

#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "TimerManager.h"

#include "FirstFPS/GameStates/LobbyGameState.h"

void ULobbyWidget::NativeConstruct()
{
    Super::NativeConstruct();

    RefreshLobbyStatus();

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            RefreshTimer,
            this,
            &ULobbyWidget::RefreshLobbyStatus,
            0.2f,
            true
        );
    }
}

void ULobbyWidget::NativeDestruct()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(RefreshTimer);
    }

    Super::NativeDestruct();
}

void ULobbyWidget::RefreshLobbyStatus()
{
    UWorld* World = GetWorld();

    ALobbyGameState* LobbyState =
        World ? World->GetGameState<ALobbyGameState>() : nullptr;

    if (!LobbyState)
    {
        if (PlayerCountText)
        {
            PlayerCountText->SetText(
                FText::FromString(TEXT("-- / --"))
            );
        }

        if (StatusText)
        {
            StatusText->SetText(
                FText::FromString(TEXT("Connecting to lobby..."))
            );
        }

        return;
    }

    if (PlayerCountText)
    {
        PlayerCountText->SetText(
            FText::FromString(
                FString::Printf(
                    TEXT("当前人数: %d / %d"),
                    LobbyState->GetPlayerCount(),
                    LobbyState->GetRequiredPlayers()
                )
            )
        );
    }

    if (StatusText)
    {
        StatusText->SetText(
            FText::FromString(
                LobbyState->IsStarting()
                ? TEXT("Players ready. Starting game...")
                : TEXT("ABC123 | 中文测试 | \u4E2D\u6587\u6D4B\u8BD5")
            )
        );
    }
}