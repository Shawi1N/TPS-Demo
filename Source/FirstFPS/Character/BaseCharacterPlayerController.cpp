// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseCharacterPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "FirstFPS/HUD/BaseCharacterHUD.h"
#include "FirstFPS/HUD/CharacterOverlay.h"
#include "FirstFPS/HUD/AnnouncementWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "FirstFPS/Character/BaseCharacter.h"
#include "Net/UnrealNetwork.h"
#include "FirstFPS/GameModes/CombatGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "FirstFPS/GameStates/CombatGameState.h"
#include "FirstFPS/PlayerState/BaseCharacterPlayerState.h"
#include "Components/Image.h"

void ABaseCharacterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	BaseCharacterHUD = Cast<ABaseCharacterHUD>(GetHUD());
	ServerCheckMatchState();
}

void ABaseCharacterPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHUDTime();

	/** 时间同步检查 */
	TimeSyncRunningTime += DeltaTime;
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency) { // 当累计时间超过时间同步频率(间隔), 重新向服务器请求一次时间同步
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}

	PollInit();

	/* 定期检查当前玩家Ping值: 若 Ping值 超过阈值(HighPingThreshold), 则播放高延迟警告动画; 当动画播放超过指定时间(HighPingDuration)后自动停止 */
	CheckPing(DeltaTime);
}

void ABaseCharacterPlayerController::CheckPing(float DeltaTime)
{
	HighPingRunningTime += DeltaTime;
	if (HighPingRunningTime > CheckPingFrequency) {
		if (PlayerState == nullptr) {
			PlayerState = GetPlayerState<APlayerState>();
		}
		if (PlayerState) {
			if (PlayerState->GetPingInMilliseconds() > HighPingThreshold) { // GetPingInMilliseconds(): 获取该玩家的大致Ping值, 单位是毫秒(ms)
				HighPingWarning();
				PingAnimationRunningTime = 0.f;
				ServerReportPingStatus(true);
			}
			else {
				ServerReportPingStatus(false);
			}
		}
		HighPingRunningTime = 0.f;
	}
	bool bHighAnimationPlaying = BaseCharacterHUD &&
		BaseCharacterHUD->CharacterOverlay &&
		BaseCharacterHUD->CharacterOverlay->HighPingAnimation &&
		BaseCharacterHUD->CharacterOverlay->IsAnimationPlaying(BaseCharacterHUD->CharacterOverlay->HighPingAnimation);
	if (bHighAnimationPlaying) { // 如果 HighPingAnimation 正在播放, 记录 PingAnimationRunningTime
		PingAnimationRunningTime += DeltaTime;
		if (PingAnimationRunningTime > HighPingDuration) {
			StopHighPingWarning();
		}
	}
}

void ABaseCharacterPlayerController::ServerReportPingStatus_Implementation(bool bHighPing)
{
	HighPingDelegate.Broadcast(bHighPing);
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
	else {
		bInitializeHealth = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}

void ABaseCharacterPlayerController::SetHUDShield(float Shield, float MaxShield)
{
	BaseCharacterHUD = (BaseCharacterHUD == nullptr ? Cast<ABaseCharacterHUD>(GetHUD()) : BaseCharacterHUD);

	bool bHUDValid = BaseCharacterHUD &&
		BaseCharacterHUD->CharacterOverlay &&
		BaseCharacterHUD->CharacterOverlay->ShieldBar &&
		BaseCharacterHUD->CharacterOverlay->ShieldText;
	if (bHUDValid) {
		const float ShieldPercent = Shield / MaxShield;
		BaseCharacterHUD->CharacterOverlay->ShieldBar->SetPercent(ShieldPercent);

		FString ShieldText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Shield), FMath::CeilToInt(MaxShield));
		BaseCharacterHUD->CharacterOverlay->ShieldText->SetText(FText::FromString(ShieldText));
	}
	else {
		bInitializeShield = true;
		HUDShield = Shield;
		HUDMaxShield = MaxShield;
	}
}

void ABaseCharacterPlayerController::SetHUDScore(float Score)
{
	BaseCharacterHUD = (BaseCharacterHUD == nullptr ? Cast<ABaseCharacterHUD>(GetHUD()) : BaseCharacterHUD);

	bool bHUDValid = BaseCharacterHUD &&
		BaseCharacterHUD->CharacterOverlay &&
		BaseCharacterHUD->CharacterOverlay->ScoreAmount;
	if (bHUDValid) {
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));				// 将 float 转成 FString
		BaseCharacterHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText)); // 将 FString 转成 FText
	}
	else {
		bInitializeScore = true;
		HUDScore = Score;
	}
}

