// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameState.h"

#include "Net/UnrealNetwork.h"

void ALobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ALobbyGameState, PlayerCount);
    DOREPLIFETIME(ALobbyGameState, RequiredPlayers);
    DOREPLIFETIME(ALobbyGameState, bStarting);
}

void ALobbyGameState::SetLobbyStatus(int32 InPlayerCount, int32 InRequiredPlayers, bool bInStarting)
{
    if (!HasAuthority()) {
        return;
    }

    PlayerCount = InPlayerCount;
    RequiredPlayers = InRequiredPlayers;
    bStarting = bInStarting;

    ForceNetUpdate();
}