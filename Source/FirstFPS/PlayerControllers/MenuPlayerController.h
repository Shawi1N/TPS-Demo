// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MenuPlayerController.generated.h"

class UMainMenuWidget;


/**
 * 负责创建菜单实例，并设置本地玩家的输入方式。
 */
UCLASS()
class FIRSTFPS_API AMenuPlayerController : public APlayerController
{
    GENERATED_BODY()

protected:
    virtual void BeginPlay() override;

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // 在 BP_MenuPlayerController 中设置为 WBP_MainMenu。
    UPROPERTY(EditDefaultsOnly, Category = "Menu")
    TSubclassOf<UMainMenuWidget> MainMenuWidgetClass;

private:
    UPROPERTY(Transient)
    UMainMenuWidget* MainMenuWidget = nullptr;
};