void ABaseCharacterPlayerController::SetHUDDeaths(int32 Death)
{
	BaseCharacterHUD = (BaseCharacterHUD == nullptr ? Cast<ABaseCharacterHUD>(GetHUD()) : BaseCharacterHUD);

	bool bHUDValid = BaseCharacterHUD &&
		BaseCharacterHUD->CharacterOverlay &&
		BaseCharacterHUD->CharacterOverlay->DeathAmount;
	if (bHUDValid) {
		FString DeathsText = FString::Printf(TEXT("%d"), Death);								 // 将 int32 转成 FString
		BaseCharacterHUD->CharacterOverlay->DeathAmount->SetText(FText::FromString(DeathsText)); // 将 FString 转成 FText
	}
	else {
		bInitializeDeaths = true;
		HUDDeaths = Death;
	}
}

void ABaseCharacterPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	BaseCharacterHUD = (BaseCharacterHUD == nullptr ? Cast<ABaseCharacterHUD>(GetHUD()) : BaseCharacterHUD);

	bool bHUDValid = BaseCharacterHUD &&
		BaseCharacterHUD->CharacterOverlay &&
		BaseCharacterHUD->CharacterOverlay->WeaponAmmoAmount;
	if (bHUDValid) {
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		BaseCharacterHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
	}
	else {
		bIniitializeWeaponAmmo = true;
		HUDWeaponAmmo = Ammo;
	}
}

void ABaseCharacterPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
	BaseCharacterHUD = (BaseCharacterHUD == nullptr ? Cast<ABaseCharacterHUD>(GetHUD()) : BaseCharacterHUD);

	bool bHUDValid = BaseCharacterHUD &&
		BaseCharacterHUD->CharacterOverlay &&
		BaseCharacterHUD->CharacterOverlay->CarriedAmmoAmount;
	if (bHUDValid) {
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		BaseCharacterHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
	}
	else {
		bIniitializeCarriedAmmo = true;
		HUDCarriedAmmo = Ammo;
	}
}

void ABaseCharacterPlayerController::SetHUDAnnouncementCountdown(float Countdown)
{
	BaseCharacterHUD = (BaseCharacterHUD == nullptr ? Cast<ABaseCharacterHUD>(GetHUD()) : BaseCharacterHUD);
	bool bHUDValid = BaseCharacterHUD &&
		BaseCharacterHUD->Announcement &&
		BaseCharacterHUD->Announcement->WarmupTime;
	if (bHUDValid) {
		if (Countdown < 0.f) {
			BaseCharacterHUD->Announcement->WarmupTime->SetText(FText()); // 如果Countdown为负值, 显示空文本(相当于不显示)
			return;
		}

		int32 Minutes = FMath::FloorToInt(Countdown / 60.f);
		int32 Seconds = Countdown - Minutes * 60;
		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		BaseCharacterHUD->Announcement->WarmupTime->SetText(FText::FromString(CountdownText));
	}
}

void ABaseCharacterPlayerController::SetHUDMatchCountdown(float Countdown)
{
	BaseCharacterHUD = (BaseCharacterHUD == nullptr ? Cast<ABaseCharacterHUD>(GetHUD()) : BaseCharacterHUD);
	bool bHUDValid = BaseCharacterHUD &&
		BaseCharacterHUD->CharacterOverlay &&
		BaseCharacterHUD->CharacterOverlay->MatchCountdownText;
	if (bHUDValid) {
		if (Countdown < 0.f) {
			BaseCharacterHUD->CharacterOverlay->MatchCountdownText->SetText(FText()); // 如果Countdown为负值, 显示空文本(相当于不显示)
			return;
		}

		int32 Minutes = FMath::FloorToInt(Countdown / 60.f);
		int32 Seconds = Countdown - Minutes * 60;
		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds); // %02d: 0:空位使用0补齐;	2: 最小显示宽度为2
		BaseCharacterHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));
	}
}

