// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BaseCharacterPlayerController.generated.h"

class UInputMappingContext;
class ABaseCharacterHUD;
class UCharacterOverlay;

UCLASS()
class FIRSTFPS_API ABaseCharacterPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	virtual void Tick(float DeltaTime) override;

	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDMatchCountdown(float Countdown);
	void SetHUDTime();

	/* APlayerController::OnPossess(): 当 这个Controller 开始控制某个 Pawn 时, 触发该函数 */
	virtual void OnPossess(APawn* InPawn) override; 
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual float GetServerTime(); // 与Server时间同步
	virtual void ReceivedPlayer() override; /* PlayerController 的虚函数, 在当前 PlayerController 已经与对应网络连接建立关联后调用 */

	void OnMatchStateSet(FName State);

	UFUNCTION(Client, Reliable)
	void ClientHandleMatchEnded(
		bool bWon,
		const FString& WinnerName
	);

	/*
	 * 在 PlayerController 蓝图子类中实现这个事件，
	 * 创建并显示胜利/失败 Widget。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Match")
	void BP_ShowMatchResult(
		bool bWon,
		const FString& WinnerName
	);
protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	virtual void SetupInputComponent() override;

	/* 同步 Client 与 Server 时间 */
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest); // 给 Server 发送 Client当前时间(Server无需对该函数进行处理, 仅用于计算RTT)
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClient); // 给 Client 发送 Server当前时间

	float ClientServerDelta = 0.f; // Client与Server的时间差
	UPROPERTY(EditAnywhere, Category = "Time")
	float TimeSyncFrequency = 5.f; // 时间同步频率: 每5秒, 与Client时间进行一次同步 (时钟漂移、网络延迟导致RTT变动、客户端卡顿...)
	float TimeSyncRunningTime = 0.f;
private:
	ABaseCharacterHUD* BaseCharacterHUD;

	float MatchTime = 120.f; // 比赛总时长(2分钟)
	uint32 CountdownInt = 0;

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;
	UFUNCTION()
	void OnRep_MatchState();

	UPROPERTY()
	UCharacterOverlay* CharacterOverlay;
};