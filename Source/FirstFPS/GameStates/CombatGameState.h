// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "CombatGameState.generated.h"

class ABaseCharacterPlayerState;

UCLASS()
class FIRSTFPS_API ACombatGameState : public AGameState
{
	GENERATED_BODY()
public:
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void UpdateTopScore(ABaseCharacterPlayerState* ScoringPlayer); // 维护当前最高分玩家

	UPROPERTY(Replicated)
	TArray<ABaseCharacterPlayerState*> TopScoringPlayers; // 记录最高分玩家(可能有多个并列第一)
private:
	float TopScore = 0.f; // 最高得分(击杀数)
};