void ABaseCharacterPlayerController::SetHUDTime()
{
	/** GetServerTime(): 从Server启动以来经过的时间(单位: 秒) */
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart) TimeLeft = WarmupTime - (GetServerTime() - LevelStartTime);
	else if (MatchState == MatchState::InProgress) TimeLeft = WarmupTime + MatchTime - (GetServerTime() - LevelStartTime);
	else if (MatchState == MatchState::Cooldown) TimeLeft = WarmupTime + MatchTime + CooldownTime - (GetServerTime() - LevelStartTime);
	int32 SecondsLeft = FMath::CeilToInt(TimeLeft); // 剩余时间

	if (SecondsLeft != CountdownInt) { // 当 剩余时间(秒数) 发生变化时, 才更新倒计时HUD <每秒调用SetHUDxxxCountdown(), 无需每帧更新>
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown) {
			SetHUDAnnouncementCountdown(TimeLeft);
		}
		if (MatchState == MatchState::InProgress) {
			SetHUDMatchCountdown(TimeLeft);
		}
	}
	CountdownInt = SecondsLeft;
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
	/* TimeOfClientRequest: 客户端发出请求那一刻的本地时间;	ServerTimeOfReceipt: 服务器记录收到请求时的时间	*/
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds(); // 在 Server 读取当前Server时间
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ABaseCharacterPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest; // 从 Client 发出 RPC 到现在的实际耗时
	SingleTripTime = 0.5f * RoundTripTime;
	float CurrentServerTime = TimeServerReceivedClientRequest + SingleTripTime;
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

void ABaseCharacterPlayerController::PollInit()
{
	if (CharacterOverlay == nullptr) {
		if (BaseCharacterHUD && BaseCharacterHUD->CharacterOverlay) {
			CharacterOverlay = BaseCharacterHUD->CharacterOverlay;
			if (CharacterOverlay) {
				// SetHUDxxx均需要保证 BaseCharacterHUD->CharacterOverlay 有效 (在AddCharacterOverlay()创建), 才能正常更新HUD参数显示
				if (bInitializeHealth) SetHUDHealth(HUDHealth, HUDMaxHealth);
				if (bInitializeShield) SetHUDShield(HUDShield, HUDMaxShield);
				if (bInitializeScore) SetHUDScore(HUDScore);
				if (bInitializeDeaths) SetHUDDeaths(HUDDeaths);
				if (bIniitializeWeaponAmmo) SetHUDWeaponAmmo(HUDWeaponAmmo);
				if (bIniitializeCarriedAmmo) SetHUDCarriedAmmo(HUDCarriedAmmo);
			}
		} 
	}
}

void ABaseCharacterPlayerController::HighPingWarning()
{
	BaseCharacterHUD = (BaseCharacterHUD == nullptr ? Cast<ABaseCharacterHUD>(GetHUD()) : BaseCharacterHUD);
	bool bHUDValid = BaseCharacterHUD &&
		BaseCharacterHUD->CharacterOverlay &&
		BaseCharacterHUD->CharacterOverlay->HighPingImage &&
		BaseCharacterHUD->CharacterOverlay->HighPingAnimation;
	if (bHUDValid) {
		BaseCharacterHUD->CharacterOverlay->HighPingImage->SetOpacity(1.f);
		BaseCharacterHUD->CharacterOverlay->PlayAnimation(BaseCharacterHUD->CharacterOverlay->HighPingAnimation);
	}
}

void ABaseCharacterPlayerController::StopHighPingWarning()
{
	BaseCharacterHUD = (BaseCharacterHUD == nullptr ? Cast<ABaseCharacterHUD>(GetHUD()) : BaseCharacterHUD);
	bool bHUDValid = BaseCharacterHUD &&
		BaseCharacterHUD->CharacterOverlay &&
		BaseCharacterHUD->CharacterOverlay->HighPingImage &&
		BaseCharacterHUD->CharacterOverlay->HighPingAnimation;
	if (bHUDValid) {
		BaseCharacterHUD->CharacterOverlay->HighPingImage->SetOpacity(0.f);
		if (BaseCharacterHUD->CharacterOverlay->IsAnimationPlaying(BaseCharacterHUD->CharacterOverlay->HighPingAnimation)) {
			BaseCharacterHUD->CharacterOverlay->StopAnimation(BaseCharacterHUD->CharacterOverlay->HighPingAnimation);
		}
	}
}

