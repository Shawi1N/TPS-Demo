// Fill out your copyright notice in the Description page of Project Settings.


#include "MenuGameMode.h"
#include "FirstFPS/PlayerControllers/MenuPlayerController.h"

AMenuGameMode::AMenuGameMode()
{
    // 菜单阶段不生成 Pawn。
    DefaultPawnClass = nullptr;

    // 默认使用菜单 Controller。
    // 后面在 BP_MenuGameMode 中改为其配置了资源的蓝图子类。
    PlayerControllerClass = AMenuPlayerController::StaticClass();

    // 主菜单由 UUserWidget 显示，不需要额外的 AHUD。
    HUDClass = nullptr;
}