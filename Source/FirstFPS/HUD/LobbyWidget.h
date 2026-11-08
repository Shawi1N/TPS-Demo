// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LobbyWidget.generated.h"

class UTextBlock;

UCLASS()
class FIRSTFPS_API ULobbyWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    UPROPERTY(meta = (BindWidget))
    UTextBlock* PlayerCountText = nullptr;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* StatusText = nullptr;

    FTimerHandle RefreshTimer;

    void RefreshLobbyStatus();
};
