#include "MainMenuWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/KismetSystemLibrary.h"
#include "FirstFPS/GameInstances/TPSGameInstance.h"
#include "Components/EditableTextBox.h"

#define LOCTEXT_NAMESPACE "MainMenu"

void UMainMenuWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // 绑定按钮点击事件。
    if (HostButton) {
        HostButton->OnClicked.AddUniqueDynamic(this, &UMainMenuWidget::HandleHostClicked);
    }

    if (JoinButton) {
        JoinButton->OnClicked.AddUniqueDynamic(this, &UMainMenuWidget::HandleJoinClicked);
    }

    if (QuitButton){
        QuitButton->OnClicked.AddUniqueDynamic(this, &UMainMenuWidget::HandleQuitClicked);
    }

    SetStatus(LOCTEXT("InitialStatus", "Please select an option"));
}

void UMainMenuWidget::FocusDefaultButton()
{
    APlayerController* OwningController = GetOwningPlayer();

    if (HostButton && OwningController) {
        HostButton->SetUserFocus(OwningController);
    }
}

void UMainMenuWidget::SetStatus(const FText& Message)
{
    if (StatusText) {
        StatusText->SetText(Message);
    }
}

void UMainMenuWidget::HandleHostClicked()
{
    UTPSGameInstance* GI = Cast<UTPSGameInstance>(GetGameInstance());

    if (!GI) {
        SetStatus(FText::FromString(
            TEXT("TPSGameInstance is not configured.")
        ));
        return;
    }

    SetStatus(FText::FromString(TEXT("Creating room...")));

    GI->HostLANGame();
}

void UMainMenuWidget::HandleJoinClicked()
{
    UTPSGameInstance* GI = Cast<UTPSGameInstance>(GetGameInstance());

    if (!GI)
    {
        SetStatus(FText::FromString(
            TEXT("TPSGameInstance is not configured.")
        ));
        return;
    }

    if (!IPAddressInput)
    {
        SetStatus(FText::FromString(
            TEXT("IPAddressInput is not configured.")
        ));
        return;
    }

    const FString Address = IPAddressInput->GetText().ToString();

    FString ErrorMessage;

    if (!GI->JoinLANGame(Address, ErrorMessage))
    {
        SetStatus(FText::FromString(ErrorMessage));
        return;
    }

    SetStatus(FText::FromString(TEXT("Connecting to host...")));
}

void UMainMenuWidget::HandleQuitClicked() {
    UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

#undef LOCTEXT_NAMESPACE