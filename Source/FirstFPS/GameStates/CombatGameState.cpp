// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatGameState.h"
#include "Net/UnrealNetwork.h"
#include "FirstFPS/PlayerState/BaseCharacterPlayerState.h"

void ACombatGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACombatGameState, TopScoringPlayers);
}

void ACombatGameState::UpdateTopScore(ABaseCharacterPlayerState* ScoringPlayer)
{
	if (TopScoringPlayers.Num() == 0) {
		TopScoringPlayers.Add(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
	else if (ScoringPlayer->GetScore() == TopScore) {
		TopScoringPlayers.AddUnique(ScoringPlayer);
	}
	else if (ScoringPlayer->GetScore() > TopScore) {
		TopScoringPlayers.Empty();
		TopScoringPlayers.Add(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
}
