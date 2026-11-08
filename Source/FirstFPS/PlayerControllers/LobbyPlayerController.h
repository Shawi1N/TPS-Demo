// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LobbyPlayerController.generated.h"

class ULobbyWidget;
class UInputMappingContext;

UCLASS()
class FIRSTFPS_API ALobbyPlayerController : public APlayerController
{
    GENERATED_BODY()

protected:
    virtual void BeginPlay() override;

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(EditDefaultsOnly, Category = "Lobby")
    TSubclassOf<ULobbyWidget> LobbyWidgetClass;

    virtual void SetupInputComponent() override;
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TArray<UInputMappingContext*> DefaultMappingContexts;

private:
    UPROPERTY(Transient)
    ULobbyWidget* LobbyWidget = nullptr;
};