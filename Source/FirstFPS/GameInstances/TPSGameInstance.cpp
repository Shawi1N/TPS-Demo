// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSGameInstance.h"

#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

void UTPSGameInstance::HostLANGame()
{
    if (!GetWorld())
    {
        return;
    }

    // listen：让当前玩家同时作为服务器和本地玩家。
    UGameplayStatics::OpenLevel(this, LobbyMapName, true, TEXT("listen"));
}

bool UTPSGameInstance::JoinLANGame(const FString& Address, FString& OutError)
{
    OutError.Reset();

    FString CleanAddress = Address;
    CleanAddress.TrimStartAndEndInline();

    if (CleanAddress.IsEmpty()) {
        OutError = TEXT("Please enter the host IP.");
        return false;
    }

    // 本示例仅接受 IPv4 或 IPv4:Port。
    // 例如：192.168.1.100 或 192.168.1.100:7777。
    FString IP = CleanAddress;
    FString PortText = TEXT("7777");

    if (CleanAddress.Contains(TEXT(":")))
    {
        CleanAddress.Split(TEXT(":"), &IP, &PortText);
    }

    auto IsDecimal = [](const FString& Value) -> bool
        {
            if (Value.IsEmpty())
            {
                return false;
            }

            for (const TCHAR Character : Value)
            {
                if (Character < TEXT('0') || Character > TEXT('9'))
                {
                    return false;
                }
            }

            return true;
        };

    TArray<FString> Parts;
    IP.ParseIntoArray(Parts, TEXT("."), false);

    if (Parts.Num() != 4)
    {
        OutError = TEXT("Invalid IPv4 address.");
        return false;
    }

    for (const FString& Part : Parts)
    {
        if (Part.Len() > 3 ||
            !IsDecimal(Part) ||
            FCString::Atoi(*Part) > 255)
        {
            OutError = TEXT("Invalid IPv4 address.");
            return false;
        }
    }

    if (PortText.Len() > 5 || !IsDecimal(PortText))
    {
        OutError = TEXT("Invalid port.");
        return false;
    }

    const int32 Port = FCString::Atoi(*PortText);

    if (Port < 1 || Port > 65535)
    {
        OutError = TEXT("Port must be between 1 and 65535.");
        return false;
    }

    APlayerController* PC = GetFirstLocalPlayerController();

    if (!PC)
    {
        OutError = TEXT("Local PlayerController was not found.");
        return false;
    }

    const FString ConnectAddress = FString::Printf(TEXT("%s:%d"), *IP, Port);

    UE_LOG(
        LogTemp,
        Log,
        TEXT("Connecting to %s"),
        *ConnectAddress
    );

    PC->ClientTravel(ConnectAddress, TRAVEL_Absolute);
    return true;
}

void UTPSGameInstance::LeaveGame()
{
    UGameplayStatics::OpenLevel(
        this,
        MenuMapName,
        true
    );
}