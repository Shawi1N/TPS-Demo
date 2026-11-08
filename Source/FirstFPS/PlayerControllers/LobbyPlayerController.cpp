// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyPlayerController.h"
#include "FirstFPS/HUD/LobbyWidget.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"

void ALobbyPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (!IsLocalController()) {
        return;
    }

    // 大厅允许控制角色。
    SetInputMode(FInputModeGameOnly());
    bShowMouseCursor = false;

    // UI 创建失败也不影响角色操作。
    if (!LobbyWidgetClass)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("LobbyWidgetClass is not configured.")
        );
        return;
    }

    LobbyWidget = CreateWidget<ULobbyWidget>(this, LobbyWidgetClass);

    if (LobbyWidget) {
        LobbyWidget->AddToViewport(100);

        // 大厅 UI 仅显示信息，不拦截鼠标命中。
        LobbyWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
}

void ALobbyPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (LobbyWidget) {
        LobbyWidget->RemoveFromParent();
        LobbyWidget = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}

void ALobbyPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (!IsLocalController()) {
        return;
    }

    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    if (!LocalPlayer) {
        return;
    }

    UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);

    if (!InputSubsystem) {
        return;
    }

    for (UInputMappingContext* MappingContext : DefaultMappingContexts) {
        if (MappingContext) {
            InputSubsystem->AddMappingContext(MappingContext, 0);
        }
    }
}