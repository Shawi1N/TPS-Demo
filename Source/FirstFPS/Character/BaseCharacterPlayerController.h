// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BaseCharacterPlayerController.generated.h"

// 声明一个 "动态多播委托" 类型
// DYNAMIC: 可以和 UE 反射系统、蓝图 配合;
// MULTICAST: 可以绑定多个函数;
// OneParam: 这个委托广播时带 1 个参数
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHighPingDelegate, bool, bPingTooHigh);
// 创建一种 委托类型FHighPingDelegate, 用该类型名创建的 委托实例 可以同时通知多个监听者, 并且广播时会附带一个参数 bool bPingTooHigh

class UInputMappingContext;
class ABaseCharacterHUD;
class UCharacterOverlay;
class ACombatGameMode;

UCLASS()
class FIRSTFPS_API ABaseCharacterPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	virtual void Tick(float DeltaTime) override;

	/** 处理HUD */
	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDShield(float Shield, float MaxShield);
	void SetHUDScore(float Score);
	void SetHUDDeaths(int32 Death);
	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDCarriedAmmo(int32 Ammo);
	void SetHUDAnnouncementCountdown(float Countdown);
	void SetHUDMatchCountdown(float Countdown);

	/* APlayerController::OnPossess(): 当 这个Controller 开始控制某个 Pawn 时, 触发该函数 */
	virtual void OnPossess(APawn* InPawn) override; 
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual float GetServerTime(); /* 统一获取Server的游戏开局时间 (统一与 Server时间 同步) */
	/** PlayerController 自带的虚函数, 当 PlayerController 的 viewport / net connection 已经关联后立即调用. 此处用于计算服务器时间同步 */
	virtual void ReceivedPlayer() override; // 客户端必须已经具备有效的本地 PlayerController 和网络连接, Server RPC 才有明确的发送通道

	void OnMatchStateSet(FName State); // 将 GameMode::MatchState的值 传入 PlayerController的MatchState变量

	void HandleMatchHasStarted();
	void HandleCooldown();

	float SingleTripTime = 0.f; // 单程传输时间 (用于服务器回溯)
	FHighPingDelegate HighPingDelegate; /* 多播委托 实例*/

protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;
	virtual void SetupInputComponent() override;

	void SetHUDTime();

	/* 同步 Client 与 Server 时间 */
	UFUNCTION(Server, Reliable) // 服务器RPC: 给 Server 发送 Client当前时间 (用于计算RTT)
	void ServerRequestServerTime(float TimeOfClientRequest);
	UFUNCTION(Client, Reliable) // 客户端RPC: 给 Client 发送 Server当前时间
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

	float ClientServerDelta = 0.f; // Client与Server的时间差
	UPROPERTY(EditAnywhere, Category = "Time")
	float TimeSyncFrequency = 5.f;	 // 时间同步频率: Clients每隔多少秒, 需要向Server请求一次时间校准 (降低时钟漂移和网络延迟波动造成的误差)
	float TimeSyncRunningTime = 0.f; // 时间同步累计计时器: 记录距离上一次向服务器请求时间同步, 已经过去了多少秒

	void PollInit();

	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();
	UFUNCTION(Client, Reliable)
	void ClientJoinMidgame(FName State, float StartTime, float Warmup, float Match, float Cooldown);

	/* 触发 高延迟警告 (Widget Animation) 的相关函数 */
	void HighPingWarning();
	void StopHighPingWarning();
	void CheckPing(float DeltaTime);
private:
	UPROPERTY()
	ABaseCharacterHUD* BaseCharacterHUD;
	UPROPERTY()
	UCharacterOverlay* CharacterOverlay;

	/** HUD显示的各类时间参数 */
	float LevelStartTime = 0.f; // 当 ACombatGameMode 开始运行时的当前世界时间, 作为本局比赛开局倒计时的起始基准
	float WarmupTime = 0.f;		// 正式比赛前的等待时长: 由GameMode设置具体时长
	float MatchTime = 0.f;		// 单局比赛总时长: 同上
	float CooldownTime = 0.f;	// 正式比赛结束, 下一局比赛开始的等待时长
	uint32 CountdownInt = 0;

	UPROPERTY()
	ACombatGameMode* CombatGameMode;

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState; // 记录 GameMode 此时对应的比赛状态
	UFUNCTION()
	void OnRep_MatchState();

	/* 将 Character属性 缓存起来, 等 CharacterOverlay变量被赋值 后再设置 */
	float HUDHealth;
	bool bInitializeHealth = false;
	float HUDMaxHealth;
	float HUDShield;
	bool bInitializeShield = false;
	float HUDMaxShield;
	float HUDScore;
	bool bInitializeScore = false;
	float HUDDeaths;
	bool bInitializeDeaths = false;
	float HUDWeaponAmmo;
	bool bIniitializeWeaponAmmo = false;
	float HUDCarriedAmmo;
	bool bIniitializeCarriedAmmo = false;

	float HighPingRunningTime = 0.f;	  // 自上次显示 高延迟警告 以来的时长
	float PingAnimationRunningTime = 0.f; // 高延迟警告动画 播放时长
	UPROPERTY(EditAnywhere)
	float HighPingDuration = 5.f;		  // 高延迟警告的显示时长
	UPROPERTY(EditAnywhere)
	float CheckPingFrequency = 20.f;	  // 检查 延迟 的频率
	UFUNCTION(Server, Reliable)
	void ServerReportPingStatus(bool bHighPing);
	UPROPERTY(EditAnywhere)
	float HighPingThreshold = 50;		  // 延迟 超过该值, 显示 高延迟警告
};