// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseCharacterPlayerState.h"
#include "FirstFPS/Character/BaseCharacterPlayerController.h"
#include "FirstFPS/Character/BaseCharacter.h"
#include "Net/UnrealNetwork.h"

void ABaseCharacterPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaseCharacterPlayerState, Deaths);
}

void ABaseCharacterPlayerState::AddToScore(float ScoreAmount)
{
	SetScore(GetScore() + ScoreAmount); // Score 只能由 Server 进行修改!
	/** 更新HUD击杀数 */
	/*
	Controller = (Controller == nullptr ? Cast<ABaseCharacterPlayerController>(GetPlayerController()) : Controller);
	if (Controller) {
		Controller->SetHUDScore(GetScore());
	}
	*/
	Character = Character == nullptr ? Cast<ABaseCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<ABaseCharacterPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDScore(GetScore());
		}
	}
}

void ABaseCharacterPlayerState::AddToDeaths(int32 DeathsAmount)
{
	Deaths += DeathsAmount;
	/** 更新HUD死亡数 */
	Controller = (Controller == nullptr ? Cast<ABaseCharacterPlayerController>(GetPlayerController()) : Controller);
	if (Controller) {
		Controller->SetHUDDeaths(Deaths);
	}
}

void ABaseCharacterPlayerState::OnRep_Score()
{
	Super::OnRep_Score();
	/*
	Controller = (Controller == nullptr ? Cast<ABaseCharacterPlayerController>(GetPlayerController()) : Controller);
	if (Controller) {
		Controller->SetHUDScore(GetScore());
	}
	*/
	Character = Character == nullptr ? Cast<ABaseCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<ABaseCharacterPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDScore(GetScore());
		}
	}
}

void ABaseCharacterPlayerState::OnRep_Deaths()
{
	/** 更新HUD死亡数 */
	Controller = (Controller == nullptr ? Cast<ABaseCharacterPlayerController>(GetPlayerController()) : Controller);
	if (Controller) {
		Controller->SetHUDDeaths(Deaths);
	}
}
