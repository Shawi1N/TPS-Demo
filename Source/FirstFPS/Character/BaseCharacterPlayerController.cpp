// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseCharacterPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "FirstFPS/HUD/BaseCharacterHUD.h"
#include "FirstFPS/HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "FirstFPS/Character/BaseCharacter.h"
#include "Net/UnrealNetwork.h"
#include "FirstFPS/GameModes/CombatGameMode.h"
#include "GameFramework/PlayerController.h" /////////////

void ABaseCharacterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	BaseCharacterHUD = Cast<ABaseCharacterHUD>(GetHUD());

	////////////////////
	if (IsLocalController())
	{
		bShowMouseCursor = false;

		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);

		SetIgnoreMoveInput(false);
		SetIgnoreLookInput(false);

		FlushPressedKeys();
	}
}

void ABaseCharacterPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHUDTime();

	TimeSyncRunningTime += DeltaTime;
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency) { // 重新同步
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
}

void ABaseCharacterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	BaseCharacterHUD = (BaseCharacterHUD == nullptr ? Cast<ABaseCharacterHUD>(GetHUD()) : BaseCharacterHUD);

	bool bHUDValid = BaseCharacterHUD && 
		BaseCharacterHUD->CharacterOverlay &&
		BaseCharacterHUD->CharacterOverlay->HealthBar && 
		BaseCharacterHUD->CharacterOverlay->HealthText;
	if (bHUDValid) {
		const float HealthPercent = Health / MaxHealth;
		BaseCharacterHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);

		FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth)); // FMath::CeilToInt: 向上取整
		BaseCharacterHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText)); // FText::FromString: 将 FString 转成 FText
	}
}

void ABaseCharacterPlayerController::SetHUDMatchCountdown(float Countdown)
{
	BaseCharacterHUD = (BaseCharacterHUD == nullptr ? Cast<ABaseCharacterHUD>(GetHUD()) : BaseCharacterHUD);
	bool bHUDValid = BaseCharacterHUD &&
		BaseCharacterHUD->CharacterOverlay &&
		BaseCharacterHUD->CharacterOverlay->MatchCountdownText;
	if (bHUDValid) {
		int32 Minutes = FMath::FloorToInt(Countdown / 60.f);
		int32 Seconds = Countdown - Minutes * 60;
		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		BaseCharacterHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));
	}
}

void ABaseCharacterPlayerController::SetHUDTime()
{
	/*
	// GetWorld()->GetTimeSeconds(): 当前 UWorld 从开始运行游戏以来经过的游戏时间(单位是秒)
	int32 SecondsLeft = FMath::CeilToInt(MatchTime - GetServerTime()); // 游戏剩余时间 <GetServerTime!!!>
	if (SecondsLeft != CountdownInt) { // 倒计时发生变化
		SetHUDMatchCountdown(MatchTime - GetServerTime());
	}
	CountdownInt = SecondsLeft;
	*/
	if (!GetWorld())
	{
		return;
	}

	if (MatchState == MatchState::WaitingPostMatch)
	{
		SetHUDMatchCountdown(0.f);
		return;
	}

	const float TimeLeft =
		FMath::Max(0.f, MatchTime - GetServerTime());

	const int32 SecondsLeft =
		FMath::CeilToInt(TimeLeft);

	if (SecondsLeft != CountdownInt)
	{
		SetHUDMatchCountdown(TimeLeft);
		CountdownInt = SecondsLeft;
	}
}

void ABaseCharacterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(InPawn);
	if (BaseCharacter) {
		SetHUDHealth(BaseCharacter->GetHealth(), BaseCharacter->GetMaxHealth());
	}
}

void ABaseCharacterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABaseCharacterPlayerController, MatchState);
}

void ABaseCharacterPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (IsLocalPlayerController()) {
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer())) {
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts) {
				Subsystem->AddMappingContext(CurrentContext, 0);
			}
		}
	}
}


void ABaseCharacterPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds(); // 在 Server 读取当前Server时间
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ABaseCharacterPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClient)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest; // 从 Client 发出 RPC 到现在的实际耗时
	float CurrentServerTime = TimeServerReceivedClient + (0.5f * RoundTripTime);
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}


float ABaseCharacterPlayerController::GetServerTime()
{
	if (HasAuthority())
		return GetWorld()->GetTimeSeconds();
	else
		return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void ABaseCharacterPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	if (IsLocalController()) { // 本地机器
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void ABaseCharacterPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;

	if (MatchState == MatchState::WaitingPostMatch)
	{
		SetHUDMatchCountdown(0.f);
	}
}

void ABaseCharacterPlayerController::OnRep_MatchState()
{
	OnMatchStateSet(MatchState);
}

void ABaseCharacterPlayerController::
ClientHandleMatchEnded_Implementation(
	bool bWon,
	const FString& WinnerName)
{
	// 只影响这个客户端的本地输入
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);

	bShowMouseCursor = true;

	FInputModeUIOnly InputMode;
	SetInputMode(InputMode);

	BP_ShowMatchResult(bWon, WinnerName);
}