void ABaseCharacterPlayerController::ServerCheckMatchState_Implementation()
{
	/* 根据当前加入游戏的 MatchState, 初始化 PlayerController */
	ACombatGameMode* GameMode = Cast<ACombatGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode) {
		MatchState = GameMode->GetMatchState();
		LevelStartTime = GameMode->LevelStartingTime;
		WarmupTime = GameMode->WarmupTime;
		MatchTime = GameMode->MatchTime;
		CooldownTime = GameMode->CooldownTime;

		/* 将参数传入 各Client 的PlayerController: 使用 Client RPC, 只针对刚加入的Client, 一次性发送相关参数 */
		ClientJoinMidgame(MatchState, LevelStartTime, WarmupTime, MatchTime, CooldownTime);
	}
}

void ABaseCharacterPlayerController::ClientJoinMidgame_Implementation(FName State, float StartTime, float Warmup, float Match, float Cooldown)
{
	/** 所有Clients 都是中途加入 Server 的 */
	MatchState = State;
	LevelStartTime = StartTime;
	WarmupTime = Warmup;
	MatchTime = Match;
	CooldownTime = Cooldown;
	OnMatchStateSet(MatchState);
	if (BaseCharacterHUD && MatchState == MatchState::WaitingToStart) {
		BaseCharacterHUD->AddAnnouncement();
	}
}

void ABaseCharacterPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State; // 仅在 Server 中执行, 因为只有 Server 才有 GameMode

	if (MatchState == MatchState::InProgress) {
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown) {
		HandleCooldown();
	}
}

void ABaseCharacterPlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress) {
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown) {
		HandleCooldown();
	}
}

void ABaseCharacterPlayerController::HandleMatchHasStarted()
{
	BaseCharacterHUD = (BaseCharacterHUD == nullptr ? Cast<ABaseCharacterHUD>(GetHUD()) : BaseCharacterHUD);
	if (BaseCharacterHUD) {
		BaseCharacterHUD->AddCharacterOverlay(); // 正式比赛阶段, 才显示CharacterOverlay界面
		if (BaseCharacterHUD->Announcement) {
			BaseCharacterHUD->Announcement->SetVisibility(ESlateVisibility::Hidden); // 同时, 隐藏Announcement界面
		}
	}
}

void ABaseCharacterPlayerController::HandleCooldown()
{
	BaseCharacterHUD = (BaseCharacterHUD == nullptr ? Cast<ABaseCharacterHUD>(GetHUD()) : BaseCharacterHUD);
	if (BaseCharacterHUD) {
		BaseCharacterHUD->CharacterOverlay->RemoveFromParent();
		bool bHUDValid = BaseCharacterHUD->Announcement && 
			BaseCharacterHUD->Announcement->AnnouncementText &&
			BaseCharacterHUD->Announcement->InfoText;
		if (bHUDValid) {
			BaseCharacterHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
			FString AnnouncementText(TEXT("下一局比赛即将开始:"));
			BaseCharacterHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));

			ACombatGameState* CombatGameState = Cast<ACombatGameState>(UGameplayStatics::GetGameState(this));
			ABaseCharacterPlayerState* CharacterPlayerState = GetPlayerState<ABaseCharacterPlayerState>();
			FString InfoTextString;
			if (CombatGameState && CharacterPlayerState) {
				TArray<ABaseCharacterPlayerState*> TopPlayers = CombatGameState->TopScoringPlayers;
				if (TopPlayers.Num() == 0) {
					InfoTextString = FString(TEXT("无人获胜！"));
				}
				else if (TopPlayers.Num() == 1 && TopPlayers[0] == CharacterPlayerState) {
					InfoTextString = FString(TEXT("获胜！"));
				}
				else if (TopPlayers.Num() == 1) { // 只有一人获胜, 但不是 当前Controller 对应的Player
					InfoTextString = FString::Printf(TEXT("胜者: \n%s"), *TopPlayers[0]->GetPlayerName());
				}
				else if (TopPlayers.Num() > 1) {
					InfoTextString = FString::Printf(TEXT("胜者: \n"));
					for (auto TiedPlayer : TopPlayers) {
						InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
					}
				}
			}
			BaseCharacterHUD->Announcement->InfoText->SetText(FText::FromString(InfoTextString));
		}
	}
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(GetPawn());
	if (BaseCharacter) {
		BaseCharacter->bDisableDealDamage = true;
	}
}