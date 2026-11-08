// Fill out your copyright notice in the Description page of Project Settings.


#include "MenuPlayerController.h"
#include "FirstFPS/HUD/MainMenuWidget.h"

void AMenuPlayerController::BeginPlay()
{
    Super::BeginPlay();

    // 只在本地玩家的 Controller 上创建 UI
    if (!IsLocalController()) {
        return;
    }

    if (!MainMenuWidgetClass) {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("MainMenuWidgetClass is not configured.")
        );
        return;
    }

    MainMenuWidget = CreateWidget<UMainMenuWidget>(this, MainMenuWidgetClass);

    if (!MainMenuWidget)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("Failed to create MainMenuWidget.")
        );
        return;
    }

    MainMenuWidget->AddToViewport(100);

    FInputModeUIOnly InputMode;
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

    SetInputMode(InputMode);

    bShowMouseCursor = true;

    MainMenuWidget->FocusDefaultButton();
}

void AMenuPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (MainMenuWidget) {
        MainMenuWidget->RemoveFromParent();
        MainMenuWidget = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}